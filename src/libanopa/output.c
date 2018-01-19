/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * output.c
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

#include <unistd.h> /* isatty() */
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <skalibs/djbunix.h>
#include <skalibs/types.h>
#include <anopa/output.h>
#include <anopa/err.h> /* ERR_IO */

static int istty[2] = { -1, 0 };
static int double_output = 0;

static char buf_log[BUFFER_OUTSIZE];
static buffer_t buffer_log = BUFFER_ZERO;

#define is_tty(n)           (istty[0] > -1 || chk_tty ()) && istty[n]

#define putb(w,s,l)         buffer_put ((w) ? buffer_2 : buffer_1small, s, l)
#define putb_flush(w,s,l)   buffer_putflush ((w) ? buffer_2 : buffer_1small, s, l)

#define logb(s,l)           buffer_put (&buffer_log, s, l)
#define logb_flush(s,l)     buffer_putflush (&buffer_log, s, l)

static int
chk_tty (void)
{
    istty[0] = isatty (1);
    istty[1] = isatty (2);
    return 1;
}

void
aa_set_double_output (int enabled)
{
    double_output = !!enabled;
}

int aa_set_log_file (const char *file_or_fd)
{
    if (buffer_log.fd >= 0)
    {
        fd_close (buffer_log.fd);
        buffer_log.fd = -1;
    }

    if (!file_or_fd)
        return 0;

    if (*file_or_fd >= '0' && *file_or_fd <= '9')
    {
        /* Note: we don't allow to use a fd <= 2 */
        errno = 0;
        if (!int0_scan (file_or_fd, &buffer_log.fd) || buffer_log.fd <= 2)
        {
            if (errno == 0)
                errno = EINVAL;
            buffer_log.fd = -1;
            return -1;
        }
    }
    else
    {
        buffer_log.fd = open_append (file_or_fd);
        if (buffer_log.fd < 0)
            return -2;
    }

    if (!buffer_init (&buffer_log, &fd_writev, buffer_log.fd, buf_log, sizeof (buf_log)))
    {
        int e = errno;
        fd_close (buffer_log.fd);
        errno = e;
        buffer_log.fd = -1;
        return -3;
    }

    return 0;
}

void aa_set_log_file_or_die (const char *file_or_fd)
{
    int r;

    r = aa_set_log_file (file_or_fd);
    if (r < 0)
    {
        if (r == -1)
            aa_strerr_diefu3sys (1, "set logfile to FD '", file_or_fd, "'");
        else
            aa_strerr_diefu3sys (ERR_IO, "set logfile to '", file_or_fd, "'");
    }
}

void
aa_bb (int where, const char *s, size_t len)
{
    putb (where, s, len);
    if (double_output)
        putb (!where, s, len);
    if (buffer_log.fd >= 0)
        logb (s, len);
}

void
aa_bb_flush (int where, const char *s, size_t len)
{
    putb_flush (where, s, len);
    if (double_output)
        putb_flush (!where, s, len);
    if (buffer_log.fd >= 0)
        logb_flush (s, len);
}

void
aa_ib (int where, const char *s, size_t len)
{
    if (is_tty (where))
        putb (where, s, len);
    if (double_output && is_tty (!where))
        putb (!where, s, len);
}

void
aa_ib_flush (int where, const char *s, size_t len)
{
    if (is_tty (where))
        putb_flush (where, s, len);
    if (double_output && is_tty (!where))
        putb_flush (!where, s, len);
}

void
aa_bs_end (int where)
{
    aa_is (where, ANSI_HIGHLIGHT_OFF);
    aa_bs_flush (where, "\n");
}

void
aa_put_err (const char *name, const char *msg, int end)
{
    aa_is (AA_ERR, ANSI_HIGHLIGHT_RED_ON);
    aa_bs (AA_ERR, "==> ERROR: ");
    aa_is (AA_ERR, ANSI_HIGHLIGHT_ON);
    aa_bs (AA_ERR, name);
    if (msg)
    {
        aa_bs (AA_ERR, ": ");
        aa_bs (AA_ERR, msg);
    }
    if (end)
        aa_end_err ();
}

void
aa_put_warn (const char *name, const char *msg, int end)
{
    aa_is (AA_ERR, ANSI_HIGHLIGHT_YELLOW_ON);
    aa_bs (AA_ERR, "==> WARNING: ");
    aa_is (AA_ERR, ANSI_HIGHLIGHT_ON);
    aa_bs (AA_ERR, name);
    if (msg)
    {
        aa_bs (AA_ERR, ": ");
        aa_bs (AA_ERR, msg);
    }
    if (end)
        aa_end_warn ();
}

void
aa_put_title (int main, const char *name, const char *title, int end)
{
    aa_is (AA_OUT, (main) ? ANSI_HIGHLIGHT_GREEN_ON : ANSI_HIGHLIGHT_BLUE_ON);
    aa_bs (AA_OUT, (main) ? "==> " : "  -> ");
    aa_is (AA_OUT, ANSI_HIGHLIGHT_ON);
    aa_bs (AA_OUT, name);
    if (title)
    {
        aa_bs (AA_OUT, ": ");
        aa_bs (AA_OUT, title);
    }
    if (end)
        aa_end_title ();
}

void
aa_strerr_warn (const char *s1,
                const char *s2,
                const char *s3,
                const char *s4,
                const char *s5,
                const char *s6,
                const char *s7,
                const char *s8,
                const char *s9,
                const char *s10)
{
    aa_bs (AA_ERR, PROG);
    aa_bs (AA_ERR, ": ");
    if (s1)
        aa_bs (AA_ERR, s1);
    if (s2)
        aa_bs (AA_ERR, s2);
    if (s3)
        aa_bs (AA_ERR, s3);
    if (s4)
        aa_bs (AA_ERR, s4);
    if (s5)
        aa_bs (AA_ERR, s5);
    if (s6)
        aa_bs (AA_ERR, s6);
    if (s7)
        aa_bs (AA_ERR, s7);
    if (s8)
        aa_bs (AA_ERR, s8);
    if (s9)
        aa_bs (AA_ERR, s9);
    if (s10)
        aa_bs (AA_ERR, s10);
    aa_bs_flush (AA_ERR, "\n");
}

void
aa_strerr_die (int rc,
               const char *s1,
               const char *s2,
               const char *s3,
               const char *s4,
               const char *s5,
               const char *s6,
               const char *s7,
               const char *s8,
               const char *s9)
{
    aa_strerr_warn ("fatal: ", s1, s2, s3, s4, s5, s6, s7, s8, s9);
    _exit (rc);
}
