/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * enable_service.h
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

#ifndef AA_ENABLE_SERVICE_H
#define AA_ENABLE_SERVICE_H

#include <skalibs/stralloc.h>
#include <anopa/common.h>

typedef enum
{
    AA_FLAG_AUTO_ENABLE_NEEDS   = (1 << 0),
    AA_FLAG_AUTO_ENABLE_WANTS   = (1 << 1),
    AA_FLAG_SKIP_DOWN           = (1 << 2),
    AA_FLAG_UPGRADE_SERVICEDIR  = (1 << 3),
    AA_FLAG_NO_SUPERVISE        = (1 << 4),
    /* private */
    _AA_FLAG_IS_SERVICEDIR      = (1 << 5),
    _AA_FLAG_IS_CONFIGDIR       = (1 << 6),
    _AA_FLAG_IS_1OF4            = (1 << 7),
    _AA_FLAG_IS_LOGGER          = (1 << 8)
} aa_enable_flags;

extern stralloc aa_sa_sources;

typedef void (*aa_warn_fn) (const char *name, int err);
typedef void (*aa_auto_enable_cb) (const char *name, aa_enable_flags type);

extern int aa_enable_service (const char        *name,
                              aa_warn_fn         warn_fn,
                              aa_enable_flags    flags,
                              aa_auto_enable_cb  ae_cb);

#endif /* AA_ENABLE_SERVICE_H */
