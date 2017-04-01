/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * ga_list.c
 * Copyright (C) 2015-2017 Olivier Brunel <jjk@jjacky.com>
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

#include <string.h>
#include <anopa/ga_list.h>

void
ga_remove (genalloc *ga, size_t size, int i)
{
    size_t len = ga->len / size;
    size_t c = len - i - 1;

    if (i < 0 || (size_t) i >= len)
        return;
    if (c > 0)
        memmove (ga->s + (i * size), ga->s + ((i + 1) * size), c * size);

    ga->len -= size;
}

int
ga_find (genalloc *ga, size_t size, void const *val)
{
    size_t len = ga->len / size;
    size_t i;

    for (i = 0; i < len; ++i)
        if (memcmp (ga->s + (i * size), val, size) == 0)
            return i;
    return -1;
}

int
ga_add_val (genalloc *ga, size_t size, void const *val, int check_for_dupes)
{
    if (check_for_dupes)
    {
        int i = ga_find (ga, size, val);

        if (i >= 0)
            return 0;
    }

    stralloc_catb (ga, val, size);
    return 1;
}

int
ga_remove_val (genalloc *ga, size_t size, void const *val)
{
    int i = ga_find (ga, size, val);

    if (i < 0)
        return 0;

    ga_remove (ga, size, i);
    return 1;
}
