/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-reboot.c
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

#include <getopt.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <anopa/common.h>
#include <anopa/output.h>

static void
dieusage (int rc)
{
    aa_die_usage (rc, "OPTION",
            " -r, --reboot                  Reboot the machine NOW\n"
            " -H, --halt                    Halt the machine NOW\n"
            " -p, --poweroff                Power off the machine NOW\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-reboot";
    struct
    {
        int cmd;
        const char *desc;
    } cmd[3] = {
        { .cmd = RB_HALT_SYSTEM, .desc = "halt" },
        { .cmd = RB_POWER_OFF,   .desc = "power off" },
        { .cmd = RB_AUTOBOOT,    .desc = "reboot" }
    };
    int i = -1;

    for (;;)
    {
        struct option longopts[] = {
            { "halt",               no_argument,        NULL,   'H' },
            { "help",               no_argument,        NULL,   'h' },
            { "poweroff",           no_argument,        NULL,   'p' },
            { "reboot",             no_argument,        NULL,   'r' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "HhprV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'H':
                i = 0;
                break;

            case 'h':
                dieusage (0);

            case 'p':
                i = 1;
                break;

            case 'r':
                i = 2;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 0 || i < 0)
        dieusage (1);

    if (reboot (cmd[i].cmd) < 0)
        aa_strerr_diefu2sys (2, cmd[i].desc, " the machine");

    /* unlikely :p */
    return 0;
}
