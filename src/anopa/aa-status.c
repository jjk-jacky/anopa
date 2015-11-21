/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-status.c
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

#define _BSD_SOURCE

#include "anopa/config.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <skalibs/bytestr.h>
#include <skalibs/djbunix.h>
#include <skalibs/genalloc.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>
#include <skalibs/uint.h>
#include <skalibs/djbtime.h>
#include <skalibs/tai.h>
#include <skalibs/sig.h>
#include <skalibs/error.h>
#include <s6/s6-supervise.h>
#include <anopa/common.h>
#include <anopa/output.h>
#include <anopa/init_repo.h>
#include <anopa/scan_dir.h>
#include <anopa/service.h>
#include <anopa/service_status.h>
#include <anopa/err.h>
#include "util.h"
#include "common.h"

enum
{
    MODE_NORMAL = 0,
    MODE_LIST,
    MODE_DRY_LIST
};

struct config
{
    int mode;
    int cols;
    int max_name;
};

struct serv
{
    int si;
    int is_s6;
    s6_svstatus_t st6;
    tain_t stamp;
};

enum
{
    FILTER_NONE = 0,
    /* status */
    FILTER_UP,
    FILTER_DOWN,
    FILTER_ERROR,
    /* type */
    FILTER_ONESHOT,
    FILTER_LONGRUN,
    FILTER_LOGGED,
    FILTER_LOG,
    FILTER_ALL
};

enum
{
    SORT_ASC,
    SORT_DESC
};

static genalloc ga_serv = GENALLOC_ZERO;

static unsigned int filter_type = FILTER_NONE;
static unsigned int filter_status = FILTER_NONE;
static unsigned int sort_order = SORT_ASC;

static int put_s_max (const char *s, int max, int pad);

static void
put_wstat (int wstat, int max, int pad)
{
    char buf[20];

    if (WIFEXITED (wstat))
    {
        byte_copy (buf, 9, "exitcode ");
        buf[9 + uint_fmt (buf + 9, WEXITSTATUS (wstat))] = '\0';
    }
    else
    {
        const char *signame;

        signame = sig_name (WTERMSIG (wstat));
        byte_copy (buf, 10, "signal SIG");
        byte_copy (buf + 10, strlen (signame) + 1, signame);
    }
    put_s_max (buf, max, pad);
}

static void
put_time (tain_t *st_stamp, int strict)
{
    char buf[LOCALTMN_FMT];
    localtmn_t local;
    tain_t stamp;
    int n;

    localtmn_from_tain (&local, st_stamp, 1);

    if (strict)
    {
        localtmn_fmt (buf, &local);
        buf[19] = ' ';
        buf[20] = '\0';
        aa_bs_noflush (AA_OUT, buf);
        return;
    }

    aa_bb_noflush (AA_OUT, buf, localtmn_fmt (buf, &local));
    aa_bs_noflush (AA_OUT, " (");
    tain_sub (&stamp, &STAMP, st_stamp);
    if (stamp.sec.x > 86400)
    {
        n = stamp.sec.x / 86400;
        stamp.sec.x -= 86400 * n;

        buf[uint_fmt (buf, n)] = '\0';
        aa_bs_noflush (AA_OUT, buf);
        aa_bs_noflush (AA_OUT, "d ");
    }
    if (stamp.sec.x > 3600)
    {
        n = stamp.sec.x / 3600;
        stamp.sec.x -= 3600 * n;

        buf[uint_fmt (buf, n)] = '\0';
        aa_bs_noflush (AA_OUT, buf);
        aa_bs_noflush (AA_OUT, "h ");
    }
    if (stamp.sec.x > 60)
    {
        n = stamp.sec.x / 60;
        stamp.sec.x -= 60 * n;

        buf[uint_fmt (buf, n)] = '\0';
        aa_bs_noflush (AA_OUT, buf);
        aa_bs_noflush (AA_OUT, "m ");
    }
    buf[uint_fmt (buf, stamp.sec.x)] = '\0';
    aa_bs_noflush (AA_OUT, buf);
    aa_bs_noflush (AA_OUT, "s ago)\n");
}

static struct col
{
    const char *title;
    int len;
} cols[4] = {
    { .title = "Service",   .len = 8 },
    { .title = "Type",      .len = 8 },
    { .title = "Since",     .len = 20 },
    { .title = "Status",    .len = 15 }
};

static inline void
pad_with (int left)
{
    for ( ; left > 0; left -= 10)
        aa_bb_noflush (AA_OUT, "          ", (left >= 10) ? 10 : left);
}

static inline void
pad_col (int i, int done)
{
    if (cols[i].len && cols[i].len > done)
        pad_with (cols[i].len - done);
}

static int
put_list_header (struct config *cfg)
{
    int best;
    int i;

    if (cfg->max_name + 1 > cols[0].len)
        best = cfg->max_name + 1;
    else
        best = cols[0].len;

    if (cfg->cols < 0)
    {
        cols[0].len = best;
        cols[3].len = 0;
    }
    else
    {
        int len;

        /* try with the best width */
        len = best + cols[1].len + cols[2].len + cols[3].len;
        if (cfg->cols >= len)
            cols[0].len = best;
        else
        {
            int added;
            int n;

            added = best - cols[0].len;
            n = len - cfg->cols;
            /* can we remove some from col0 to get col3 at its min size? */
            if (added >= n)
            {
                len -= n;
                cols[0].len = best - n;
            }
            else
            {
                /* try all min sizes */
                len = cols[0].len + cols[1].len + cols[2].len + cols[3].len;
                if (cfg->cols < len)
                {
                    /* try without col1 */
                    len -= cols[1].len;
                    if (len < len)
                    {
                        aa_strerr_warn1x ("Terminal too small, disabling list mode");
                        cfg->mode = MODE_NORMAL;
                        return 0;
                    }
                    cols[1].len = 0;
                }
            }
        }

        cols[3].len += cfg->cols - len;
    }

    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    for (i = 0; i < 4; ++i)
    {
        if (i < 3 && cols[i].len == 0)
            continue;

        aa_bs_noflush (AA_OUT, cols[i].title);
        if (i < 3)
            pad_col (i, strlen (cols[i].title));
    }
    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
    aa_bs_flush (AA_OUT, "\n");

    return 1;
}

static int
put_s_max (const char *s, int max, int pad)
{
    int l = strlen (s);

    if (max <= 4)
        return 0;
    else if (l >= max)
    {
        aa_bb_noflush (AA_OUT, s, max - 4);
        aa_bs_noflush (AA_OUT, "... ");
    }
    else
    {
        aa_bs_noflush (AA_OUT, s);
        if (!pad)
            return l;
        pad_col (0, l);
    }
    return max;
}

#define put_s(s)                        \
    if (cfg->mode == MODE_LIST)         \
        max -= put_s_max (s, max, 0);   \
    else                                \
        aa_bs_noflush (AA_OUT, s);

static void
status_service (struct serv *serv, struct config *cfg)
{
    static int first = 1;
    aa_service *s = aa_service (serv->si);
    const char *msg;

    if (cfg->mode == MODE_DRY_LIST)
    {
        aa_bs_noflush (AA_OUT, aa_service_name (s));
        aa_bs_flush (AA_OUT, "\n");
        return;
    }
    else if (cfg->mode == MODE_LIST)
    {
        if (first && !put_list_header (cfg))
            aa_bs_noflush (AA_OUT, "\n");
    }
    else if (!first)
        aa_bs_noflush (AA_OUT, "\n");

    if (cfg->mode == MODE_LIST)
    {
        put_s_max (aa_service_name (s), cols[0].len, 1);

        if (cols[1].len)
        {
            if (s->st.type == AA_TYPE_ONESHOT)
                aa_bs_noflush (AA_OUT, "oneshot");
            else
                aa_bs_noflush (AA_OUT, "longrun");
            pad_col (1, 7);
        }

        if (!serv->is_s6 && s->st.event == AA_EVT_NONE)
        {
            aa_bs_noflush (AA_OUT, "-");
            pad_col (2, 1);
        }
        else
            put_time (&serv->stamp, 1);
    }
    else
    {
        aa_bs_noflush (AA_OUT, "Service: ");
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
        aa_bs_noflush (AA_OUT, aa_service_name (s));
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
        if (s->st.type == AA_TYPE_ONESHOT)
            aa_bs_noflush (AA_OUT, " (one-shot)");
        else
            aa_bs_noflush (AA_OUT, " (long-run)");
        aa_bs_noflush (AA_OUT, "\n");

        aa_bs_noflush (AA_OUT, "Since:   ");
        if (!serv->is_s6 && s->st.event == AA_EVT_NONE)
            aa_bs_noflush (AA_OUT, "-\n");
        else
            put_time (&serv->stamp, 0);

        if (serv->is_s6 && serv->st6.flagready)
        {
            aa_bs_noflush (AA_OUT, "Ready:   ");
            put_time (&serv->st6.readystamp, 0);
        }

        aa_bs_noflush (AA_OUT, "Status:  ");
    }

    if (serv->is_s6)
    {
        int max = cols[3].len;

        if (serv->st6.pid && !serv->st6.flagfinishing)
        {
            char buf[UINT_FMT];

            aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_GREEN_ON);
            put_s ("Up");
            if (serv->st6.flagready)
            {
                put_s (" & Ready");
            }
            aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
            put_s (" (PID ");
            buf[uint_fmt (buf, serv->st6.pid)] = '\0';
            put_s (buf);
            if (cfg->mode == MODE_LIST && !serv->st6.flagwant)
            {
                put_s ("; Once");
            }
            put_s (")");

            if (cfg->mode != MODE_LIST)
            {
                aa_bs_noflush (AA_OUT, "\nMode:    ");
                if (serv->st6.flagwant)
                {
                    put_s ("Automatic restart (want up)");
                }
                else
                {
                    put_s ("Once (want down)");
                }
            }
        }
        else
        {
            aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
            put_s ("Down");
            if (serv->st6.flagready)
            {
                put_s (" & Ready");
            }
            aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
            put_s (" (");
            put_wstat (serv->st6.wstat, max, 0);
            put_s (")");
        }
    }
    else
    {
        int max = cols[3].len;

        switch (s->st.event)
        {
            case AA_EVT_NONE:
                put_s (eventmsg[s->st.event]);
                break;

            case AA_EVT_ERROR:
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_RED_ON);
                put_s (eventmsg[s->st.event]);
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
                put_s (": ");
                put_s (errmsg[s->st.code]);
                msg = aa_service_status_get_msg (&s->st);
                if (msg && *msg)
                {
                    put_s (": ");
                    put_s (msg);
                }
                break;

            case AA_EVT_STARTING:
            case AA_EVT_STOPPING:
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_BLUE_ON);
                put_s (eventmsg[s->st.event]);
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
                break;

            case AA_EVT_STARTING_FAILED:
            case AA_EVT_STOPPING_FAILED:
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_RED_ON);
                put_s (eventmsg[s->st.event]);
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
                if (cfg->mode == MODE_LIST && max <= 6)
                {
                    if (max > 1)
                        aa_bb_noflush (AA_OUT, "...", (max > 4) ? 3 : max - 1);
                    aa_bs_noflush (AA_OUT, " ");
                }
                else
                {
                    put_s (": ");
                    put_s (errmsg[s->st.code]);
                    msg = aa_service_status_get_msg (&s->st);
                    if (msg && *msg)
                    {
                        put_s (": ");
                        put_s (msg);
                    }
                }
                break;

            case AA_EVT_START_FAILED:
            case AA_EVT_STOP_FAILED:
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_RED_ON);
                put_s (eventmsg[s->st.event]);
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
                if (cfg->mode == MODE_LIST && max <= 6)
                {
                    if (max > 1)
                        aa_bb_noflush (AA_OUT, "...", (max > 4) ? 3 : max - 1);
                    aa_bs_noflush (AA_OUT, " ");
                }
                else
                {
                    put_s (": ");
                    put_wstat (s->st.code, max, 0);
                }
                break;

            case AA_EVT_STARTED:
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_GREEN_ON);
                put_s (eventmsg[s->st.event]);
                aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
                break;

            case AA_EVT_STOPPED:
                put_s (eventmsg[s->st.event]);
                break;

            case _AA_NB_EVT: /* silence warning */
                break;
        }
    }
    aa_bs_flush (AA_OUT, "\n");

    if (first)
        first = 0;
}

static int
match_status (struct serv *serv, unsigned int filter)
{
    unsigned int current;

    if (serv->is_s6)
        current = (serv->st6.pid && !serv->st6.flagfinishing) ? FILTER_UP : FILTER_DOWN;
    else
        switch (aa_service (serv->si)->st.event)
        {
            case AA_EVT_STARTED:
            case AA_EVT_STOPPING:
                current = FILTER_UP;
                break;

            case AA_EVT_NONE:
            case AA_EVT_STOPPED:
            case AA_EVT_STARTING:
                current = FILTER_DOWN;
                break;

            case AA_EVT_ERROR: /* == ERR_DEPEND */
            case AA_EVT_STARTING_FAILED:
            case AA_EVT_START_FAILED:
                return filter == FILTER_ERROR || filter == FILTER_DOWN;

            case AA_EVT_STOPPING_FAILED:
            case AA_EVT_STOP_FAILED:
                return filter == FILTER_ERROR || filter == FILTER_UP;

            /* silence warnings */
            case _AA_NB_EVT:
            default:
                current = FILTER_NONE;
                break;
        }

    return current == filter;
}

static inline unsigned int
filter_to_type (unsigned int filter)
{
    switch (filter)
    {
        case FILTER_ONESHOT:
            return AA_TYPE_ONESHOT;

        case FILTER_LONGRUN:
        case FILTER_LOGGED:
        case FILTER_LOG:
            return AA_TYPE_LONGRUN;

        default:
            return AA_TYPE_UNKNOWN;
    }
}

static int
load_service (const char *name, struct config *cfg)
{
    aa_service *s;
    struct serv serv = { 0, };
    int r;

    r = aa_get_service (name, &serv.si, 1);
    if (r < 0)
    {
        aa_put_err (name, errmsg[-r], 1);
        return -1;
    }

    r = aa_preload_service (serv.si);
    if (r < 0)
    {
        aa_put_err (name, errmsg[-r], 1);
        return -1;
    }

    s = aa_service (serv.si);
    if (aa_service_status_read (&s->st, aa_service_name (s)) < 0 && errno != ENOENT)
    {
        int e = errno;

        aa_put_err (name, "Failed to read service status file: ", 0);
        aa_bs_noflush (AA_ERR, error_str (e));
        aa_end_err ();
        return -1;
    }

    if (filter_type != FILTER_NONE && filter_type != FILTER_ALL
            && s->st.type != filter_to_type (filter_type))
        return -1;
    if (filter_type == FILTER_LOG && aa_service_name (s)[strlen (aa_service_name (s)) - 4] != '/')
        return -1;

    if (s->st.type == AA_TYPE_LONGRUN)
    {
        if (!s6_svstatus_read (name, &serv.st6))
        {
            if (errno != ENOENT)
            {
                int e = errno;

                aa_put_err (name, "Unable to read s6 status: ", 0);
                aa_bs_noflush (AA_ERR, error_str (e));
                aa_end_err ();
                return -1;
            }
        }
        else if (tain_less (&s->st.stamp, &serv.st6.stamp))
        {
            serv.is_s6 = 1;
            if (cfg->mode == MODE_LIST && serv.st6.flagready)
                serv.stamp = serv.st6.readystamp;
            else
                serv.stamp = serv.st6.stamp;
        }
    }
    if (!serv.is_s6)
        serv.stamp = s->st.stamp;

    if (filter_status != FILTER_NONE && !match_status (&serv, filter_status))
        return -1;

    if (cfg->mode == MODE_LIST)
    {
        int l = strlen (name);

        if (l > cfg->max_name)
            cfg->max_name = l;
    }

    genalloc_append (struct serv, &ga_serv, &serv);
    return serv.si;
}

static int
it_all (direntry *d, void *data)
{
    int si;

    if (*d->d_name == '.' || d->d_type != DT_DIR)
        return 0;

    si = load_service (d->d_name, data);
    if ((filter_type == FILTER_ALL || filter_type == FILTER_LOG
                || filter_type == FILTER_LOGGED)
            /* si < 0 could be a error, or just that it was filtered out */
            && (si < 0 || aa_service (si)->st.type == AA_TYPE_LONGRUN))
    {
        int l = satmp.len;
        int ln = strlen (d->d_name);
        int r;

        /* is this not a logger already? */
        if (ln < 5 || d->d_name[ln - 4] != '/')
        {
            stralloc_cats (&satmp, d->d_name);
            stralloc_catb (&satmp, "/log/run", strlen ("/log/run") + 1);
            r = access (satmp.s + l, F_OK);
            if (r < 0 && (errno != ENOTDIR && errno != ENOENT))
                aa_strerr_diefu3sys (ERR_IO, "load service: access(", satmp.s + l, ")");
            else if (r == 0)
            {
                satmp.s[satmp.len - 5] = '\0';
                load_service (satmp.s + l, data);
            }
        }
        satmp.len = l;
    }

    return 0;
}

static int
it_listdir (direntry *d, void *data)
{
    if (*d->d_name == '.')
        return 0;
    load_service (d->d_name, data);
    return 0;
}

static int
set_filter (const char *filter)
{
    /* type */
    if (str_equal (filter, "os") || str_equal (filter, "oneshot"))
        filter_type = FILTER_ONESHOT;
    else if (str_equal (filter, "lr") || str_equal (filter, "longrun"))
        filter_type = FILTER_LONGRUN;
    else if (str_equal (filter, "all") || str_equal (filter, "os+lr+log"))
        filter_type = FILTER_ALL;
    else if (str_equal (filter, "log"))
        filter_type = FILTER_LOG;
    else if (str_equal (filter, "logged") || str_equal (filter, "lr+log"))
        filter_type = FILTER_LOGGED;
    /* status */
    else if (str_equal (filter, "up") || str_equal (filter, "start")
            || str_equal (filter, "started"))
        filter_status = FILTER_UP;
    else if (str_equal (filter, "down") || str_equal (filter, "stop")
            || str_equal (filter, "stopped"))
        filter_status = FILTER_DOWN;
    else if (str_equal (filter, "error") || str_equal (filter, "fail")
            || str_equal (filter, "failed"))
        filter_status = FILTER_ERROR;
    else
    {
        errno = EINVAL;
        return 0;
    }

    return 1;
}

static int
cmp_serv_name (const void *_serv1, const void *_serv2)
{
    const struct serv *serv1 = _serv1;
    const struct serv *serv2 = _serv2;
    int r;

    r = strcmp (aa_service_name (aa_service (serv1->si)),
            aa_service_name (aa_service (serv2->si)));
    return (sort_order == SORT_ASC) ? r : -r;
}

static int
cmp_serv_stamp (const void *_serv1, const void *_serv2)
{
    const struct serv *serv1 = _serv1;
    const struct serv *serv2 = _serv2;
    int r;

    if (!serv1->is_s6 && aa_service (serv1->si)->st.event == AA_EVT_NONE)
        r = (!serv2->is_s6 && aa_service (serv2->si)->st.event == AA_EVT_NONE) ? 0 : -1;
    else if (!serv2->is_s6 && aa_service (serv2->si)->st.event == AA_EVT_NONE)
        r = 1;
    else
        r = (tain_less (&serv1->stamp, &serv2->stamp) != 0) ? -1 : 1;

    return (sort_order == SORT_ASC) ? r : -r;
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] [service...]",
            " -D, --double-output           Enable double-output mode\n"
            " -r, --repodir DIR             Use DIR as repository directory\n"
            " -l, --listdir DIR             Use DIR to list services to get status of\n"
            " -a, --all                     Show status of all services\n"
            " -f, --filter FILTER           Only process services matching FILTER, one of:\n"
            "   oneshot, longrun, log, logged, all, up, down, error   (see aa-status(1) for more)\n"
            " -s, --sort SORT               Sort by SORT, one of: none, name, time (default)\n"
            " -R, --reverse                 Reverse sort order\n"
            " -N, --name                    Sort by name\n"
            " -L, --list                    Show statuses as one-liners list\n"
            " -n, --dry-list                Only show service names\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-status";
    const char *path_repo = "/run/services";
    const char *path_list = NULL;
    struct config cfg = { 0, };
    int (*sort_fn) (const void *, const void *) = cmp_serv_stamp;
    int all = 0;
    int i;
    int r;

    for (;;)
    {
        struct option longopts[] = {
            { "all",                no_argument,        NULL,   'a' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "filter",             required_argument,  NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "listdir",            required_argument,  NULL,   'l' },
            { "list",               no_argument,        NULL,   'L' },
            { "name",               no_argument,        NULL,   'N' },
            { "dry-list",           no_argument,        NULL,   'n' },
            { "reverse",            no_argument,        NULL,   'R' },
            { "repodir",            required_argument,  NULL,   'r' },
            { "sort",               required_argument,  NULL,   's' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "aDf:hl:LNnRr:s:V", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'a':
                all = 1;
                break;

            case 'D':
                aa_set_double_output (1);
                break;

            case 'f':
                if (!set_filter (optarg))
                    aa_strerr_diefu3sys (1, "set filter '", optarg, "'");
                break;

            case 'h':
                dieusage (0);

            case 'l':
                unslash (optarg);
                path_list = optarg;
                break;

            case 'L':
                cfg.mode = MODE_LIST;
                break;

            case 'N':
                sort_fn = cmp_serv_name;
                break;

            case 'n':
                cfg.mode = MODE_DRY_LIST;
                break;

            case 'R':
                sort_order = SORT_DESC;
                break;

            case 'r':
                unslash (optarg);
                path_repo = optarg;
                break;

            case 's':
                if (str_equal (optarg, "none"))
                    sort_fn = NULL;
                else if (str_equal (optarg, "name"))
                    sort_fn = cmp_serv_name;
                else if (str_equal (optarg, "time"))
                    sort_fn = cmp_serv_stamp;
                else
                {
                    errno = EINVAL;
                    aa_strerr_diefu3sys (1, "set sort order '", optarg, "'");
                }
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (!all && !path_list && argc < 1)
        dieusage (1);

    r = aa_init_repo (path_repo, AA_REPO_READ);
    if (r < 0)
        aa_strerr_diefu2sys (2, "init repository ", path_repo);

    if (cfg.mode == MODE_LIST)
    {
        struct winsize win;

        if (isatty (1))
        {
            if (ioctl (1, TIOCGWINSZ, &win) == 0)
                cfg.cols = win.ws_col;
            else
                cfg.cols = 80;
        }
        else
            cfg.cols = 999999;
    }

    if (all)
    {
        stralloc sa = STRALLOC_ZERO;

        stralloc_catb (&sa, ".", 2);
        r = aa_scan_dir (&sa, 0, it_all, &cfg);
        if (r < 0)
            aa_strerr_diefu2sys (-r, "scan repo directory ", path_repo);
    }
    else if (path_list)
    {
        stralloc sa = STRALLOC_ZERO;
        int r;

        if (*path_list != '/' && *path_list != '.')
            stralloc_cats (&sa, LISTDIR_PREFIX);
        stralloc_catb (&sa, path_list, strlen (path_list) + 1);
        r = aa_scan_dir (&sa, 1, it_listdir, &cfg);
        stralloc_free (&sa);
        if (r < 0)
            aa_strerr_diefu3sys (-r, "read list directory ",
                    (*path_list != '/' && *path_list != '.') ? LISTDIR_PREFIX : path_list,
                    (*path_list != '/' && *path_list != '.') ? path_list : "");
    }
    else
        for (i = 0; i < argc; ++i)
            if (str_equal (argv[i], "-"))
            {
                if (process_names_from_stdin ((names_cb) load_service, &cfg) < 0)
                    aa_strerr_diefu1sys (ERR_IO, "process names from stdin");
            }
            else
                load_service (argv[i], &cfg);

    if (sort_fn)
        qsort (genalloc_s(struct serv, &ga_serv), genalloc_len (struct serv, &ga_serv),
                sizeof (struct serv), sort_fn);

    for (i = 0; i < genalloc_len (struct serv, &ga_serv); ++i)
        status_service (&genalloc_s (struct serv, &ga_serv)[i], &cfg);

    return 0;
}
