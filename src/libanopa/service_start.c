
#include <skalibs/direntry.h>
#include <skalibs/strerr2.h>
#include <anopa/service.h>
#include <anopa/ga_int_list.h>
#include <anopa/err.h>
#include <anopa/service_status.h>
#include "service_internal.h"

void
aa_unmark_service (int si)
{
    aa_service *s = aa_service (si);
    int i;

    if (--s->nb_mark > 0)
        return;

    for (i = 0; i < genalloc_len (int, &s->needs); ++i)
        aa_unmark_service (list_get (&s->needs, i));
    for (i = 0; i < genalloc_len (int, &s->wants); ++i)
        aa_unmark_service (list_get (&s->wants, i));

    add_to_list (&aa_tmp_list, si, 0);
    remove_from_list (&aa_main_list, si);
}

int
aa_mark_service (int si, int in_main, int no_wants, aa_load_fail_cb lf_cb)
{
    int r;

    r = aa_ensure_service_loaded (si, AA_MODE_START, no_wants, lf_cb);
    if (r < 0)
    {
        if (in_main)
        {
            add_to_list (&aa_tmp_list, si, 0);
            remove_from_list (&aa_main_list, si);
        }
        return r;
    }

    if (!in_main)
    {
        add_to_list (&aa_main_list, si, 0);
        remove_from_list (&aa_tmp_list, si);
    }

    aa_service (si)->nb_mark++;
    return 0;
}

int
_it_start_needs (direntry *d, void *data)
{
    struct it_data *it_data = data;
    int type;
    int sni;
    int r;

    tain_now_g ();
    type = aa_get_service (d->d_name, &sni, 1);
    if (type < 0)
        r = type;
    else
        r = aa_mark_service (sni, type == AA_SERVICE_FROM_MAIN,
                it_data->no_wants, it_data->lf_cb);
    if (r == -ERR_ALREADY_UP)
        return 0;
    else if (r < 0)
    {
        aa_service *s = aa_service (it_data->si);
        const char *name = aa_service_name (s);
        int l_n = strlen (name);
        int l_em = strlen (errmsg[-r]);
        char buf[l_n + 2 + l_em + 1];
        int l = genalloc_len (int, &s->needs);
        int i;

        for (i = 0; i < l; ++i)
            aa_unmark_service (list_get (&s->needs, i));

        byte_copy (buf, l_n, name);
        byte_copy (buf + l_n, 2, ": ");
        byte_copy (buf + l_n + 2, l_em + 1, errmsg[-r]);

        aa_service_status_set_err (&s->st, ERR_DEPEND, buf);
        if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
            strerr_warnwu2sys ("write service status file for ", aa_service_name (s));

        if (it_data->lf_cb)
            it_data->lf_cb (it_data->si, AA_LOADFAIL_NEEDS, d->d_name, -r);

        return -ERR_DEPEND;
    }

    add_to_list (&aa_service (it_data->si)->needs, sni, 0);
    add_to_list (&aa_service (it_data->si)->after, sni, 1);
    return 0;
}

int
_it_start_wants (direntry *d, void *data)
{
    struct it_data *it_data = data;
    int type;
    int swi;
    int r;

    tain_now_g ();
    type = aa_get_service (d->d_name, &swi, 1);
    if (type < 0)
        r = type;
    else
        r = aa_mark_service (swi, type == AA_SERVICE_FROM_MAIN,
                it_data->no_wants, it_data->lf_cb);
    if (r == -ERR_ALREADY_UP)
        return 0;
    if (r < 0)
    {
        if (it_data->lf_cb)
            it_data->lf_cb (it_data->si, AA_LOADFAIL_WANTS, d->d_name, -r);
        return r;
    }

    add_to_list (&aa_service (it_data->si)->wants, swi, 0);
    return 0;
}

int
_it_start_after (direntry *d, void *data)
{
    struct it_data *it_data = data;
    int sai;
    int r;

    tain_now_g ();
    r = aa_get_service (d->d_name, &sai, 0);
    if (r < 0)
        return 0;

    add_to_list (&aa_service (it_data->si)->after, sai, 1);
    return 0;
}

int
_it_start_before (direntry *d, void *data)
{
    struct it_data *it_data = data;
    int sbi;
    int r;

    tain_now_g ();
    r = aa_get_service (d->d_name, &sbi, 0);
    if (r < 0)
        return 0;

    add_to_list (&aa_service (sbi)->after, it_data->si, 1);
    return 0;
}
