/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * service_stop.c
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

#include <skalibs/direntry.h>
#include <anopa/service.h>
#include <anopa/ga_int_list.h>
#include "service_internal.h"

int
_name_stop_needs (const char *name, struct it_data *it_data)
{
    int sni;
    int r;

    tain_now_g ();
    r = aa_get_service (name, &sni, 0);
    if (r < 0)
        return 0;

    if (it_data->al_cb)
        it_data->al_cb (sni, AA_AUTOLOAD_NEEDS, aa_service_name (aa_service (it_data->si)), 0);

    add_to_list (&aa_service (sni)->needs, it_data->si, 0);
    add_to_list (&aa_service (sni)->after, it_data->si, 1);
    return 0;
}

int
_it_stop_needs (direntry *d, void *data)
{
    return _name_stop_needs (d->d_name, (struct it_data *) data);
}

int
_it_stop_after (direntry *d, void *data)
{
    struct it_data *it_data = data;
    int sai;
    int r;

    tain_now_g ();
    r = aa_get_service (d->d_name, &sai, 0);
    if (r < 0)
        return 0;

    add_to_list (&aa_service (sai)->after, it_data->si, 1);
    return 0;
}

int
_it_stop_before (direntry *d, void *data)
{
    struct it_data *it_data = data;
    int sbi;
    int r;

    tain_now_g ();
    r = aa_get_service (d->d_name, &sbi, 0);
    if (r < 0)
        return 0;

    add_to_list (&aa_service (it_data->si)->after, sbi, 1);
    return 0;
}
