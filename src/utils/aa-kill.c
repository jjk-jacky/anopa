/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-kill.c
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

#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <skalibs/sig.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/uint.h>
#include <anopa/common.h>
#include <anopa/output.h>
#include <anopa/scan_dir.h>

static struct {
    unsigned int term : 1;
    unsigned int kill : 1;
    unsigned int hup : 1;

    unsigned int skip_at : 1;
} send = { 0, };
static char ownpid[UINT_FMT];

#if 0
#include <skalibs/buffer.h>
static void _kill (pid_t pid, int sig)
{
    char buf[UINT_FMT];
    unsigned int u;

    u = pid;
    buf[uint_fmt (buf, u)] = 0;
    buffer_putsnoflush (buffer_1small, "kill(");
    if (u == (unsigned int) -1)
        buffer_putsnoflush (buffer_1small, "-1");
    else
        buffer_putsnoflush (buffer_1small, buf);
    buffer_putsnoflush (buffer_1small, ",");
    buffer_putsnoflush (buffer_1small, sig_name (sig));
    buffer_putsflush (buffer_1small, ")\n");
}
#else
#define _kill(pid,sig)  kill (pid, sig)
#endif

static int
it_kill (direntry *d, void *data)
{
    stralloc *sa = data;
    char c;
    int l;
    int r;

    /* ignore files, not-number dirs, PID 1 and ourself */
    if (d->d_type != DT_DIR || *d->d_name < '1' || *d->d_name > '9'
            || str_equal (d->d_name, "1") || str_equal (d->d_name, ownpid))
        return 0;

    l = sa->len;
    sa->s[l - 1] = '/';
    if (stralloc_cats (sa, d->d_name)
            && stralloc_catb (sa, "/cmdline", sizeof ("/cmdline")))
        r = openreadnclose (sa->s, &c, 1);
    else
        r = -1;
    sa->len = l;
    sa->s[l - 1] = '\0';

    /* skip empty cmdline (kernel threads) and anything starting with '@' */
    if (r == 1 && c != '@')
    {
        unsigned int u;
        pid_t pid;

        if (!uint_scan (d->d_name, &u))
            goto done;
        pid = (pid_t) u;
        if (pid != u)
            goto done;
        if (send.hup)
            _kill (pid, SIGHUP);
        if (send.term)
        {
            _kill (pid, SIGTERM);
            _kill (pid, SIGCONT);
        }
        if (send.kill)
            _kill (pid, SIGKILL);
    }

done:
    return 0;
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...]",
            " -u, --hup                     Send SIGHUP\n"
            " -t, --term                    Send SIGTERM then SIGCONT\n"
            " -k, --kill                    Send SIGKILL\n"
            " -s, --skip-at                 Skip processes whose cmdline starts with '@'\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-kill";

    for (;;)
    {
        struct option longopts[] = {
            { "help",               no_argument,        NULL,   'h' },
            { "kill",               no_argument,        NULL,   'k' },
            { "skip-at",            no_argument,        NULL,   's' },
            { "term",               no_argument,        NULL,   't' },
            { "hup",                no_argument,        NULL,   'u' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "hkstuV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'h':
                dieusage (0);

            case 'k':
                send.kill = 1;
                break;

            case 's':
                send.skip_at = 1;
                break;

            case 't':
                send.term = 1;
                break;

            case 'u':
                send.hup = 1;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc > 0 || (!send.hup && !send.term && !send.kill))
        dieusage (1);

    if (send.skip_at)
    {
        stralloc sa = STRALLOC_ZERO;
        unsigned int u;

        u = (unsigned int) getpid ();
        ownpid[uint_fmt (ownpid, u)] = '\0';

        if (!stralloc_catb (&sa, "/proc", sizeof ("/proc")))
            aa_strerr_diefu1sys (1, "stralloc_catb");
        if (aa_scan_dir (&sa, 0, it_kill, &sa) < 0)
            aa_strerr_diefu1sys (1, "scan /proc");
        stralloc_free (&sa);
    }
    else
    {
        if (send.hup)
        {
            sig_ignore (SIGHUP);
            _kill (-1, SIGHUP);
        }

        if (send.term)
        {
            sig_ignore (SIGTERM);
            _kill (-1, SIGTERM);
            _kill (-1, SIGCONT);
        }

        if (send.kill)
            _kill (-1, SIGKILL);
    }

    return 0;
}
