/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * service_internal.h
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

#ifndef AA_SERVICE_INTERNAL_H
#define AA_SERVICE_INTERNAL_H

#include <skalibs/direntry.h>
#include <anopa/service.h>

extern ftrigr_t _aa_ft;
extern aa_exec_cb _exec_cb;

struct it_data
{
    aa_mode mode;
    int si;
    int no_wants;
    aa_autoload_cb al_cb;
};

extern int _is_valid_service_name (const char *name, int len);

extern int _name_start_needs (const char *name, struct it_data *it_data);
extern int _it_start_needs  (direntry *d, void *data);
extern int _it_start_wants  (direntry *d, void *data);
extern int _it_start_after  (direntry *d, void *data);
extern int _it_start_before (direntry *d, void *data);

extern int _name_stop_needs (const char *name, struct it_data *it_data);
extern int _it_stop_needs   (direntry *d, void *data);
extern int _it_stop_after   (direntry *d, void *data);
extern int _it_stop_before  (direntry *d, void *data);

extern int _exec_oneshot (int si, aa_mode mode);
extern int _exec_longrun (int si, aa_mode mode);

#endif /* AA_SERVICE_INTERNAL_H */
