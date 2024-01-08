/*
 * Restore /etc/resolv.con by copying from saved /etc/resolv.conf.wg
 * Requires cap_chown and cap_dac_override if not run as root.
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

#define BUFSZ 10240

struct perms {
    uid_t   euid ;
    bool cap_chown ;
    bool cap_dac_override;
};

struct file_data {
    const char *infile ;
    char *data ;
    int count ;
};

static int check_permissions(struct perms *perms) 
{
    //
    // Check permissions
    //
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

static int read_file(struct file_data *fdata)
{
    //
    // Read file_A and return allocated buffer of contents.
    // resolv.conf is small typically < 5K so its fine to read all into memory before writing.
    // Returns data buffer of contents or NULL on error
    // Caller should free the buffer
    //
    int bytes=-1 ;
    size_t chunk=0;
    int fdin=0 ;

    fdata->count = 0 ;
    fdata->data = NULL ;

    //
    // Read file into buffer created in chunks of BUFSZ
    //
    fdin = open(fdata->infile, O_RDONLY) ;
    if (fdin < 0) {
        printf("Failed to open file %s : %s\n", fdata->infile, strerror(errno));
        return(-1);
    }

    chunk = BUFSZ ;
    fdata->data = (char *) calloc(1, chunk);
    if (fdata->data == NULL) {
        printf("Failed to allocate mem : %s\n", strerror(errno));
        return(-1);
    }

    fdata->count = 0;
    do {
        bytes = read(fdin, &(fdata->data[fdata->count]), BUFSZ) ;
        if (bytes < 0) {
            printf("Error reading %s : %s\n", fdata->infile, strerror(errno));
            return(-1);
        }
        fdata->count += bytes;
        if (bytes == 0) {
            break ;
        } else {
            chunk += BUFSZ ;
            fdata->data = (char *)realloc((void *) fdata->data, chunk) ;
            if (fdata->data == NULL) {
                printf("Failed to reellocate mem : %s\n", strerror(errno));
                return(-1);
            }
        }
        fdata->data[fdata->count] = 0;
    } while(bytes > 0);

    close(fdin) ;

    return(0);
}

static int write_file(struct file_data *fdata, const char *file)
{
    //
    // Write output file 
    // Returns 0 on success and -1 on error
    //
    int ret, fdout;
    char file_tmp[MAXPATHLEN];
    pid_t pid ;

    pid = getpid();
    snprintf(file_tmp, MAXPATHLEN, "%s.%X", fdata->infile, (unsigned int)pid);

    fdout = open(file_tmp, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fdout < 0) {
        printf("Failed to open file %s : %s\n", file, strerror(errno));
        return(-1) ;
    }

    if (fdata->count > 0) {
        ret = write(fdout, fdata->data, fdata->count);
        if (ret < 0){
            printf("Failed to write file %s : %s\n", file_tmp, strerror(errno));
            return(-1) ;
        }
    }
    close(fdout) ;

    //
    // All is well - Rename tmp file 
    //
    ret = rename((const char *)file_tmp, file) ;
    if (ret < 0) {
        printf("Failed to rename %s to %s : %s\n", file_tmp, file, strerror(errno));
        return(-1) ;
    }
    return(0) ;
}

int main(int argc, char **argv) {
    //
    // Requires capabilities : CAP_CHOWN, CAP_DAC_OVERRIDE
    // caps needed to write /etc/resolv.conf
    // Hard code pathnames to minimize attack surface.
    // NB: any change to binary will cause caps to be dropped
    //
    const char *infile = "/etc/resolv.conf.wg" ;
    const char *outfile = "/etc/resolv.conf";
    int ret = -1 ;
    struct perms perms ;
    struct file_data file_data ;

    check_permissions(&perms);

    if (perms.cap_dac_override == false) {
        printf("Warning : missing CAP_DAC_OVERRIDE capability\n");
    }

    // Read /etc/resolv.conf.wg
    file_data.infile = infile ;
    ret = read_file(&file_data);
    if (ret < 0) {
        printf("Read failed\n");
        return(-1);
    }

    // Write /etc/resolv.conf
    ret = write_file(&file_data, outfile) ;
    if (ret < 0) {
        printf("Write failed\n");
        return(-1);
    }
    free((void *)file_data.data) ;

    // chown to root if needed and permitted
    if (perms.euid != 0 && perms.cap_chown == true) {
        ret = chown(outfile, 0, 0); 
        if (ret < 0) {
            printf("chown root failed\n");
            return(-1);
        }
    }

    return(0);
}
