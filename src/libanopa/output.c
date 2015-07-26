/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * output.c
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

#include <unistd.h> /* isatty() */
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <anopa/output.h>

static int istty[2] = { -1, 0 };
static int double_output = 0;

#define is_tty(n)           (istty[0] > -1 || chk_tty ()) && istty[n]

#define putb_noflush(w,s,l) buffer_putnoflush ((w) ? buffer_2 : buffer_1small, s, l)
#define putb_flush(w,s,l)   buffer_putflush ((w) ? buffer_2 : buffer_1small, s, l)

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

void
aa_bb_noflush (int where, const char *s, int len)
{
    putb_noflush (where, s, len);
    if (double_output)
        putb_noflush (!where, s, len);
}

void
aa_bb_flush (int where, const char *s, int len)
{
    putb_flush (where, s, len);
    if (double_output)
        putb_flush (!where, s, len);
}

void
aa_ib_noflush (int where, const char *s, int len)
{
    if (is_tty (where))
        putb_noflush (where, s, len);
    if (double_output && is_tty (!where))
        putb_noflush (!where, s, len);
}

void
aa_ib_flush (int where, const char *s, int len)
{
    if (is_tty (where))
        putb_flush (where, s, len);
    if (double_output && is_tty (!where))
        putb_flush (!where, s, len);
}

void
aa_bs_end (int where)
{
    aa_is_noflush (where, ANSI_HIGHLIGHT_OFF);
    aa_bs_flush (where, "\n");
}

void
aa_put_err (const char *name, const char *msg, int end)
{
    aa_is_noflush (AA_ERR, ANSI_HIGHLIGHT_RED_ON);
    aa_bs_noflush (AA_ERR, "==> ERROR: ");
    aa_is_noflush (AA_ERR, ANSI_HIGHLIGHT_ON);
    aa_bs_noflush (AA_ERR, name);
    if (msg)
    {
        aa_bs_noflush (AA_ERR, ": ");
        aa_bs_noflush (AA_ERR, msg);
    }
    if (end)
        aa_end_err ();
}

void
aa_put_warn (const char *name, const char *msg, int end)
{
    aa_is_noflush (AA_ERR, ANSI_HIGHLIGHT_YELLOW_ON);
    aa_bs_noflush (AA_ERR, "==> WARNING: ");
    aa_is_noflush (AA_ERR, ANSI_HIGHLIGHT_ON);
    aa_bs_noflush (AA_ERR, name);
    if (msg)
    {
        aa_bs_noflush (AA_ERR, ": ");
        aa_bs_noflush (AA_ERR, msg);
    }
    if (end)
        aa_end_warn ();
}

void
aa_put_title (int main, const char *name, const char *title, int end)
{
    aa_is_noflush (AA_OUT, (main) ? ANSI_HIGHLIGHT_GREEN_ON : ANSI_HIGHLIGHT_BLUE_ON);
    aa_bs_noflush (AA_OUT, (main) ? "==> " : "  -> ");
    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    aa_bs_noflush (AA_OUT, name);
    if (title)
    {
        aa_bs_noflush (AA_OUT, ": ");
        aa_bs_noflush (AA_OUT, title);
    }
    if (end)
        aa_end_title ();
}
