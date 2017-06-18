/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * services.c
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

#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <s6/ftrigr.h>
#include <anopa/service.h>

genalloc aa_services    = GENALLOC_ZERO;
stralloc aa_names       = STRALLOC_ZERO;
genalloc aa_main_list   = GENALLOC_ZERO;
genalloc aa_tmp_list    = GENALLOC_ZERO;
int aa_secs_timeout     = 0;

ftrigr_t _aa_ft         = FTRIGR_ZERO;
aa_exec_cb _exec_cb     = NULL;
