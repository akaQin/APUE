#include "../apue.3e/include/apue.h"
#include <dirent.h>
#include <limits.h>

typedef int Myfunc(const char *, struct stat *, int);
static Myfunc myfunc;
static int myftw(char *, Myfunc *);
static int dopath(Myfunc *);

static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;

int main(int argc, char **argv) {
    if (argc != 2) {
        err_quit("usage:  ftw  <starting-pathname>");
    }
    int ret = myftw(argv[1], myfunc);

    ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
    if (ntot == 0) {
        ntot = 1;
    }

    printf("regular files  = %7ld, %5.2f %%\n", nreg, nreg * 100.0 / ntot);
    printf("directories    = %7ld, %5.2f %%\n", ndir, ndir * 100.0 / ntot);
    printf("block special  = %7ld, %5.2f %%\n", nblk, nblk * 100.0 / ntot);
    printf("char special   = %7ld, %5.2f %%\n", nchr, nchr * 100.0 / ntot);
    printf("FIFOs          = %7ld, %5.2f %%\n", nfifo, nfifo * 100.0 / ntot);
    printf("symbolic links = %7ld, %5.2f %%\n", nslink, nslink * 100.0 / ntot);
    printf("sockets        = %7ld, %5.2f %%\n", nsock, nsock * 100.0 / ntot);

    exit(ret);
}

static char *filename;
static size_t pathlen;

static int myftw(char *pathname, Myfunc *myfunc)
{
    filename = path_alloc(&pathlen);
    if (pathlen < strlen(pathname)) {
        pathlen = strlen(pathname) * 2;
        if ((filename = realloc(filename, pathlen)) == NULL) {
            err_sys("realloc failed");
        }
    }
    strcpy(filename, pathname);
    if (chdir(filename) < 0) {
        err_sys("chdir %s failed", filename);
    }
    return dopath(myfunc);
}

#define	FTW_F	1		/* file other than directory */
#define	FTW_D	2		/* directory */
#define	FTW_DNR	3		/* directory that can't be read */
#define	FTW_NS	4		/* file that we can't stat */

static int dopath(Myfunc *func)
{
    struct stat statbuf;
    if (lstat(filename, &statbuf) < 0) {
        return func(filename, &statbuf, FTW_NS);
    }
    if (S_ISDIR(statbuf.st_mode) == 0) {
        return func(filename, &statbuf, FTW_F);
    }

    int ret, n;
    if ((ret = func(filename, &statbuf, FTW_D)) != 0) {
        printf("%s\n", filename);
        return ret;
    }

    n = strlen(filename);
    if (n + NAME_MAX + 2 > pathlen) {
        pathlen *= 2;
        if ((filename = realloc(filename, pathlen)) == NULL) {
            err_sys("realloc failed");
        }
    }

    DIR *dirp;
    if ((dirp = opendir(filename)) == NULL) {
        return func(filename, &statbuf, FTW_DNR);
    }
    if (chdir(filename) < 0) {
        err_sys("chdir %s error", filename);
    }

    struct dirent *direntp;
    while ((direntp = readdir(dirp)) != NULL) {
        if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
            continue;
        }
        filename = direntp->d_name;
        if ((ret = dopath(func)) != 0) {
            break;
        }
    }

    if (chdir("..") < 0) {
        err_sys("chdir %s error", filename);
    }
    if (closedir(dirp) < 0) {
        err_ret("can't close directory %s", filename);
    }

    return ret;
}

static int myfunc(const char *pathname, struct stat *statptr, int type)
{
    switch (type) {
        case FTW_F:
            switch (statptr->st_mode & S_IFMT) {
                case S_IFREG: nreg++;       break;
                case S_IFBLK: nblk++;       break;
                case S_IFCHR: nchr++;       break;
                case S_IFIFO: nfifo++;      break;
                case S_IFLNK: nslink++;     break;
                case S_IFSOCK: nsock++;     break;
                case S_IFDIR:
                    err_dump("for S_IFDIR for %s", pathname);
            }
            break;
        case FTW_D:
            ndir++;
            break;
        case FTW_DNR:
            err_ret("can't read directory %s", pathname);
            break;
        case FTW_NS:
            err_ret("stat error for %s", pathname);
            break;
        default:
            err_dump("unknown type %d for pathname %s", type, pathname);
    }
    return 0;
}