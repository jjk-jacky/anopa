/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-mount.c
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

#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <skalibs/stralloc.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>
#include <anopa/output.h>
#include "mount-constants.h"

struct mnt_opt
{
    const char *name;
    size_t len;
    unsigned long value;
    enum {
        OP_ADD,
        OP_REM,
        OP_SET
    } op;
};

/* special: we don't own sa */
static int
add_option (stralloc *sa, unsigned long *flags, char const *options)
{
#define mnt_opt(name, op, val)  { name, strlen (name), val, OP_##op }
    struct mnt_opt mnt_options[] = {
        mnt_opt ("defaults",        SET, MS_MGC_VAL),
        mnt_opt ("ro",              ADD, MS_RDONLY),
        mnt_opt ("rw",              REM, MS_RDONLY),
        mnt_opt ("bind",            ADD, MS_BIND),
        mnt_opt ("move",            ADD, MS_MOVE),
        mnt_opt ("async",           REM, MS_SYNCHRONOUS),
        mnt_opt ("atime",           REM, MS_NOATIME),
        mnt_opt ("noatime",         ADD, MS_NOATIME),
        mnt_opt ("dev",             REM, MS_NODEV),
        mnt_opt ("nodev",           ADD, MS_NODEV),
        mnt_opt ("diratime",        REM, MS_NODIRATIME),
        mnt_opt ("nodiratime",      ADD, MS_NODIRATIME),
        mnt_opt ("dirsync",         ADD, MS_DIRSYNC),
        mnt_opt ("exec",            REM, MS_NOEXEC),
        mnt_opt ("noexec",          ADD, MS_NOEXEC),
        mnt_opt ("mand",            ADD, MS_MANDLOCK),
        mnt_opt ("nomand",          REM, MS_MANDLOCK),
        mnt_opt ("relatime",        ADD, MS_RELATIME),
        mnt_opt ("norelatime",      REM, MS_RELATIME),
        mnt_opt ("strictatime",     ADD, MS_STRICTATIME),
        mnt_opt ("nostrictatime",   REM, MS_STRICTATIME),
        mnt_opt ("suid",            REM, MS_NOSUID),
        mnt_opt ("nosuid",          ADD, MS_NOSUID),
        mnt_opt ("remount",         ADD, MS_REMOUNT),
        mnt_opt ("sync",            ADD, MS_SYNCHRONOUS)
    };
#undef mnt_opt
    size_t nb = sizeof (mnt_options) / sizeof (*mnt_options);

    for (;;)
    {
        size_t e;
        size_t i;

        e = str_chr (options, ',');
        for (i = 0; i < nb; ++i)
        {
            if (e == mnt_options[i].len
                    && !str_diffn (options, mnt_options[i].name, e))
            {
                switch (mnt_options[i].op)
                {
                    case OP_ADD:
                        *flags |= mnt_options[i].value;
                        break;

                    case OP_REM:
                        *flags &= ~mnt_options[i].value;
                        break;

                    case OP_SET:
                        *flags = mnt_options[i].value;
                        break;
                }
                break;
            }
        }
        if (i >= nb)
        {
            /* add user option as-is */
            if ((sa->len > 0 && !stralloc_catb (sa, ",", 1))
                    || !stralloc_catb (sa, options, e))
                return 0;
        }

        options += e;
        if (*options == '\0')
            return 1;
        ++options;
    }
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] DEVICE MOUNTPOINT",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -B, --bind                    Remount a subtree somewhere else\n"
            " -M, --move                    Move a subtree to some other place\n"
            " -t, --type FSTYPE             Use FSTYPE as type of filesystem\n"
            " -o, --options OPTIONS         Use OPTIONS as mount options\n"
            " -r, --read-only               Mount read-only\n"
            " -w, --read-write              Mount read-write\n"
            " -d, --mkdir                   Create MOUNTPOINT before mounting\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-mount";
    stralloc sa = STRALLOC_ZERO;
    unsigned long flags = MS_MGC_VAL;
    const char *fstype = NULL;
    int mk = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "bind",               no_argument,        NULL,   'B' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "mkdir",              no_argument,        NULL,   'd' },
            { "help",               no_argument,        NULL,   'h' },
            { "move",               no_argument,        NULL,   'M' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "options",            required_argument,  NULL,   'o' },
            { "read-only",          no_argument,        NULL,   'r' },
            { "type",               required_argument,  NULL,   't' },
            { "version",            no_argument,        NULL,   'V' },
            { "read-write",         no_argument,        NULL,   'w' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "BDdhMO:o:rt:Vw", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'B':
                if (!add_option (&sa, &flags, "bind"))
                    aa_strerr_diefu1sys (RC_FATAL_MEMORY, "build user options");
                break;

            case 'D':
                aa_set_double_output (1);
                break;

            case 'd':
                mk = 1;
                break;

            case 'h':
                dieusage (RC_OK);

            case 'M':
                if (!add_option (&sa, &flags, "move"))
                    aa_strerr_diefu1sys (RC_FATAL_MEMORY, "build user options");
                break;

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'o':
                if (!add_option (&sa, &flags, optarg))
                    aa_strerr_diefu1sys (RC_FATAL_MEMORY, "build user options");
                break;

            case 'r':
                if (!add_option (&sa, &flags, "ro"))
                    aa_strerr_diefu1sys (RC_FATAL_MEMORY, "build user options");
                break;

            case 't':
                fstype = optarg;
                break;

            case 'V':
                aa_die_version ();

            case 'w':
                if (!add_option (&sa, &flags, "rw"))
                    aa_strerr_diefu1sys (RC_FATAL_MEMORY, "build user options");
                break;

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 2)
        dieusage (RC_FATAL_USAGE);

    if (!stralloc_0 (&sa))
        aa_strerr_diefu1sys (RC_FATAL_MEMORY, "build user options");
    if (mk && mkdir (argv[1], 0755) < 0 && errno != EEXIST)
        aa_strerr_diefu4sys (RC_FATAL_IO, "mkdir ", argv[1], " to mount ", argv[0]);
    if (mount (argv[0], argv[1], fstype, flags, sa.s) < 0)
        aa_strerr_diefu4sys (RC_FATAL_IO, "mount ", argv[0], " on ", argv[1]);

    return RC_OK;
}
