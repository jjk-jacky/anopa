/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-bw.c
 * Copyright (C) 2018 Olivier Brunel <jjk@jjacky.com>
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
#include <skalibs/types.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>
#include <anopa/output.h>

#define RC_ST_FAIL      1 << 1

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] NUM1 NUM2",
            " -A, --all                     Success if NUM1 & NUM2 == NUM2\n"
            " -a, --and                     Success if NUM1 & NUM2 != 0; Default\n"
            " -e, --equal                   Success if NUM1 == NUM2\n"
            " -p, --print                   Print NUM1 & NUM2 (NUM1 w/ --equal)\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[], char * const envp[])
{
    PROG = "aa-bw";
    enum
    {
        MODE_AND = 0,
        MODE_ALL,
        MODE_EQUAL
    } mode = MODE_AND;
    unsigned int n1;
    unsigned int n2;
    int print = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "all",                no_argument,        NULL,   'A' },
            { "and",                no_argument,        NULL,   'a' },
            { "equal",              no_argument,        NULL,   'e' },
            { "help",               no_argument,        NULL,   'h' },
            { "print",              no_argument,        NULL,   'p' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "AaehpV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'A':
                mode = MODE_ALL;
                break;

            case 'a':
                mode = MODE_AND;
                break;

            case 'e':
                mode = MODE_EQUAL;
                break;

            case 'h':
                dieusage (RC_OK);

            case 'p':
                print = 1;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2)
        dieusage (RC_FATAL_USAGE);

    if (!uint0_scan (argv[0], &n1))
        aa_strerr_dief2x (RC_FATAL_USAGE, "Invalid argument: ", argv[0]);

    if (!uint0_scan (argv[1], &n2))
        aa_strerr_dief2x (RC_FATAL_USAGE, "Invalid argument: ", argv[1]);

    if (mode != MODE_EQUAL)
        n1 &= n2;

    if (print)
    {
        char buf[UINT_FMT];
        size_t len;

        len = uint_fmt (buf, n1);
        buf[len] = '\n';
        aa_bb_flush (AA_OUT, buf, len + 1);
    }

    if (mode == MODE_AND)
        return (n1) ? RC_OK : RC_ST_FAIL;
    else
        return (n1 == n2) ? RC_OK : RC_ST_FAIL;
}
