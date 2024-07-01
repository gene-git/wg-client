/*
 * Restore /etc/resolv.con by copying from saved /etc/resolv.conf.wg
 * Requires cap_chown and cap_dac_override if not run as root.
 *
 * Uses 
 *      - openssl to compute file hash for fast compare
 * Since resolv.conf are always tiny we read them all into memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/capability.h>
#include <openssl/evp.h>

#define OPENSSL_ENGINE NULL
#define BUFSZ 10240

struct perms {
    uid_t   euid ;
    bool cap_chown ;
    bool cap_dac_override;
};

struct file_data {
    const char *pathname ;
    unsigned char *data ;
    unsigned int data_len ;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len ;
    bool good_file ;
};

#if defined(TESTING)
//
// testing tool : print digest it in standard hex form
//
static void print_digest(struct file_data *fdata)
{
    unsigned int i;
    for (i = 0; i < fdata->digest_len; i++)
        printf("%02x", fdata->digest[i]);
    printf("\n");
}
#endif


//
// Check caps/perms
//
static int check_permissions(struct perms *perms) 
{
    cap_t cap ;
    cap_flag_value_t cap_val ;

    perms->euid = geteuid() ;
    perms->cap_chown = false ;
    perms->cap_dac_override = false ;
    if (perms->euid == 0) {
        perms->cap_chown = true;
        perms->cap_dac_override = true ;
        return(0);
    }

	cap = cap_get_pid(0);
	if (cap == NULL) {
		perror("cap_get_pid");
		return(-1);
	}

    cap_get_flag(cap, CAP_CHOWN, CAP_PERMITTED, &cap_val);
    if (cap_val == CAP_SET) {
        perms->cap_chown = true ;
    }

    cap_get_flag(cap, CAP_DAC_OVERRIDE, CAP_PERMITTED, &cap_val);
    if (cap_val == CAP_SET) {
        perms->cap_dac_override = true ;
    }
    return(0) ;
}

//
// Compute file hash
//  - use openssl crypto lib to do the work
//
static const int sha_hash(struct file_data *fdata)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    if (fdata == NULL || fdata->data == NULL || fdata->data_len < 1){
        printf("Hash: missing input\n") ;
        return(-1) ;
    }
    fdata->digest_len = 0 ;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), OPENSSL_ENGINE) < 0) {
        printf("Hash: Error Digest init\n");
        EVP_MD_CTX_free(ctx);
        return(-1) ;
    }

    // Hash data into the digest context ctx
    if (EVP_DigestUpdate(ctx, fdata->data, fdata->data_len) < 0) {
        printf("Hash: digest update failed\n");
        EVP_MD_CTX_free(ctx);
        return(-1) ;
    }

    // retrieve digest value from ctx and save into provided digest buffer
    if (EVP_DigestFinal_ex(ctx, fdata->digest, &fdata->digest_len) < 0) {
        printf("Hash: digest finalization failed.\n");
        EVP_MD_CTX_free(ctx);
        return(-1) ;
    }

    // clean up
    EVP_MD_CTX_free(ctx);

    return(0);
}

//
// Read fdata->pathname and return allocated buffer of contents.
// resolv.conf is small typically < 5K so its fine to read all into memory before writing.
// After successful read:
//  - Data is saved in fdata->data
//  - Data is hashed into fdata->digest using sha_hash()
// Returns:
//    0 = success
//   -1 = error
// Caller should free buffer fdata->data
//
static int read_file(struct file_data *fdata)
{
    unsigned int bytes = -1 ;
    size_t chunk = 0;
    int fdin = 0, ret ;

    fdata->data = NULL ;
    fdata->data_len = 0 ;
    fdata->good_file = true ;

    //
    // Read file in BUFSZ chunks
    //
    fdin = open(fdata->pathname, O_RDONLY) ;
    if (fdin < 0) {
        printf("Failed to open file %s : %s\n", fdata->pathname, strerror(errno));
        fdata->good_file = false ;
        return(-1);
    }

    chunk = BUFSZ ;
    fdata->data = (unsigned char *) calloc(1, chunk);
    if (fdata->data == NULL) {
        printf("Failed to allocate mem : %s\n", strerror(errno));
        fdata->good_file = false ;
        return(-1);
    }

    do {
        bytes = read(fdin, &(fdata->data[fdata->data_len]), BUFSZ) ;
        if (bytes < 0) {
            printf("Error reading %s : %s\n", fdata->pathname, strerror(errno));
            fdata->good_file = false ;
            return(-1);
        }
        fdata->data_len += bytes;
        if (bytes == 0) {
            break ;
        } else {
            chunk += BUFSZ ;
            fdata->data = (unsigned char *)realloc((void *) fdata->data, chunk) ;
            if (fdata->data == NULL) {
                printf("Failed to reellocate mem : %s\n", strerror(errno));
                fdata->good_file = false ;
                return(-1);
            }
        }
        fdata->data[fdata->data_len] = 0;
    } while(bytes > 0);

    close(fdin) ;

    //
    // All good - compute the hash 
    //
    ret = sha_hash(fdata);
    if (ret < 0) {
        printf("Failed to generae digest : %s\n", fdata->pathname);
        return(-1);
    }
    return(0);
}

//
// Write data from fdata->data to pathname  
// Returns 0 on success and -1 on error
//
static int write_file(struct file_data *fdata, const char *pathname)
{
    int ret, fdout;
    char path_tmp[MAXPATHLEN];
    pid_t pid ;

    pid = getpid();
    snprintf(path_tmp, MAXPATHLEN, "%s.%X", fdata->pathname, (unsigned int)pid);

    fdout = open(path_tmp, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fdout < 0) {
        printf("Failed to open path %s : %s\n", pathname, strerror(errno));
        return(-1) ;
    }

    if (fdata->data_len > 0) {
        ret = write(fdout, fdata->data, fdata->data_len);
        if (ret < 0){
            printf("Failed to write path %s : %s\n", path_tmp, strerror(errno));
            return(-1) ;
        }
    }
    close(fdout) ;

    //
    // All is well - Rename tmp file 
    //
    ret = rename((const char *)path_tmp, pathname) ;
    if (ret < 0) {
        printf("Failed to rename %s to %s : %s\n", path_tmp, pathname, strerror(errno));
        return(-1) ;
    }
    return(0) ;
}

//
// Chown to root if possible
//
static int chown_root(struct perms *perms, const char *pathname) {

    if (perms->euid == 0 || (perms->euid != 0 && perms->cap_chown == true)) {
        if (chown(pathname, 0, 0)) {
            printf("Failed chown root : %s\n", pathname);
            return(-1);
        }
    }
    return(0);
}

//
// compare 2 files 
//
static int file_compare(struct file_data *fd1, struct file_data *fd2){
    int comp ;
    const char *digest1 = (const char *)fd1->digest ;
    const char *digest2 = (const char *)fd2->digest ;

    //
    // check length first then digest
    //
    if (fd1->digest_len < fd2->digest_len) {
        comp = -1 ;
    } else if (fd1->digest_len > fd2->digest_len) {
        comp = 1 ;
    } else {
        comp = strncmp(digest1, digest2, EVP_MAX_MD_SIZE) ;
    }
    return (comp) ;
}

//
// App to manage resolv.conf file and ensure 
// wireguard version is in /etc/resolv.conf
// While VPN is running networking tools (e.g. dhcp)
// may replace it.
// Requires capabilities : CAP_CHOWN, CAP_DAC_OVERRIDE
//  - caps are needed to write /etc/resolv.conf
// Hard code pathnames to minimize attack surface.
// NB: any change to binary will cause caps to be dropped
//
int main(int argc, char **argv) {
    int ret = -1 ;
    struct perms perms ;
    struct file_data fdata_wg, fdata_save, fdata_resolv ;

    memset((void *)&fdata_wg, 0, sizeof(fdata_wg)) ;
    memset((void *)&fdata_save, 0, sizeof(fdata_save)) ;
    memset((void *)&fdata_resolv, 0, sizeof(fdata_resolv)) ;

    fdata_wg.pathname = "/etc/resolv.conf.wg" ;
    fdata_save.pathname = "/etc/resolv.conf.saved" ;
    fdata_resolv.pathname = "/etc/resolv.conf" ;

    check_permissions(&perms);
    if (perms.cap_dac_override == false) {
        printf("Warning : missing CAP_DAC_OVERRIDE capability\n");
    }

    //
    // Read wg resolv
    //  - quit if not found 
    //
    ret = read_file(&fdata_wg);
    if (ret < 0) {
        return(-1);
    }

    //
    // Read resolv
    //
    ret = read_file(&fdata_resolv);
    if (ret < 0) {
        // If Missing resolv.conf then replace with wg version
        ret = write_file(&fdata_wg, fdata_resolv.pathname) ;
        if (ret < 1) {
            return(-1);
        }
        return (0);
    }

    //
    // Check resolv.conf == resolv.conf.save
    //
    ret = read_file(&fdata_save);
    if (ret < 0) {
        printf("Failed read : %s\n", fdata_save.pathname);
    } 
    
    //
    // Check resolv.conf :
    //   make sure it is same as resolv.conf.wg
    //
    if (file_compare(&fdata_resolv, &fdata_wg) != 0) {
        //
        // resolv.conf doesn't match wireguard version so replace it.
        //
        ret = write_file(&fdata_wg, fdata_resolv.pathname) ;
        if (ret < 0) {
            return(-1);
        }

        //
        // resolv.conf.saved
        //   check the changed resolv.conf is same as saved
        //   if not, then updated saved
        //   Should only happen if moved to new network location with different default resolver
        //
        if (!fdata_save.good_file || 
            file_compare(&fdata_resolv, &fdata_save) != 0){
            printf("Updating : %s\n", fdata_save.pathname);
            ret = write_file(&fdata_resolv, fdata_save.pathname);
            if (ret < 0) {
                return(-1);
            }
        }
    } 

    free((void *)fdata_wg.data) ;
    free((void *)fdata_resolv.data) ;
    free((void *)fdata_save.data) ;

    //
    // chown to root if needed and permitted
    //  - leave saved and wg files alone
    //
    chown_root(&perms, fdata_resolv.pathname) ;

    return(0);
}
