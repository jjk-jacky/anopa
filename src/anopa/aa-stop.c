/*
 * anopa - Copyright (C) 2015 Olivier Brunel
 *
 * aa-stop.c
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

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/direntry.h>
#include <skalibs/genalloc.h>
#include <skalibs/strerr2.h>
#include <skalibs/error.h>
#include <skalibs/uint.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <anopa/common.h>
#include <anopa/err.h>
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
static int rc = 0;
static const char *skip = NULL;

void
check_essential (int si)
{
    /* required by start-stop.c; only used by aa-start.c */
}

static int
preload_service (const char *name)
{
    int si = -1;
    int type;
    int r;

    if (skip && str_equal (name, skip))
        return 0;

    type = aa_get_service (name, &si, 0);
    if (type < 0)
        r = type;
    else
        r = aa_ensure_service_loaded (si, AA_MODE_STOP, 0, NULL);
    if (r < 0)
    {
        /* there should be much errors possible here... basically only ERR_IO or
         * ERR_NOT_UP should be possible, and the later should be silently
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
                add_err (error_str (e));
                end_err ();
            }
        }
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
                add_err (error_str (e));
                end_err ();

                genalloc_append (int, &ga_failed, &si);
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
                strerr_diefu1sys (ERR_IO, "add service");
            }

            if (!(mode & AA_MODE_IS_DRY))
                put_title (1, name, errmsg[ERR_NOT_UP], 1);
            ++nb_already;
            r = 0;
        }
        else
        {
            int i;

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
    genalloc_append (int, &ga_depend, &si);
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] [service...]",
            " -D, --double-output           Enable double-output mode\n"
            " -r, --repodir DIR             Use DIR as repository directory\n"
            " -l, --listdir DIR             Use DIR to list services to start\n"
            " -k, --skip SERVICE            Skip (do not stop) SERVICE\n"
            " -t, --timeout SECS            Use SECS seconds as default timeout\n"
            " -a, --all                     Stop all running services\n"
            " -n, --dry-list                Only show service names (don't stop anything)\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

static void
close_fd (int fd)
{
    close_fd_for (fd, -1);
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-stop";
    const char *path_repo = "/run/services";
    const char *path_list = NULL;
    int mode_both = 0;
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
            { "repodir",            required_argument,  NULL,   'r' },
            { "timeout",            required_argument,  NULL,   't' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "aDhk:l:nr:t:V", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'a':
                mode = AA_MODE_STOP_ALL | (mode & AA_MODE_IS_DRY);
                break;

            case 'D':
                mode_both = 1;
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

            case 'r':
                unslash (optarg);
                path_repo = optarg;
                break;

            case 't':
                if (!uint0_scan (optarg, &aa_secs_timeout))
                    strerr_diefu2sys (ERR_IO, "set default timeout to ", optarg);
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    aa_init_output (mode_both);
    cols = get_cols (1);
    is_utf8 = is_locale_utf8 ();

    if ((mode & AA_MODE_STOP_ALL && (path_list || argc > 0))
            || (!(mode & AA_MODE_STOP_ALL) && !path_list && argc < 1))
        dieusage (1);

    if (aa_init_repo (path_repo, AA_REPO_WRITE) < 0)
        strerr_diefu2sys (ERR_IO, "init repository ", path_repo);

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
            strerr_diefu1sys (-r, "read repository directory");
    }

    if (mode & AA_MODE_STOP_ALL)
    {
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
                add_to_list (&aa_main_list, si, 0);
                remove_from_list (&aa_tmp_list, si);
            }
            else
                ++i;
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
            strerr_diefu3sys (-r, "read list directory ",
                    (*path_list != '/' && *path_list != '.') ? LISTDIR_PREFIX : path_list,
                    (*path_list != '/' && *path_list != '.') ? path_list : "");
    }
    else
        for (i = 0; i < argc; ++i)
            if (str_equal (argv[i], "-"))
            {
                if (process_names_from_stdin ((names_cb) add_service, NULL) < 0)
                    strerr_diefu1sys (ERR_IO, "process names from stdin");
            }
            else
                add_service (argv[i], NULL);

    tain_now_g ();

    mainloop (mode, scan_cb);

    if (!(mode & AA_MODE_IS_DRY))
    {
        aa_bs_noflush (AA_OUT, "\n");
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
    genalloc_free (int, &ga_unknown);
    genalloc_free (int, &ga_depend);
    genalloc_free (int, &ga_io);
    genalloc_free (pid_t, &ga_pid);
    genalloc_free (int, &aa_tmp_list);
    genalloc_free (int, &aa_main_list);
    stralloc_free (&aa_names);
    genalloc_deepfree (struct progress, &ga_progress, free_progress);
    aa_free_services (close_fd);
    genalloc_free (iopause_fd, &ga_iop);
    return rc;
}
