/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-terminate.c
 * Copyright (C) 2015-2018 Olivier Brunel <jjk@jjacky.com>
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

#include <getopt.h>
#include <stdio.h>
#include <mntent.h>
#include <strings.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/swap.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/dm-ioctl.h>
#include <linux/loop.h>
#include <skalibs/djbunix.h>
#include <skalibs/stralloc.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>
#include <anopa/scan_dir.h>
#include <anopa/output.h>

typedef int (*do_fn) (const char *path);
static int umnt_flags = 0;
static int level = 1;

static void
verbose_do (const char *s1, const char *s2)
{
    if (level < 2)
        return;
    aa_bs (AA_OUT, s1);
    aa_bs (AA_OUT, s2);
    aa_bs_flush (AA_OUT, "...\n");
}

static void
verbose_fail (int e)
{
    if (level < 2)
        return;
    aa_bs (AA_OUT, "Failed: ");
    aa_bs (AA_OUT, strerror (e));
    aa_bs_flush (AA_OUT, "\n");
}

static int
do_swapoff (const char *path)
{
    int r;

    verbose_do ("Turning off swap on ", path);
    r = swapoff (path);
    if (r < 0)
        verbose_fail (errno);
    else if (level > 0)
        aa_put_title (0, "Swap turned off", path, 1);
    return r;
}

static int
do_umount (const char *path)
{
    int r;

    verbose_do ("Unmounting ", path);
    r = umount2 (path, umnt_flags);
    if (r < 0)
        verbose_fail (errno);
    else if (level > 0)
        aa_put_title (0, "Unmounted", path, 1);
    return r;
}

static int
do_loop_close (const char *path)
{
    int fd;
    int r;

    verbose_do ("Closing loop device ", path);
    fd = open_read (path);
    if (fd < 0)
    {
        verbose_fail (errno);
        return -1;
    }

    r = ioctl (fd, LOOP_CLR_FD, 0);
    if (r < 0)
        verbose_fail (errno);
    else if (level > 0)
        aa_put_title (0, "Loop device closed", path, 1);

    fd_close (fd);
    return r;
}

static int
do_dm_close (const char *path)
{
    struct dm_ioctl dm = {
        .version = { DM_VERSION_MAJOR, DM_VERSION_MINOR, DM_VERSION_PATCHLEVEL },
        .data_size = sizeof (dm)
    };
    struct stat st;
    int fd;
    int r;

    verbose_do ("Removing block device ", path);
    if (stat (path, &st) < 0)
    {
        verbose_fail (errno);
        return -1;
    }
    dm.dev = st.st_rdev;

    fd = open_write ("/dev/mapper/control");
    if (fd < 0)
    {
        verbose_fail (errno);
        return -1;
    }

    r = ioctl (fd, DM_DEV_REMOVE, &dm);
    if (r < 0)
        verbose_fail (errno);
    else if (level > 0)
        aa_put_title (0, "Block device removed", path, 1);

    fd_close (fd);
    return r;
}

static int
do_work (stralloc *sa, do_fn do_it)
{
    int did = 0;
    size_t i;

    for (i = 0; i < sa->len; )
    {
        size_t l;

        l = strlen (sa->s + i) + 1;
        if (do_it (sa->s + i) < 0)
            i += l;
        else
        {
            ++did;
            if (i + l < sa->len)
                memmove (sa->s + i, sa->s + i + l, sa->len - i - l);
            sa->len -= l;
        }
    }

    return did;
}

static int
it_loops_dms (direntry *d, void *data)
{
    stralloc **sas = data;
    int i;
    size_t l;

    if (d->d_type != DT_BLK)
        return 0;

    if (!str_diffn (d->d_name, "loop", 4) && d->d_name[4] >= '0' && d->d_name[4] <= '9')
        i = 0;
    else if (!str_diffn (d->d_name, "dm-", 3))
        i = 1;
    else
        return 0;

    l = sas[i]->len;
    if (!stralloc_cats (sas[i], "/dev/") || !stralloc_cats (sas[i], d->d_name)
            || !stralloc_0 (sas[i]))
        aa_strerr_diefu1sys (2, "stralloc_catb");

    /* /dev/loop* always exists, let's find the actual ones */
    if (i == 0)
    {
        int fd;

        fd = open_read (sas[i]->s + l);
        if (fd < 0)
            sas[i]->len = l;
        else
        {
            struct loop_info64 info;
            int r;

            /* make sure it is one/in use; we get ENXIO when not */
            r = ioctl (fd, LOOP_GET_STATUS64, &info);
            if (r < 0)
                /* treat all errors the same though */
                sas[i]->len = l;

            fd_close (fd);
        }
    }

    return 0;
}

static void
show_left (const char *prefix, stralloc *sa)
{
    size_t i;

    for (i = 0; i < sa->len; )
    {
        size_t l;

        l = strlen (sa->s + i) + 1;
        aa_put_warn (prefix, sa->s +i, 1);
        i += l;
    }
}

static void
umount_api (const char *path)
{
    verbose_do ("Unmounting ", path);
    if (umount2 (path, 0) < 0 && umount2 (path, MNT_DETACH) < 0)
        verbose_fail (errno);
    else if (level > 0)
        aa_put_title (0, "Unmounted", path, 1);
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...]",
            " -D, --double-output           Enable double-output mode\n"
            " -v, --verbose                 Show what was done\n"
            " -q, --quiet                   No warnings for what's left\n"
            " -l, --lazy-umounts            Try lazy umount as last resort\n"
            " -a, --apis                    Umount /run, /sys, /proc & /dev too\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-terminate";
    stralloc sa = STRALLOC_ZERO;
    stralloc sa_swaps = STRALLOC_ZERO;
    stralloc sa_mounts = STRALLOC_ZERO;
    stralloc sa_loops = STRALLOC_ZERO;
    stralloc sa_dms = STRALLOC_ZERO;
    int apis = 0;
    int lazy = 0;
    int did_smthg;

    for (;;)
    {
        struct option longopts[] = {
            { "apis",               no_argument,        NULL,   'a' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "lazy-umounts",       no_argument,        NULL,   'l' },
            { "quiet",              no_argument,        NULL,   'q' },
            { "version",            no_argument,        NULL,   'V' },
            { "verbose",            no_argument,        NULL,   'v' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "aDhlqVv", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'a':
                apis = 1;
                break;

            case 'D':
                aa_set_double_output (1);
                break;

            case 'h':
                dieusage (0);

            case 'l':
                lazy = 1;
                break;

            case 'q':
                level = 0;
                break;

            case 'V':
                aa_die_version ();

            case 'v':
                level = 2;
                break;

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc > 0)
        dieusage (1);

again:
    for (;;)
    {
        /* re-init */
        sa_swaps.len = 0;
        sa_mounts.len = 0;
        sa_loops.len = 0;
        sa_dms.len = 0;

        /* read swaps */
        if (!openslurpclose (&sa, "/proc/swaps"))
            aa_strerr_diefu1sys (2, "read /proc/swaps");

        if (sa.len > 0)
        {
            size_t l;

            l = byte_chr (sa.s, sa.len, '\n') + 1;
            for ( ; l < sa.len; )
            {
                size_t e;

                /* FIXME: how are spaces-in-filename treated? */
                e = byte_chr (sa.s + l, sa.len - l, ' ');
                if (e < sa.len - l)
                {
                    if (!stralloc_catb (&sa_swaps, sa.s + l, e)
                            || !stralloc_0 (&sa_swaps))
                        aa_strerr_diefu1sys (2, "stralloc_catb");
                }
                l += byte_chr (sa.s + l, sa.len - l, '\n') + 1;
            }
            sa.len = 0;
        }


        /* read mounts */
        {
            FILE *mounts;
            struct mntent *mnt;

            mounts = setmntent ("/proc/mounts", "r");
            if (!mounts)
                aa_strerr_diefu1sys (2, "read /proc/mounts");

            while ((mnt = getmntent (mounts)))
            {
                if (str_equal (mnt->mnt_dir, "/")
                        || str_equal (mnt->mnt_dir, "/dev")
                        || str_equal (mnt->mnt_dir, "/proc")
                        || str_equal (mnt->mnt_dir, "/sys")
                        || str_equal (mnt->mnt_dir, "/run"))
                    continue;

                if (!stralloc_catb (&sa_mounts, mnt->mnt_dir, strlen (mnt->mnt_dir) + 1))
                    aa_strerr_diefu1sys (2, "stralloc_catb");
            }
            endmntent (mounts);
        }


        /* read loops + dms */
        {
            stralloc *sas[2] = { &sa_loops, &sa_dms };
            int r;

            stralloc_catb (&sa, "/dev", 5);
            r = aa_scan_dir (&sa, 2, it_loops_dms, &sas);
            if (r < 0)
                aa_strerr_diefu1sys (2, "scan /dev");
            sa.len = 0;
        }

        did_smthg = 0;

        if (do_work (&sa_swaps, do_swapoff))
            did_smthg = 1;
        if (do_work (&sa_mounts, do_umount))
            did_smthg = 1;
        if (do_work (&sa_loops, do_loop_close))
            did_smthg = 1;
        if (do_work (&sa_dms, do_dm_close))
            did_smthg = 1;

        if (!did_smthg)
            break;
    }

    if (lazy && umnt_flags == 0 && sa_mounts.len > 0)
    {
        verbose_do ("Switching to lazy umount mode", "");
        umnt_flags = MNT_DETACH;
        goto again;
    }

    if (apis)
    {
        umount_api ("/run");
        umount_api ("/sys");
        umount_api ("/proc");
        umount_api ("/dev");
    }

    if (level > 0)
    {
        show_left ("Remaining swap", &sa_swaps);
        show_left ("Remaining mountpoint", &sa_mounts);
        show_left ("Remaining loop device", &sa_loops);
        show_left ("Remaining block device", &sa_dms);
    }

    return (sa_swaps.len + sa_mounts.len + sa_loops.len + sa_dms.len == 0) ? 0 : 2;
}
