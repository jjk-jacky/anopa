/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-stop.c
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

#define _BSD_SOURCE

#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/direntry.h>
#include <skalibs/genalloc.h>
#include <skalibs/skamisc.h>
#include <skalibs/types.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <s6/s6-supervise.h>
#include <anopa/common.h>
#include <anopa/err.h>
#include <anopa/get_repodir.h>
#include <anopa/init_repo.h>
#include <anopa/output.h>
#include <anopa/scan_dir.h>
#include <anopa/ga_int_list.h>
#include <anopa/service_status.h>
#include <anopa/service.h>
#include <anopa/progress.h>
#include <anopa/stats.h>
#include "start-stop.h"
#include "util.h"
#include "common.h"


static genalloc ga_unknown = GENALLOC_ZERO;
static genalloc ga_depend = GENALLOC_ZERO;
static genalloc ga_io = GENALLOC_ZERO;
static aa_mode mode = AA_MODE_STOP;
static int verbose = 0;
static int rc = 0;
static const char *skip = NULL;

void
check_essential (int si)
{
    /* required by start-stop.c; only used by aa-start.c */
}

static void
autoload_cb (int si, aa_al al, const char *name, int err)
{
    int aa = (mode & AA_MODE_IS_DRY) ? AA_ERR : AA_OUT;

    aa_bs (aa, "auto-add: ");
    aa_bs (aa, aa_service_name (aa_service (si)));
    aa_bs (aa, " needs ");
    aa_bs (aa, name);
    aa_bs_flush (aa, "\n");
}

static int
preload_service (const char *name)
{
    int si = -1;
    int type;
    int r;

    type = aa_get_service (name, &si, 0);
    if (type < 0)
        r = type;
    else
        r = aa_ensure_service_loaded (si, AA_MODE_STOP, 0, (verbose) ? autoload_cb : NULL);
    if (r < 0)
    {
        /* there shouldn't be much errors possible here... basically only ERR_IO
         * or ERR_NOT_UP should be possible, and the later should be silently
         * ignored... so: */
        if (r == -ERR_IO)
        {
            /* ERR_IO from aa_get_service() means we don't have a si (it is
             * actually set to the return value); but from aa_mark_service()
             * (e.g. to read "needs") then we do */
            if (si < 0)
                put_err_service (name, ERR_IO, 1);
            else
            {
                int e = errno;

                put_err_service (name, ERR_IO, 0);
                add_err (": ");
                add_err (strerror (e));
                end_err ();
            }
        }
    }

    /* r < 0 can mean different things (e.g. ERR_NOT_UP), including that we
     * don't have a type set. Hence we only rely on it when possible, but we
     * need to always check for a logger */
    if (r < 0 || aa_service (si)->st.type == AA_TYPE_LONGRUN)
    {
        size_t l = satmp.len;

        /* for longruns, even though the dependency of the logger is auto-added,
         * we still need to ensure the service is loaded */
        stralloc_cats (&satmp, name);
        /* is this not a logger already? */
        if (satmp.len - l < 5 || satmp.s[satmp.len - 4] != '/')
        {
            stralloc_catb (&satmp, "/log/run", strlen ("/log/run") + 1);
            r = access (satmp.s + l, F_OK);
            if (r < 0 && (errno != ENOTDIR && errno != ENOENT))
                aa_strerr_diefu3sys (ERR_IO, "preload services: access(", satmp.s + l, ")");
            else if (r == 0)
            {
                satmp.s[satmp.len - 5] = '\0';
                preload_service (satmp.s + l);
            }
        }
        satmp.len = l;
    }

    return 0;
}

static int
it_preload (direntry *d, void *data)
{
    if (*d->d_name == '.' || d->d_type != DT_DIR)
        return 0;

    tain_now_g ();
    preload_service (d->d_name);

    return 0;
}

static int
add_service (const char *name, void *data)
{
    int si = -1;
    int type;
    int r;

    if (skip && str_equal (name, skip))
        return 0;

    type = aa_get_service (name, &si, 0);
    if (type < 0)
    {
        r = type;

        /* since everything was preloaded (and so we don't ensure_loaded here),
         * it can only be ERR_UNKNOWN or possibly ERR_IO, nothing else */

        if (r == -ERR_UNKNOWN)
        {
            put_err_service (name, ERR_UNKNOWN, 1);
            add_name_to_ga (name, &ga_unknown);
        }
        else if (r == -ERR_IO)
        {
            /* ERR_IO from aa_get_service() means we don't have a si (it is
             * actually set to the return value); but from aa_mark_service()
             * (e.g. to read "needs") then we do */
            if (si < 0)
            {
                put_err_service (name, ERR_IO, 1);
                add_name_to_ga (name, &ga_io);
            }
            else
            {
                int e = errno;

                put_err_service (name, ERR_IO, 0);
                add_err (": ");
                add_err (strerror (e));
                end_err ();

                add_to_list (&ga_failed, si, 0);
            }
        }
    }
    else if (type == AA_SERVICE_FROM_TMP)
    {
        /* it could be an ERR_NOT_UP, that was on tmp from preload */
        if (aa_service (si)->ls == AA_LOAD_FAIL)
        {
            /* sanity check */
            if (aa_service (si)->st.code != ERR_NOT_UP)
            {
                errno = EINVAL;
                aa_strerr_diefu2sys (ERR_IO, "add service ", name);
            }

            if (!(mode & AA_MODE_IS_DRY))
                put_title (1, name, errmsg[ERR_NOT_UP], 1);
            ++nb_already;
            r = 0;
        }
        else
        {
            size_t i;

            add_to_list (&aa_main_list, si, 0);
            remove_from_list (&aa_tmp_list, si);

            for (i = 0; i < genalloc_len (int, &aa_service (si)->needs); ++i)
            {
                int sni = list_get (&aa_service (si)->needs, i);
                add_service (aa_service_name (aa_service (sni)), NULL);
            }
        }
    }

    return 0;
}

static int
it_stop (direntry *d, void *data)
{
    if (*d->d_name == '.')
        return 0;

    tain_now_g ();
    add_service (d->d_name, NULL);

    return 0;
}

static void
scan_cb (int si, int sni)
{
    put_err_service (aa_service_name (aa_service (si)), ERR_DEPEND, 0);
    add_err (": ");
    add_err (aa_service_name (aa_service (sni)));
    end_err ();
    add_to_list (&ga_depend, si, 0);
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] [service...]",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -r, --repodir DIR             Use DIR as repository directory\n"
            " -l, --listdir DIR             Use DIR to list services to stop\n"
            " -k, --skip SERVICE            Skip (do not stop) SERVICE\n"
            " -t, --timeout SECS            Use SECS seconds as default timeout\n"
            " -a, --all                     Stop all running services\n"
            " -n, --dry-list                Only show service names (don't stop anything)\n"
            " -v, --verbose                 Print auto-added dependencies\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

static void
close_fd (int fd)
{
    close_fd_for (fd, -1);
}

static void
stop_supervise_for (int si)
{
    aa_service *s = aa_service (si);
    size_t l_sn = strlen (aa_service_name (s));
    char dir[l_sn + 1 + sizeof (S6_SUPERVISE_CTLDIR) + 8];
    int r;

    if (s->st.type != AA_TYPE_LONGRUN)
        return;

    aa_bs (AA_OUT, "Stopping s6-supervise for ");
    aa_bs (AA_OUT, aa_service_name (s));
    aa_bs_flush (AA_OUT, "...\n");

    byte_copy (dir, l_sn, aa_service_name (s));
    byte_copy (dir + l_sn, 9 + sizeof (S6_SUPERVISE_CTLDIR), "/" S6_SUPERVISE_CTLDIR "/control");

    r = s6_svc_write (dir, "x", 1);
    if (r < 0)
        aa_strerr_warnu2sys ("stop s6-supervise for ", aa_service_name (s));
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-stop";
    const char *path_repo = aa_get_repodir ();
    const char *path_list = NULL;
    int all = 0;
    int i;

    aa_secs_timeout = DEFAULT_TIMEOUT_SECS;
    for (;;)
    {
        struct option longopts[] = {
            { "all",                no_argument,        NULL,   'a' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "skip",               required_argument,  NULL,   'k' },
            { "listdir",            required_argument,  NULL,   'l' },
            { "dry-list",           no_argument,        NULL,   'n' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "repodir",            required_argument,  NULL,   'r' },
            { "timeout",            required_argument,  NULL,   't' },
            { "version",            no_argument,        NULL,   'V' },
            { "verbose",            no_argument,        NULL,   'v' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "aDhk:l:nO:r:t:Vv", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'a':
                if (all)
                    mode = AA_MODE_STOP_ALL | (mode & AA_MODE_IS_DRY);
                else
                    all = 1;
                break;

            case 'D':
                aa_set_double_output (1);
                break;

            case 'h':
                dieusage (0);

            case 'k':
                skip = optarg;
                break;

            case 'l':
                unslash (optarg);
                path_list = optarg;
                break;

            case 'n':
                mode |= AA_MODE_IS_DRY;
                break;

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'r':
                unslash (optarg);
                path_repo = optarg;
                break;

            case 't':
                if (!uint0_scan (optarg, &aa_secs_timeout))
                    aa_strerr_diefu2sys (ERR_IO, "set default timeout to ", optarg);
                break;

            case 'V':
                aa_die_version ();

            case 'v':
                verbose = 1;
                break;

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    cols = get_cols (1);
    is_utf8 = is_locale_utf8 ();

    if ((all && (path_list || argc > 0)) || (!all && !path_list && argc < 1))
        dieusage (1);

    if ((mode & AA_MODE_STOP_ALL) && aa_secs_timeout == 0)
    {
        aa_strerr_warn1x ("Default timeout cannot be infinite (0) in stop-all mode, ignoring");
        aa_secs_timeout = DEFAULT_TIMEOUT_SECS;
    }

    if (aa_init_repo (path_repo, (mode & AA_MODE_IS_DRY) ? AA_REPO_READ : AA_REPO_WRITE) < 0)
        aa_strerr_diefu2sys (ERR_IO, "init repository ", path_repo);

    /* let's "preload" every services from the repo. This will have everything
     * in tmp list, either LOAD_DONE when up, or LOAD_FAIL when not
     * (ERR_NOT_UP).
     * The idea is to load dependencies, so in case service A needs B, we've
     * added into the "needs" of B the service A, i.e. stopping B means a need
     * to also stop A (as always, an "after" will handle the ordering).
     */
    {
        stralloc sa = STRALLOC_ZERO;
        int r;

        stralloc_catb (&sa, ".", 2);
        r = aa_scan_dir (&sa, 0, it_preload, NULL);
        stralloc_free (&sa);
        if (r < 0)
            aa_strerr_diefu1sys (-r, "read repository directory");
    }

    if (all)
    {
        size_t i;

        /* to stop all (up) services, since we've preloaded everything, simply
         * means moving all services from tmp to main list. We just need to make
         * sure to process "valid" services, since there could be LOAD_FAIL ones
         * (ERR_NOT_UP) that we should simply ignore.
         */
        for (i = 0; i < genalloc_len (int, &aa_tmp_list); )
        {
            int si = list_get (&aa_tmp_list, i);

            if (aa_service (si)->ls == AA_LOAD_DONE)
            {
                if (!skip || !str_equal (aa_service_name (aa_service (si)), skip))
                {
                    add_to_list (&aa_main_list, si, 0);
                    remove_from_list (&aa_tmp_list, si);
                }
                else
                {
                    ++i;
                    if (skip)
                        skip = NULL;

                    if (mode & AA_MODE_STOP_ALL)
                        stop_supervise_for (si);
                }
            }
            else
            {
                ++i;

                if ((mode & AA_MODE_STOP_ALL) && aa_service (si)->st.code == ERR_NOT_UP)
                    stop_supervise_for (si);
            }
        }
    }
    else if (path_list)
    {
        stralloc sa = STRALLOC_ZERO;
        int r;

        if (*path_list != '/' && *path_list != '.')
            stralloc_cats (&sa, LISTDIR_PREFIX);
        stralloc_catb (&sa, path_list, strlen (path_list) + 1);
        r = aa_scan_dir (&sa, 1, it_stop, NULL);
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
                if (process_names_from_stdin ((names_cb) add_service, NULL) < 0)
                    aa_strerr_diefu1sys (ERR_IO, "process names from stdin");
            }
            else
                add_service (argv[i], NULL);

    tain_now_g ();

    mainloop (mode, scan_cb);

    if (!(mode & AA_MODE_IS_DRY))
    {
        aa_bs (AA_OUT, "\n");
        put_title (1, PROG, "Completed.", 1);
        aa_show_stat_nb (nb_already, "Not up", ANSI_HIGHLIGHT_GREEN_ON);
        aa_show_stat_nb (nb_done, "Stopped", ANSI_HIGHLIGHT_GREEN_ON);
        show_stat_service_names (&ga_timedout, "Timed out", ANSI_HIGHLIGHT_RED_ON);
        show_stat_service_names (&ga_failed, "Failed", ANSI_HIGHLIGHT_RED_ON);
        show_stat_service_names (&ga_depend, "Dependency failed", ANSI_HIGHLIGHT_RED_ON);
        aa_show_stat_names (aa_names.s, &ga_io, "I/O error", ANSI_HIGHLIGHT_RED_ON);
        aa_show_stat_names (aa_names.s, &ga_unknown, "Unknown", ANSI_HIGHLIGHT_RED_ON);
    }

    genalloc_free (int, &ga_timedout);
    genalloc_free (int, &ga_failed);
    genalloc_free (int, &ga_depend);
    genalloc_free (size_t, &ga_io);
    genalloc_free (size_t, &ga_unknown);
    genalloc_free (pid_t, &ga_pid);
    genalloc_free (int, &aa_tmp_list);
    genalloc_free (int, &aa_main_list);
    stralloc_free (&aa_names);
    genalloc_deepfree (struct progress, &ga_progress, free_progress);
    aa_free_services (close_fd);
    genalloc_free (iopause_fd, &ga_iop);
    return rc;
}
