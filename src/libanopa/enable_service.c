/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * enable_service.c
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

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h> /* rename() */
#include <skalibs/bytestr.h>
#include <skalibs/djbunix.h>
#include <skalibs/direntry.h>
#include <skalibs/skamisc.h>
#include <skalibs/stralloc.h>
#include <anopa/enable_service.h>
#include <anopa/copy_file.h>
#include <anopa/scan_dir.h>
#include <anopa/err.h>
#include "service_internal.h"

static int
copy_from_source (const char        *name,
                  int                len,
                  aa_warn_fn         warn_fn,
                  aa_enable_flags    flags,
                  aa_auto_enable_cb  ae_cb);
static int
copy_dir (const char        *src,
          const char        *dst,
          mode_t             mode,
          int                depth,
          aa_warn_fn         warn_fn,
          aa_enable_flags    flags,
          aa_auto_enable_cb  ae_cb,
          const char        *instance);


static int
copy_log (const char *name, const char *cfg, mode_t mode, aa_warn_fn warn_fn)
{
    int fd;
    int r;
    int e;

    /* get current dir (repo) so we can come back */
    fd = open_read (".");
    if (fd < 0)
        return fd;

    /* and temporarily go into the servicedir */
    r = chdir (name);
    if (r < 0)
    {
        e = errno;
        fd_close (fd);
        errno = e;
        return r;
    }

    /* this is a logger, so there's no autoenable of any kind; hence we can use
     * 0 for flags (don't process it as a servicedir either, since it doesn't
     * apply) and not bother with a callback */
    r = copy_from_source ("log", 3, warn_fn, 0, NULL);

    if (r >= 0 && cfg)
        r = copy_dir (cfg, "log", mode, 0, warn_fn, 0, NULL, NULL);

    e = errno;
    fd_chdir (fd);
    fd_close (fd);
    errno = e;

    return r;
}

static int
clear_dir (const char *path, int excludes, aa_warn_fn warn_fn)
{
    DIR *dir;
    int salen = satmp.len;

    dir = opendir (path);
    if (!dir)
        return -ERR_IO;
    errno = 0;
    for (;;)
    {
        direntry *d;
        int r = 0;

        d = readdir (dir);
        if (!d)
            break;
        if (d->d_name[0] == '.'
                && (d->d_name[1] == '\0' || (d->d_name[1] == '.' && d->d_name[2] == '\0')))
            continue;

        if (stralloc_cats (&satmp, path) < 0)
            goto err;
        if (stralloc_catb (&satmp, "/", 1) < 0)
            goto err;
        if (stralloc_cats (&satmp, d->d_name) < 0)
            goto err;
        if (!stralloc_0 (&satmp))
            goto err;

        if (d->d_type == DT_UNKNOWN)
        {
            struct stat st;

            r = stat (satmp.s + salen, &st);
            if (r < 0)
                goto err;

            if (S_ISREG (st.st_mode))
                d->d_type = DT_REG;
            else if (S_ISDIR (st.st_mode))
                d->d_type = DT_DIR;
        }

        if (excludes)
        {
            if (d->d_type == DT_REG
                    && (str_equal (d->d_name, "status.anopa")
                        || str_equal (d->d_name, "down")))
                goto skip;
            else if (d->d_type == DT_DIR
                    && (str_equal (d->d_name, "supervise")
                        || str_equal (d->d_name, "event")))
                goto skip;
        }

        if (d->d_type == DT_DIR)
        {
            r = clear_dir (satmp.s + salen, 0, warn_fn);
            if (r == 0)
                r = rmdir (satmp.s + salen);
        }
        else
            r = unlink (satmp.s + salen);
err:
        if (r < 0)
            warn_fn (satmp.s + salen, errno);
skip:
        satmp.len = salen;
        if (r < 0)
            break;
    }
    if (errno)
    {
        int e = errno;
        dir_close (dir);
        errno = e;
        return -ERR_IO;
    }
    dir_close (dir);

    return 0;
}

static int
copy_dir (const char        *src,
          const char        *dst,
          mode_t             mode,
          int                depth,
          aa_warn_fn         warn_fn,
          aa_enable_flags    flags,
          aa_auto_enable_cb  ae_cb,
          const char        *instance)
{
    unsigned int l_satmp = satmp.len;
    unsigned int l_max = strlen (AA_SCANDIR_DIRNAME);
    DIR *dir;
    struct stat st;
    struct {
        unsigned int run   : 1;
        unsigned int down  : 1;
        unsigned int began : 1;
    } has = { .run = 0, .down = 0, .began = 0 };

    dir = opendir (src);
    if (!dir)
        return -ERR_IO;

    if (depth == 0 && (flags & (_AA_FLAG_IS_SERVICEDIR | AA_FLAG_SKIP_DOWN))
            == (_AA_FLAG_IS_SERVICEDIR | AA_FLAG_SKIP_DOWN))
        /* treat as if there'e one, so don't create it */
        has.down = 1;

    errno = 0;
    for (;;)
    {
        direntry *d;
        unsigned int len;

        d = readdir (dir);
        if (!d)
            break;
        if (d->d_name[0] == '.'
                && (d->d_name[1] == '\0' || (d->d_name[1] == '.' && d->d_name[2] == '\0')))
            continue;
        len = strlen (d->d_name);
        if (len > l_max)
            l_max = len;
        if (!stralloc_catb (&satmp, d->d_name, len + 1))
            break;
        if (depth == 0 && (flags & _AA_FLAG_IS_SERVICEDIR)
                /* if UPGRADE we don't need this, so skip those tests */
                && !(flags & AA_FLAG_UPGRADE_SERVICEDIR))
        {
            if (!has.run && str_equal (d->d_name, "run"))
                has.run = 1;
            if (!has.down && str_equal (d->d_name, "down"))
                has.down = 1;
        }
    }
    if (errno)
    {
        int e = errno;
        dir_close (dir);
        errno = e;
        goto err;
    }
    dir_close (dir);

    if ((flags & (_AA_FLAG_IS_SERVICEDIR | AA_FLAG_UPGRADE_SERVICEDIR))
            == (_AA_FLAG_IS_SERVICEDIR | AA_FLAG_UPGRADE_SERVICEDIR))
    {
        if (stat (dst, &st) < 0)
            goto err;
        else if (!S_ISDIR (st.st_mode))
        {
            errno = ENOTDIR;
            goto err;
        }
        else if (clear_dir (dst, 1, warn_fn) < 0)
            goto err;
    }
    else
    {
        if (mkdir (dst, S_IRWXU) < 0)
        {
            if (errno != EEXIST || stat (dst, &st) < 0)
                goto err;
            else if (!S_ISDIR (st.st_mode))
            {
                errno = ENOTDIR;
                goto err;
            }
            else if (flags & _AA_FLAG_IS_SERVICEDIR)
            {
                errno = EEXIST;
                goto err;
            }
        }
    }

    if (flags & _AA_FLAG_IS_SERVICEDIR)
    {
        has.began = 1;
        flags &= ~_AA_FLAG_IS_SERVICEDIR;
    }

    {
        unsigned int l_inst = (instance) ? strlen (instance) : 0;
        unsigned int l_src = strlen (src);
        unsigned int l_dst = strlen (dst);
        unsigned int i = l_satmp;
        char buf_src[l_src + 1 + l_max + 1];
        char buf_dst[l_dst + 1 + l_max + l_inst + 1];

        byte_copy (buf_src, l_src, src);
        buf_src[l_src] = '/';
        byte_copy (buf_dst, l_dst, dst);
        buf_dst[l_dst] = '/';

        while (i < satmp.len)
        {
            unsigned int len;
            int r;

            len = strlen (satmp.s + i);
            byte_copy (buf_src + l_src + 1, len + 1, satmp.s + i);
            byte_copy (buf_dst + l_dst + 1, len + 1, satmp.s + i);

            if (stat (buf_src, &st) < 0)
            {
                warn_fn (buf_src, errno);
                goto err;
            }

            if (S_ISREG (st.st_mode))
            {
                if (has.began && depth == 0 && str_equal (satmp.s + i, "log"))
                {
                    r = copy_log (dst, NULL, 0, warn_fn);
                    st.st_mode = 0755;
                }
                else if ((flags & _AA_FLAG_IS_CONFIGDIR) && len > 1
                        && (satmp.s[i] == '-' || satmp.s[i] == '+'))
                {
                    byte_copy (buf_dst + l_dst + 1, len, satmp.s + i + 1);
                    /* for any file in one of the 4 special places that ends
                     * with a '@' we append our instance name
                     * (don't make much sense for '+' but useful for '-' to
                     * remove "generic" ordering/dependencies) */
                    if (depth == 1 && instance && (flags & _AA_FLAG_IS_1OF4)
                            && satmp.s[i + len - 1] == '@')
                        byte_copy (buf_dst + l_dst + len, l_inst + 1, instance);

                    if (satmp.s[i] == '-')
                    {
                        r = unlink (buf_dst);
                        if (r < 0 && errno == ENOENT)
                            /* not an error */
                            r = 0;
                        /* skip lchown/chmod calls */
                        goto next;
                    }
                    else /* '+' */
                        r = aa_copy_file (buf_src, buf_dst, st.st_mode, AA_CP_APPEND);
                }
                else
                {
                    /* for any file in one of the 4 special places that ends
                     * with a '@' we append our instance name */
                    if (depth == 1 && instance && (flags & _AA_FLAG_IS_1OF4)
                            && satmp.s[i + len - 1] == '@')
                        byte_copy (buf_dst + l_dst + 1 + len, l_inst + 1, instance);
                    r = aa_copy_file (buf_src, buf_dst, st.st_mode, AA_CP_OVERWRITE);
                }
            }
            else if (S_ISDIR (st.st_mode))
            {
                if (has.began && depth == 0 && str_equal (satmp.s + i, "log"))
                    r = copy_log (dst, buf_src, st.st_mode, warn_fn);
                else
                {
                    /* use depth because this is also enabled for the config part */
                    if (depth == 0)
                    {
                        /* flag to enable auto-rename of files above */
                        if (str_equal (satmp.s + i, "needs")
                                || str_equal (satmp.s + i, "wants")
                                || str_equal (satmp.s + i, "before")
                                || str_equal (satmp.s + i, "after"))
                            flags |= _AA_FLAG_IS_1OF4;
                    }
                    r = copy_dir (buf_src, buf_dst, st.st_mode, depth + 1,
                            warn_fn, flags, ae_cb, instance);
                    if (depth == 0)
                        flags &= ~_AA_FLAG_IS_1OF4;
                }
            }
            else if (S_ISFIFO (st.st_mode))
                r = mkfifo (buf_dst, st.st_mode);
            else if (S_ISLNK (st.st_mode))
            {
                unsigned int l_tmp = satmp.len;

                if ((sareadlink (&satmp, buf_src) < 0) || !stralloc_0 (&satmp))
                    r = -1;
                else
                    r = symlink (satmp.s + l_tmp, buf_dst);

                satmp.len = l_tmp;
            }
            else if (S_ISCHR (st.st_mode) || S_ISBLK (st.st_mode) || S_ISSOCK (st.st_mode))
                r = mknod (buf_dst, st.st_mode, st.st_rdev);
            else
            {
                errno = EOPNOTSUPP;
                r = -1;
            }

            if (r >= 0)
                r = lchown (buf_dst, st.st_uid, st.st_gid);
            if (r >= 0 && !S_ISLNK (st.st_mode) && !S_ISDIR (st.st_mode))
                r = chmod (buf_dst, st.st_mode);

next:
            if (r < 0)
            {
                warn_fn (buf_dst, errno);
                goto err;
            }

            i += len + 1;
        }

        if (has.run && !(flags & AA_FLAG_UPGRADE_SERVICEDIR))
        {
            if (!has.down)
            {
                char buf[l_dst + 1 + strlen ("down") + 1];
                int fd;

                byte_copy (buf, l_dst, dst);
                buf[l_dst] = '/';
                byte_copy (buf + l_dst + 1, 5, "down");

                fd = open_create (buf);
                if (fd < 0)
                {
                    warn_fn (buf, errno);
                    goto err;
                }
                else
                    fd_close (fd);
            }

            {
                char buf_lnk[3 + l_dst + 1];
                char buf_dst[sizeof (AA_SCANDIR_DIRNAME) + l_dst + 1];

                byte_copy (buf_lnk, 3, "../");
                byte_copy (buf_lnk + 3, l_dst + 1, dst);

                byte_copy (buf_dst, sizeof (AA_SCANDIR_DIRNAME), AA_SCANDIR_DIRNAME "/");
                byte_copy (buf_dst + sizeof (AA_SCANDIR_DIRNAME), l_dst + 1, dst);

                if (symlink (buf_lnk, buf_dst) < 0)
                {
                    warn_fn (buf_dst, errno);
                    goto err;
                }
            }
        }
    }

    if (chmod (dst, mode) < 0)
    {
        if (has.began)
            warn_fn (dst, errno);
        goto err;
    }

    satmp.len = l_satmp;
    return 0;

err:
    satmp.len = l_satmp;
    if (!has.began)
        return -ERR_IO;
    else
    {
        unsigned int l_dst = strlen (dst);
        char buf[1 + l_dst + 1];

        *buf = '@';
        byte_copy (buf + 1, l_dst + 1, dst);

        /* rename dst servicedir by prefixing with a '@' so that aa-start would
         * fail to find/start the service, and make it easilly noticable on the
         * file system, since it's in an undetermined/invalid state */
        if (rename (dst, buf) < 0)
            warn_fn (dst, errno);

        return -ERR_FAILED_ENABLE;
    }
}

static int
copy_from_source (const char        *name,
                  int                len,
                  aa_warn_fn         warn_fn,
                  aa_enable_flags    flags,
                  aa_auto_enable_cb  ae_cb)
{
    int i;

    if (aa_sa_sources.len == 0)
        return -ERR_UNKNOWN;

    i = 0;
    for (;;)
    {
        int l_sce = strlen (aa_sa_sources.s + i);
        char buf[l_sce + 1 + len + 1];
        struct stat st;

        byte_copy (buf, l_sce, aa_sa_sources.s + i);
        buf[l_sce] = '/';
        byte_copy (buf + l_sce + 1, len, name);
        buf[l_sce + 1 + len] = '\0';

        if (stat (buf, &st) < 0)
        {
            if (errno != ENOENT)
                warn_fn (buf, errno);
        }
        else if (!S_ISDIR (st.st_mode))
            warn_fn (buf, ENOTDIR);
        else
        {
            int r;

            r = copy_dir (buf, name, st.st_mode, 0, warn_fn, flags, ae_cb,
                    (name[len - 1] == '@') ? name + len : NULL);
            if (r < 0)
                return r;
            break;
        }

        i += l_sce + 1;
        if (i > aa_sa_sources.len)
            return -ERR_UNKNOWN;
    }

    return 0;
}

static int
it_cb (direntry *d, void *_data)
{
    struct {
        aa_auto_enable_cb cb;
        unsigned int flag;
    } *data = _data;

    data->cb (d->d_name, data->flag);
    return 0;
}

int
aa_enable_service (const char       *_name,
                   aa_warn_fn        warn_fn,
                   aa_enable_flags   flags,
                   aa_auto_enable_cb ae_cb)
{
    const char *name = _name;
    const char *instance = NULL;
    mode_t _mode = 0; /* silence warning */
    int l_name = strlen (name);
    int len;
    int r;

    /* if name is a /path/to/file we get the actual/service name */
    if (*name == '/')
    {
        int r;

        if (l_name == 1)
            return -ERR_INVALID_NAME;
        r = byte_rchr (name, l_name, '/') + 1;
        name += r;
        l_name -= r;
    }

    if (!_is_valid_service_name (name, l_name))
        return -ERR_INVALID_NAME;

    if (*_name == '/')
    {
        struct stat st;

        if (stat (_name, &st) < 0)
            return ERR_IO;
        else if (S_ISREG (st.st_mode))
            /* file; so nothing special to do, we can "drop" the path */
            _name = name;
        else if (!S_ISDIR (st.st_mode))
            return (errno = EINVAL, -ERR_IO);
        else
            _mode = st.st_mode;
    }

    /* len is l_name unless there's a '@', then we want up to (inc.) the '@' */
    len = byte_chr (name, l_name, '@');
    if (len < l_name)
    {
        ++len;
        instance = name + len;
    }

    r = copy_from_source (name, len, warn_fn, flags | _AA_FLAG_IS_SERVICEDIR, ae_cb);
    if (r < 0)
        return r;

    if (name != _name)
    {
        r = copy_dir (_name, name, _mode, 0, warn_fn, flags | _AA_FLAG_IS_CONFIGDIR, ae_cb, instance);
        if (r < 0)
            return r;
    }

    {
        int l = sizeof ("/log/run-args");
        char buf[l_name + l];
        struct stat st;

        byte_copy (buf, l_name, name);
        byte_copy (buf + l_name, l, "/log/run-args");

        r = stat (buf, &st);
        if (r == 0 && S_ISREG (st.st_mode))
        {
            char dst[l_name + l - 5];

            byte_copy (dst, l_name, name);
            byte_copy (dst + l_name, l - 5, "/log/run");

            r = aa_copy_file (buf, dst, st.st_mode, AA_CP_APPEND);
            if (r == 0)
                unlink (buf);
        }
        else if (r < 0 && errno == ENOENT)
            r = 0;
    }

    if (ae_cb && flags & (AA_FLAG_AUTO_ENABLE_NEEDS | AA_FLAG_AUTO_ENABLE_WANTS))
    {
        stralloc sa = STRALLOC_ZERO;
        struct {
            aa_auto_enable_cb cb;
            unsigned int flag;
        } data = { .cb = ae_cb };

        if (!stralloc_catb (&sa, name, l_name))
        {
            errno = ENOMEM;
            return -1;
        }

        if (flags & AA_FLAG_AUTO_ENABLE_NEEDS)
        {
            if (!stralloc_cats (&sa, "/needs") || !stralloc_0 (&sa))
            {
                stralloc_free (&sa);
                errno = ENOMEM;
                return -1;
            }
            data.flag = AA_FLAG_AUTO_ENABLE_NEEDS;
            r = aa_scan_dir (&sa, 1, it_cb, &data);
            if (r == -ERR_IO && errno == ENOENT)
                r = 0;
            sa.len = l_name;
        }

        if (r == 0 && flags & AA_FLAG_AUTO_ENABLE_WANTS)
        {
            if (!stralloc_cats (&sa, "/wants") || !stralloc_0 (&sa))
            {
                stralloc_free (&sa);
                errno = ENOMEM;
                return -1;
            }
            data.flag = AA_FLAG_AUTO_ENABLE_WANTS;
            r = aa_scan_dir (&sa, 1, it_cb, &data);
            if (r == -ERR_IO && errno == ENOENT)
                r = 0;
        }

        stralloc_free (&sa);
    }

    return r;
}
