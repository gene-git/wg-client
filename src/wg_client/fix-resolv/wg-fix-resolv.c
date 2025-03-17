/*
 * Restore /etc/resolv.con by copying from saved /etc/resolv.conf.wg
 * Save any newly found version of file as resolv.conf.save. This handles
 * the case when machine is sleep/resumed or dhcp lease is refreshed or
 * even a new wifi location - each leading to a new resolv.conf - if its different
 * than current saveed version then it replaces it.
 *
 * Requires :
 *  - cap_chown and cap_dac_override if not run as root.
 *  - openssl to compute file hash for fast compare
 *
 * Since resolv.conf files are tiny they are read into memory.
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
#include <openssl/err.h>

#define OPENSSL_ENGINE NULL
#define BUFSZ 10240

// track permissions / capabilities
struct perms {
    uid_t   euid ;
    bool cap_chown ;
    bool cap_dac_override;
};

// file information including a hash used for fast compare
struct file_data {
    const char *pathname ;
    unsigned char *data ;
    unsigned int data_len ;
    bool data_is_good ;
    const char *digest_algo;
    unsigned char *digest;
    unsigned int digest_len ;
};

#if defined(TESTING)
//
// testing tool : print digest in standard hex form
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
// Compute digest of the file data 
//  - use openssl crypto lib to do the work
//
static const int compute_digest(struct file_data *fdata)
{
    EVP_MD_CTX *ctx = NULL;
    EVP_MD *md = NULL;
    int ret = 0;

    if (fdata == NULL || fdata->data == NULL || fdata->data_len < 1){
        printf("Digest error: missing input\n") ;
        ret = -1;
        goto clean_up;
    }
    fdata->digest_len = 0 ;

    //
    // which algorithm
    //
    if (fdata->digest_algo == NULL) {
        fdata->digest_algo = "SHA384";
    }

    //
    // Make new md and ctx
    //
    md = EVP_MD_fetch(NULL, fdata->digest_algo, NULL);
    if (md == NULL){
        printf("Digest error: failed allocate md\n");
        ret = -1;
        goto clean_up;
    }

    ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        printf("Digest error: failed allocate ctx\n");
        ret = -1;
        goto clean_up;
    }

    if (EVP_DigestInit_ex(ctx, md, OPENSSL_ENGINE) < 0) {
        printf("Digest errir: Digest init\n");
        ret = -1;
        goto clean_up;
    }

    //
    // Pass data into the digest context ctx
    //
    if (EVP_DigestUpdate(ctx, fdata->data, fdata->data_len) < 0) {
        printf("Digest error: update failed\n");
        ret = -1;
        goto clean_up;
    }

    //
    // Make space for digest output
    //
    fdata->digest = OPENSSL_malloc(EVP_MD_get_size(md));
    if (fdata->digest == NULL) {
        printf("Digest error: memory alloc failed\n");
        ret = -1;
        goto clean_up;
    }

    //
    // Calculate digest value and save into allocated digest buffer
    //
    if (EVP_DigestFinal_ex(ctx, fdata->digest, &fdata->digest_len) < 0) {
        printf("Hash: digest finalization failed.\n");
        ret = -1;
        goto clean_up;
    }

clean_up:
    if (ctx != NULL)
        EVP_MD_CTX_free(ctx);
    if (md != NULL)
        EVP_MD_free(md);
    if (ret != 0)
        ERR_print_errors_fp(stderr);

    return(ret);
}


//
// mem_alloc helper func
//  - if curr_mem NULL then calloc() else realloc()
//  - for convenience bytes is ssize_t
//
static unsigned char *mem_alloc(unsigned char *curr_mem, ssize_t bytes)
{
    unsigned char *mem = NULL ;
    const char *which = "";

    if (curr_mem == NULL) {
        mem = (unsigned char *) calloc(1, (size_t)bytes);
    } 
    else {
        which = "re";
        mem = (unsigned char *) realloc((void *) curr_mem, (size_t)bytes) ;
    }

    if (mem == NULL) {
        printf("Error %sallocating %zd bytes: %s\n", which, bytes, strerror(errno));
    }
    return (mem);
}

//
// Read File
//   read fdata->pathname and return allocated buffer of contents.
//   resolv.conf is small (typically << 5K) so keep in memory
// After successful read:
//  - Data is saved in fdata->data
//  - Data is hashed into fdata->digest using compute_digest()
//  - comments are stripped (any line starting with '#')
// Returns:
//    0 = success
//   -1 = error
// Caller responsibility: free up data : free(fdata->data)
//
static int read_file(struct file_data *fdata)
{
    ssize_t bytes = -1 ;
    ssize_t bytes_total = 0;
    int fdin = 0, ret ;

    fdata->data = NULL ;
    fdata->data_len = 0 ;
    fdata->data_is_good = true ;

    //
    // Read file in chunks (BUFSZ) - size chosen to typically complete in one read
    //
    fdin = open(fdata->pathname, O_RDONLY) ;
    if (fdin < 0) {
        printf("Failed to open file %s : %s\n", fdata->pathname, strerror(errno));
        fdata->data_is_good = false ;
        return(-1);
    }

    //
    // Allocate space
    //
    bytes_total = BUFSZ ;
    fdata->data = mem_alloc(fdata->data, bytes_total);
    if (fdata->data == NULL) {
        fdata->data_is_good = false ;
        return(-1);
    }

    do {
        bytes = read(fdin, &(fdata->data[fdata->data_len]), BUFSZ) ;
        if (bytes < 0) {
            printf("Error reading %s : %s\n", fdata->pathname, strerror(errno));
            fdata->data_is_good = false ;
            return(-1);
        }
        fdata->data_len += (unsigned int)bytes;

        if (bytes < BUFSZ) {
            // all done all good
            break ;
        } else {
            bytes_total += BUFSZ ;
            fdata->data = mem_alloc(fdata->data, bytes_total);
            if (fdata->data == NULL) {
                fdata->data_is_good = false ;
                return(-1);
            }
        }
    } while(bytes > 0);

    //
    // Strip out any comments
    //


    //
    // Resize down to free up unused mem
    //
    bytes = fdata->data_len ;
    fdata->data = mem_alloc(fdata->data, bytes);
    if (fdata->data == NULL) {
        fdata->data_is_good = false ;
        return(-1);
    }

    close(fdin) ;

    //
    // All good - compute the hash 
    //
    ret = compute_digest(fdata);
    if (ret < 0) {
        printf("Failed to compute digest : %s\n", fdata->pathname);
        return(-1);
    }
    return(0);
}

//
// Write fdata->data to file : pathname  
// Returns 0 on success and -1 on error
// fdata->data and fdata->data_len should be valid
//
static int write_file(struct file_data *fdata, const char *pathname)
{
    int ret, fdout;
    char path_tmp[MAXPATHLEN];
    pid_t pid ;

    if (fdata->data == NULL || fdata->data_len <= 0){
        printf("No data to write to : %s\n", pathname);
        return(-1) ;
    }

    //
    // Sufficient for our temp file - simpler than mkstemp
    //
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
    // All well - Rename tmp file 
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

    if (perms->euid == 0 || perms->cap_chown == true) {
        if (chown(pathname, 0, 0)) {
            printf("Failed chown root : %s\n", pathname);
            return(-1);
        }
    }
    return(0);
}

//
// compare data of 2 files 
// Return true if same else return false
//
static bool files_same(struct file_data *fd1, struct file_data *fd2){
    //
    // check data length first then check digest
    // Both digests from same algo and are same length
    //
    if (fd1->data_len != fd2->data_len) {
        return (false);
    } 
    else if (strncmp((const char *)fd1->digest, (const char *)fd2->digest, fd1->digest_len) == 0) {
        return (true);
    } 
    return (false);
}

//
// Clean up allocated memory
//
static void clean_mem(struct file_data *fdata)
{
    if (fdata->data != NULL) {
        free((void *)fdata->data) ;
    }
    if (fdata->digest != NULL) {
        OPENSSL_free(fdata->digest);
        fdata->digest_len = 0;
    }
}

//
// Program to manage resolv.conf file and ensure 
// wireguard version is in /etc/resolv.conf
// While VPN is running, some events lead  networking tools 
// (e.g. dhcp) to replace it.
//
// When wireguard exits it restores the original resolv.conf
// using PostDown wireguard config.
//
// Our job is to keep the correct wireguard resolv.conf
//
// Requires capabilities : CAP_CHOWN, CAP_DAC_OVERRIDE
//  - caps are needed to write /etc/resolv.conf
//  or owner has appropriate file permissions.
// Hard code pathnames to minimize any attack surface.
// NB: any change to binary will cause caps to be dropped
//
int main(int argc, char **argv) {
    int ret = -1 ;
    struct perms perms ;
    struct file_data fdata_wg, fdata_save, fdata_resolv ;
    const char *digest_algo = "SHA384" ;

    memset((void *)&fdata_wg, 0, sizeof(fdata_wg)) ;
    memset((void *)&fdata_save, 0, sizeof(fdata_save)) ;
    memset((void *)&fdata_resolv, 0, sizeof(fdata_resolv)) ;

    fdata_wg.pathname = "/etc/resolv.conf.wg" ;
    fdata_save.pathname = "/etc/resolv.conf.saved" ;
    fdata_resolv.pathname = "/etc/resolv.conf" ;

    fdata_wg.digest_algo  = digest_algo;
    fdata_save.digest_algo  = digest_algo;
    fdata_resolv.digest_algo  = digest_algo;

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
        printf("Error : missing file %s\n", fdata_wg.pathname);
        return(-1);
    }

    //
    // Read resolv.conf
    //
    ret = read_file(&fdata_resolv);
    if (ret < 0) {
        //
        // If Missing resolv.conf then replace with wg version
        //
        printf("Restoring missing %s from wg version %s\n", fdata_resolv.pathname, fdata_wg.pathname);
        ret = write_file(&fdata_wg, fdata_resolv.pathname) ;
        if (ret < 1) {
            return(-1);
        }
        return (0);
    }

    //
    // Read resolv.conf.saved
    //
    ret = read_file(&fdata_save);
    if (ret < 0) {
        printf("Warning: Unable to read : %s\n", fdata_save.pathname);
    } 
    
    //
    // Check resolv.conf :
    //   make sure it is same as resolv.conf.wg
    //   chown root if permitted any newly written file.
    //
    if (!files_same(&fdata_resolv, &fdata_wg)) {
        //
        // resolv.conf changed and doesn't match wireguard version so replace it.
        //
        printf("Updating : %s\n", fdata_resolv.pathname);
        ret = write_file(&fdata_wg, fdata_resolv.pathname) ;
        if (ret < 0) {
            return(-1);
        }
        chown_root(&perms, fdata_resolv.pathname) ;

        //
        // resolv.conf.saved
        //   check the new resolv.conf is same as saved and update saved if not same
        //   Probably only happens after changing network location with different default resolver
        //
        if (!fdata_save.data_is_good || !files_same(&fdata_resolv, &fdata_save)){
            printf("Updating : %s\n", fdata_save.pathname);
            ret = write_file(&fdata_resolv, fdata_save.pathname);
            if (ret < 0) {
                return(-1);
            }
            chown_root(&perms, fdata_save.pathname) ;
        }
    } 


    //
    // Clean up mem
    //
    clean_mem(&fdata_wg);
    clean_mem(&fdata_resolv);
    clean_mem(&fdata_save);

    return(0);
}
