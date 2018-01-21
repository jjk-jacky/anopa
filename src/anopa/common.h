/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * common.h
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

#ifndef _AA_COMMON_H
#define _AA_COMMON_H

#include <anopa/rc.h>

#define LISTDIR_PREFIX      "/etc/anopa/listdirs/"

enum
{
    RC_ST_UNKNOWN       = 1 << 1,   /* at least 1 service unknown */
    RC_ST_SKIPPED       = 1 << 2,   /* at least 1 service skipped (start) */
    RC_ST_FAILED        = 1 << 3,   /* at least 1 service failed */
    RC_ST_ESSENTIAL     = 1 << 4    /* at least 1 failed service was essential */
};

#endif /* _AA_COMMON_H */
