
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <anopa/err.h>
#include <anopa/common.h>
#include <anopa/init_repo.h>

int
aa_init_repo (const char *path_repo, aa_repo_init ri)
{
    int amode;

    umask (0);

    if (ri == AA_REPO_CREATE && mkdir (path_repo,
                S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
    {
        struct stat st;

        if (errno != EEXIST)
            return -ERR_IO_REPODIR;
        if (stat (path_repo, &st) < 0)
            return -ERR_IO_REPODIR;
        if (!S_ISDIR (st.st_mode))
        {
            errno = ENOTDIR;
            return -ERR_IO_REPODIR;
        }
    }
    if (chdir (path_repo) < 0)
        return -ERR_IO;

    if (ri == AA_REPO_CREATE && mkdir (AA_SCANDIR_DIRNAME, S_IRWXU) < 0)
    {
        struct stat st;

        if (errno != EEXIST)
            return -ERR_IO_SCANDIR;
        if (stat (AA_SCANDIR_DIRNAME, &st) < 0)
            return -ERR_IO_SCANDIR;
        if (!S_ISDIR (st.st_mode))
        {
            errno = ENOTDIR;
            return -ERR_IO_SCANDIR;
        }
    }

    amode = R_OK;
    if (ri != AA_REPO_READ)
        amode |= W_OK;

    if (access (".", amode) < 0)
        return -ERR_IO;

    return 0;
}
