
#include <errno.h>
#include <fcntl.h>
#include <skalibs/djbunix.h>

int
aa_copy_file (const char *src, const char *dst, mode_t mode, int overwrite)
{
    int fd_src;
    int fd_dst;

    fd_src = open_readb (src);
    if (fd_src < 0)
        return -1;

    fd_dst = open3 (dst, O_WRONLY | O_CREAT | ((overwrite) ? O_TRUNC : O_EXCL), mode);
    if (fd_dst < 0)
    {
        int e = errno;
        fd_close (fd_src);
        errno = e;
        return -1;
    }

    if (fd_cat (fd_src, fd_dst) < 0)
    {
        int e = errno;
        fd_close (fd_src);
        fd_close (fd_dst);
        errno = e;
        return -1;
    }

    fd_close (fd_src);
    fd_close (fd_dst);
    return 0;
}
