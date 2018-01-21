/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-test.c
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
#include <unistd.h>
#include <sys/stat.h>
#include <skalibs/types.h>
#include <anopa/common.h>
#include <anopa/output.h>

enum
{
    RC_ST_EXIST     = 1 << 1,
    RC_ST_FAIL      = 2 << 1
};

static int
is_group_member (gid_t gid)
{
    int nb = getgroups (0, NULL);
    gid_t groups[nb];
    int i;

    if (gid == getgid () || gid == getegid())
        return 1;

    if (getgroups (nb, groups) < 0)
        return 0;

    for (i = 0; i < nb; ++i)
        if (gid == groups[i])
            return 1;

    return 0;
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "OPTION FILE",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -b, --block                   Test whether FILE is a block special\n"
            " -d, --directory               Test whether FILE is a directory\n"
            " -e, --exists                  Test whether FILE exists\n"
            " -f, --file                    Test whether FILE is a regular file\n"
            " -L, --symlink                 Test whether FILE is a symbolic link\n"
            " -p, --pipe                    Test whether FILE is a named pipe\n"
            " -S, --socket                  Test whether FILE is a socket\n"
            " -r, --read                    Test for read permission on FILE\n"
            " -w, --write                   Test for write permission on FILE\n"
            " -x, --execute                 Test for execute permission on FILE\n"
            " -R, --repeat[=TIMES]          Repeat test every second up to TIMES times\n"
            "\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-test";
    struct stat st;
    uid_t euid;
    int mode = 0; /* silence warning */
    char test = 0;
    unsigned int repeat = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "block",              no_argument,        NULL,   'b' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "directory",          no_argument,        NULL,   'd' },
            { "exists",             no_argument,        NULL,   'e' },
            { "file",               no_argument,        NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "symlink",            no_argument,        NULL,   'L' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "pipe",               no_argument,        NULL,   'p' },
            { "read",               no_argument,        NULL,   'r' },
            { "repeat",             optional_argument,  NULL,   'R' },
            { "socket",             no_argument,        NULL,   'S' },
            { "version",            no_argument,        NULL,   'V' },
            { "write",              no_argument,        NULL,   'w' },
            { "execute",            no_argument,        NULL,   'x' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "bDdefhLO:prR::SVwx", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'b':
            case 'd':
            case 'e':
            case 'f':
            case 'L':
            case 'p':
            case 'r':
            case 'S':
            case 'w':
            case 'x':
                test = c;
                break;

            case 'D':
                aa_set_double_output (1);
                break;

            case 'h':
                dieusage (RC_OK);

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'R':
                if (optarg && !uint0_scan (optarg, &repeat))
                    aa_strerr_diefu2sys (RC_FATAL_USAGE, "set repeat counter to ", optarg);
                else if (!optarg)
                    repeat = 1;
                else
                    ++repeat;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1 || test == 0)
        dieusage (RC_FATAL_USAGE);

again:
    if (lstat (argv[0], &st) < 0)
    {
        if (errno != ENOENT)
            aa_strerr_diefu2sys (RC_FATAL_IO, "stat ", argv[0]);
        else if (repeat >= 1)
        {
            if (repeat > 2)
                --repeat;
            else if (repeat == 2)
                return RC_ST_EXIST;
            sleep (1);
            goto again;
        }
        else
            return RC_ST_EXIST;
    }

    switch (test)
    {
        case 'b':
            return (S_ISBLK (st.st_mode)) ? RC_OK : RC_ST_FAIL;

        case 'd':
            return (S_ISDIR (st.st_mode)) ? RC_OK : RC_ST_FAIL;

        case 'e':
            return 0;

        case 'f':
            return (S_ISREG (st.st_mode)) ? RC_OK : RC_ST_FAIL;

        case 'L':
            return (S_ISLNK (st.st_mode)) ? RC_OK : RC_ST_FAIL;

        case 'p':
            return (S_ISFIFO (st.st_mode)) ? RC_OK : RC_ST_FAIL;

        case 'r':
            mode = R_OK;
            break;

        case 'S':
            return (S_ISSOCK (st.st_mode)) ? RC_OK : RC_ST_FAIL;

        case 'w':
            mode = W_OK;
            break;

        case 'x':
            mode = X_OK;
            break;
    }

    euid = geteuid ();
    if (euid == 0)
    {
        /* root can read/write any file */
        if (mode != X_OK)
            return RC_OK;
        /* and execute anything any execute permission set */
        else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
            return RC_OK;
        else
            return RC_ST_FAIL;
    }

    if (st.st_uid == euid)
        mode <<= 6;
    else if (is_group_member (st.st_gid))
        mode <<= 3;

    return (st.st_mode & mode) ? RC_OK : RC_ST_FAIL;
}
