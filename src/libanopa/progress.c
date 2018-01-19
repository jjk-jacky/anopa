/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * progress.c
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

#include <skalibs/bytestr.h>
#include <skalibs/types.h>
#include <anopa/progress.h>
#include <anopa/output.h>

#if 0
static const char *utf8_edge[] = { "\u2595", " ", "\u258f" };
static const char *utf8_bars[] = { " ", "\u258f", "\u258e", "\u258d", "\u258c", "\u258b", "\u258a", "\u2589", "\u2588" };
#define utf8_per_c  sizeof (utf8_bars) / sizeof (*utf8_bars)
#else
static const char *utf8_edge[] = { "\u2590", " ", "\u258c" };
static const char *utf8_bars[] = { " ", "\u258c", "\u2588" };
#define utf8_per_c  sizeof (utf8_bars) / sizeof (*utf8_bars)
#endif

static const char *ascii_edge[] = { "[", " ", "]" };
static const char *ascii_bars[] = { " ", "-", "=", "#" };
#define ascii_per_c  sizeof (ascii_bars) / sizeof (*ascii_bars)

void
aa_progress_free (aa_progress *p)
{
    stralloc_free (&p->sa);
}

int
aa_progress_update (aa_progress *pg)
{
    char *s;
    size_t skip;
    size_t len;
    size_t rr;
    int cur;
    int max;
    size_t r;

    /* sanity: we require at least a NUL byte (for empty msg) */
    if (pg->sa.len == 0)
        return -1;

    /* moving past msg */
    skip = byte_chr (pg->sa.s, pg->sa.len, '\0') + 1;
    s = pg->sa.s + skip;
    len = pg->sa.len - skip;
    if (len <= 0)
        return -1;

    /* now look for last full line to process */
    r = byte_rchr (s, len, '\n');
    if (r >= len)
        return -1;
    s[r] = '\0';

    rr = byte_rchr (s, r, '\n');
    if (rr < r)
    {
        s += rr + 1;
        len = r - rr - 1;
    }
    else
        len = r;

    /* step */
    for (rr = 0; *s != ' ' && len > 0; ++s, --len)
    {
        if (*s < '0' || *s > '9')
            goto err;
        rr *= 10;
        rr += *s - '0';
    }
    if (len <= 1)
        goto err;
    ++s; --len;

    for (cur = 0; *s != ' ' && len > 0; ++s, --len)
    {
        if (*s < '0' || *s > '9')
            goto err;
        cur *= 10;
        cur += *s - '0';
    }
    if (len <= 1)
        goto err;
    ++s; --len;

    for (max = 0; *s != ' ' && len > 0; ++s, --len)
    {
        if (*s < '0' || *s > '9')
            goto err;
        max *= 10;
        max += *s - '0';
    }
    if (*s == ' ')
    {
        ++s; --len;
    }
    else if (len > 0)
        goto err;

    pg->step = rr;
    pg->pctg = (double) cur / (double) max;

    ++len; /* include NUL */
    memmove (pg->sa.s, s, len);
    pg->sa.len = len;
    return 0;

err:
    s = pg->sa.s + skip + r + 1;
    len = pg->sa.len - skip - r - 1;
    memmove (pg->sa.s + skip, s, len);
    pg->sa.len = skip + len;
    return -2;
}

void
aa_progress_draw (aa_progress *pg, const char *title, int cols, int is_utf8)
{
    const char **edge;
    const char **bars;
    int per_c;
    char buf[UINT_FMT];
    unsigned int p1;
    unsigned int p2;
    size_t w;
    double d;
    size_t n;
    size_t i;

    p1 = 100 * pg->pctg;
    p2 = 10000 * pg->pctg - (100 * p1);
    if (p2 == 100)
    {
        ++p1;
        p2 = 0;
    }

    if (is_utf8)
    {
        edge = utf8_edge;
        bars = utf8_bars;
        per_c = utf8_per_c;
    }
    else
    {
        edge = ascii_edge;
        bars = ascii_bars;
        per_c = ascii_per_c;
    }

    /* 7: for "100.0% "   10: margin on the right */
    w = (size_t) cols - strlen (title) - 1 - 7 - 10;
    if (pg->sa.s[0] != '\0')
        w -= byte_chr (pg->sa.s, pg->sa.len, '\0');
    if (w < 10)
        w = 0;
    d = pg->pctg * w * per_c;
    n = d / per_c;

    aa_is (AA_OUT, title);
    aa_is (AA_OUT, ":");
    if (w)
    {
        aa_is (AA_OUT, edge[0]);
        for (i = 0; i < n; ++i)
            aa_is (AA_OUT, bars[per_c - 1]);
        if (n < w)
            aa_is (AA_OUT, bars[(int) d % per_c]);
        for (i = n + 1; i < w; ++i)
            aa_is (AA_OUT, edge[1]);
        aa_is (AA_OUT, edge[2]);
    }
    aa_is (AA_OUT, " ");

    buf[uint_fmt (buf, p1)] = '\0';
    aa_is (AA_OUT, buf);
    aa_is (AA_OUT, ".");
    if (uint_fmt (buf, p2) == 1)
        buf[1] = '0';
    buf[2] = '\0';
    aa_is (AA_OUT, buf);
    aa_is (AA_OUT, "% ");

    if (pg->sa.s[0] != '\0')
        aa_is (AA_OUT, pg->sa.s);
    aa_is_flush (AA_OUT, "\n");
}
