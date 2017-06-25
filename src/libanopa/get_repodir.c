/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * get_repodir.c
 * Copyright (C) 2017 Olivier Brunel <jjk@jjacky.com>
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

#include <stdlib.h>
#include <string.h>
#include <anopa/common.h>
#include <anopa/get_repodir.h>

#define AA_DEFAULT_REPODIR          "/run/services"

const char *
aa_get_repodir (void)
{
    const char *repodir;

    repodir = getenv ("AA_REPODIR");
    if (repodir)
        repodir = strdup (repodir);
    else
        repodir = AA_DEFAULT_REPODIR;

    return repodir;
}
