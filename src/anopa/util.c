/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * util.c
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

#include <skalibs/buffer.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>
#include <string.h>
#include <errno.h>
#include "util.h"

int
process_names_from_stdin (names_cb process_name, void *data)
{
    int salen = satmp.len;
    int r;

    for (;;)
    {
        satmp.len = salen;
        r = skagetlnsep (buffer_0small, &satmp, "\n", 1);
        if (r < 0)
        {
            if (errno != EPIPE)
                break;
        }
        else if (r == 0)
            break;
        else
            satmp.len--;

        if (!stralloc_0 (&satmp))
        {
            r = -1;
            break;
        }
        process_name (satmp.s + salen, data);
    }

    satmp.len = salen;
    return r;
}

void
unslash (char *s)
{
    int l = strlen (s) - 1;
    if (l <= 0)
        return;
    if (s[l] == '/')
        s[l] = '\0';
}
