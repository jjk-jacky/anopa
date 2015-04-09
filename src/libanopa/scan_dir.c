/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * scan_dir.c
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

#define _BSD_SOURCE

#include <sys/stat.h>
#include <errno.h>
#include <skalibs/direntry.h>
#include <skalibs/stralloc.h>
#include <anopa/err.h>
#include <anopa/scan_dir.h>


/* breaking the rule here: we get a stralloc* but we don't own it, it's just so
 * we can use it if needed to stat() */
int
aa_scan_dir (stralloc *sa, int files_only, aa_sd_it_fn iterator, void *data)
{
    DIR *dir;
    int e = 0;
    int r = 0;

    dir = opendir (sa->s);
    if (!dir)
        return -ERR_IO;

    for (;;)
    {
        direntry *d;

        errno = 0;
        d = readdir (dir);
        if (!d)
        {
            e = errno;
            break;
        }
        if (d->d_name[0] == '.'
                && (d->d_name[1] == '\0' || (d->d_name[1] == '.' && d->d_name[2] == '\0')))
            continue;
        if (d->d_type == DT_UNKNOWN)
        {
            struct stat st;
            int l;
            int rr;

            l = sa->len;
            sa->s[l - 1] = '/';
            stralloc_catb (sa, d->d_name, str_len (d->d_name) + 1);
            rr = stat (sa->s, &st);
            sa->len = l;
            sa->s[l - 1] = '\0';
            if (rr != 0)
                continue;
            if (S_ISREG (st.st_mode))
                d->d_type = DT_REG;
            else if (S_ISDIR (st.st_mode))
                d->d_type = DT_DIR;
            else if (S_ISBLK (st.st_mode))
                d->d_type = DT_BLK;
        }
        if (d->d_type != DT_REG && (
                    files_only == 1
                    || (files_only == 0 && d->d_type != DT_DIR)
                    || (files_only == 2 && d->d_type != DT_BLK)
                    ))
            continue;

        r = iterator (d, data);
        if (r < 0)
            break;
    }
    dir_close (dir);

    if (e > 0)
    {
        r = -ERR_IO;
        errno = e;
    }
    return r;
}
