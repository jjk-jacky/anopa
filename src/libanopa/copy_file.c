
#include <errno.h>
#include <fcntl.h>
#include <skalibs/djbunix.h>
#include <anopa/copy_file.h>

int
aa_copy_file (const char *src, const char *dst, mode_t mode, aa_cp cp)
{
    int fd_src;
    int fd_dst;
    int flag[_AA_CP_NB];

    fd_src = open_readb (src);
    if (fd_src < 0)
        return -1;

    flag[AA_CP_CREATE] = O_EXCL;
    flag[AA_CP_OVERWRITE] = O_TRUNC;
    flag[AA_CP_APPEND] = O_APPEND;

    fd_dst = open3 (dst, O_WRONLY | O_CREAT | flag[cp], mode);
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
