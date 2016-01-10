/*
 * anopa - Copyright (C) 2015-2016 Olivier Brunel
 *
 * ga_int_list.c
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

#include <anopa/ga_int_list.h>

int
add_to_list (genalloc *list, int si, int check_for_dupes)
{
    if (check_for_dupes)
    {
        int len = genalloc_len (int, list);
        int i;

        for (i = 0; i < len; ++i)
            if (list_get (list, i) == si)
                return 0;
    }

    genalloc_append (int, list, &si);
    return 1;
}

int
remove_from_list (genalloc *list, int si)
{
    int len = genalloc_len (int, list);
    int i;

    for (i = 0; i < len; ++i)
        if (list_get (list, i) == si)
        {
            ga_remove (int, list, i);
            return 1;
        }

    return 0;
}

int
is_in_list (genalloc *list, int si)
{
    int len = genalloc_len (int, list);
    int i;

    for (i = 0; i < len; ++i)
        if (list_get (list, i) == si)
            return 1;
    return 0;
}
