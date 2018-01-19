/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-incmdline.c
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
#include <skalibs/djbunix.h>
#include <skalibs/stralloc.h>
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <anopa/common.h>
#include <anopa/output.h>

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION] NAME",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -f, --file FILE               Use FILE instead of /proc/cmdline\n"
            " -q, --quiet                   Don't write value (if any) to stdout\n"
            " -s, --safe[=C]                Ignore argument if value contain C (default: '/')\n"
            " -r, --required                Ignore argument if no value specified\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-incmdline";
    stralloc sa = STRALLOC_ZERO;
    const char *file = "/proc/cmdline";
    int quiet = 0;
    int req = 0;
    char safe = '\0';
    size_t len_arg;
    size_t start;
    size_t i;

    for (;;)
    {
        struct option longopts[] = {
            { "double-output",      no_argument,        NULL,   'D' },
            { "file",               no_argument,        NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "quiet",              no_argument,        NULL,   'q' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "required",           no_argument,        NULL,   'r' },
            { "safe",               optional_argument,  NULL,   's' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "Df:hO:qrs::V", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                aa_set_double_output (1);
                break;

            case 'f':
                file = optarg;
                break;

            case 'h':
                dieusage (0);

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'q':
                quiet = 1;
                break;

            case 'r':
                req = 1;
                break;

            case 's':
                if (!optarg)
                    safe = '/';
                else if (!*optarg || optarg[1])
                    dieusage (1);
                else
                    safe = *optarg;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
        dieusage (1);

    if (!openslurpclose (&sa, file))
            aa_strerr_diefu2sys (2, "read ", file);

    len_arg = strlen (argv[0]);
    for (start = i = 0; i < sa.len; ++i)
    {
        if (sa.s[i] == '=' || sa.s[i] == ' ' || sa.s[i] == '\t'
                || sa.s[i] == '\n' || sa.s[i] == '\0')
        {
            int found = (i - start == len_arg && !str_diffn (sa.s + start, argv[0], len_arg));
            size_t len;

            if (sa.s[i] != '=')
            {
                if (found)
                    return (req) ? 3 : 0;
                start = ++i;
                goto next;
            }
            else if (found && quiet && !safe)
                return (req) ? 3 : 0;

            start = ++i;
            if (sa.s[start] != '"')
                for (len = 0;
                        start + len < sa.len
                        && sa.s[start + len] != ' ' && sa.s[start + len] != '\t';
                        ++len)
                    ;
            else
            {
                ++start;
                len = byte_chr (sa.s + start, sa.len - start, '"');
            }

            if (found)
            {
                if (len == sa.len - start)
                    --len;
                if (safe && byte_chr (sa.s + start, len, safe) < len)
                    return 3;
                if (req && len == 0)
                    return 3;
                else if (!quiet)
                {
                    aa_bb (AA_OUT, sa.s + start, len);
                    aa_bs_flush (AA_OUT, "\n");
                }
                return 0;
            }

            start += len;
            i = ++start;
next:
            while (i < sa.len && (sa.s[i] == ' ' || sa.s[i] == '\t'))
                start = ++i;
        }
    }

    return 3;
}
