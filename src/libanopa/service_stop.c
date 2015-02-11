
#include <skalibs/direntry.h>
#include <anopa/service.h>
#include <anopa/ga_int_list.h>
#include "service_internal.h"

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
