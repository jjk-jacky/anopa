/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * init_repo.c
 * Copyright (C) 2015 Olivier Brunel <jjk@jjacky.com>
 *
 * This file is part of anopa.
 *
 * anopa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * anopa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * anopa. If not, see http://www.gnu.org/licenses/
 */

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
