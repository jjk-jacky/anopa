/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * service_name.c
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

#include <skalibs/bytestr.h>

int
_is_valid_service_name (const char *name, int len)
{
    if (len <= 0)
        return 0;
    if (name[0] == '.')
        return 0;
    if (name[0] == '@' || name[len - 1] == '@')
        return 0;
    if (byte_chr (name, len, '/') < len)
        return 0;
    return 1;
}
