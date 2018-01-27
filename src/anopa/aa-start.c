/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-start.c
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

#include <locale.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <langinfo.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/direntry.h>
#include <skalibs/genalloc.h>
#include <skalibs/tai.h>
#include <skalibs/iopause.h>
#include <skalibs/djbunix.h>
#include <skalibs/types.h>
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


#define ESSENTIAL_FILENAME      "essential"

static genalloc ga_depend = GENALLOC_ZERO;
static genalloc ga_skipped = GENALLOC_ZERO;
static genalloc ga_unknown = GENALLOC_ZERO;
static genalloc ga_io = GENALLOC_ZERO;
static aa_mode mode = AA_MODE_START;
static int no_wants = 0;
static int verbose = 0;
static int rc = RC_OK;

void
check_essential (int si)
{
    if (!(rc & RC_ST_ESSENTIAL))
    {
        struct stat st;
        const char *name = aa_service_name (aa_service (si));
        size_t l_name = strlen (name);
        char buf[l_name + 1 + sizeof (ESSENTIAL_FILENAME)];

        byte_copy (buf, l_name, name);
        byte_copy (buf + l_name, 1 + sizeof (ESSENTIAL_FILENAME), "/" ESSENTIAL_FILENAME);

        if (stat (buf, &st) < 0)
        {
            if (errno != ENOENT)
            {
                int e = errno;
                put_warn (name, "Failed to stat " ESSENTIAL_FILENAME ": ", 0);
                add_warn (strerror (e));
                end_warn ();
            }
        }
        else
            rc |= RC_ST_ESSENTIAL;
    }
}

static void
autoload_cb (int si, aa_al al, const char *name, int err)
{
    if (verbose)
    {
        int aa = (mode & AA_MODE_IS_DRY) ? AA_ERR : AA_OUT;

        aa_bs (aa, "auto-add: ");
        aa_bs (aa, aa_service_name (aa_service (si)));
        aa_bs (aa, (al == AA_AUTOLOAD_NEEDS) ? " needs " : " wants ");
        aa_bs (aa, name);
        aa_bs_flush (aa, "\n");
    }

    if (al == AA_AUTOLOAD_WANTS && err > 0)
    {
        put_warn (aa_service_name (aa_service (si)), "Skipping wanted service ", 0);
        add_warn (name);
        add_warn (": ");
        add_warn (errmsg[err]);
        end_warn ();
        add_name_to_ga (name, &ga_skipped);
    }
}

static int
add_service (const char *name, void *data)
{
    int si = -1;
    int type;
    int r;

    type = aa_get_service (name, &si, 1);
    if (type < 0)
        r = type;
    else
        r = aa_mark_service (mode, si, type == AA_SERVICE_FROM_MAIN, no_wants, autoload_cb);
    if (r < 0)
    {
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
                check_essential (si);
            }
        }
        else if (r == -ERR_ALREADY_UP)
        {
            if (!(mode & AA_MODE_IS_DRY))
                put_title (1, name, errmsg[-r], 1);
            ++nb_already;
            r = 0;
        }
        else
        {
            aa_service *s = aa_service (si);
            const char *msg = aa_service_status_get_msg (&s->st);
            int has_msg;

            has_msg = s->st.event == AA_EVT_ERROR && s->st.code == -r && !!msg;
            put_err_service (name, -r, !has_msg);
            if (has_msg)
            {
                add_err (": ");
                add_err (msg);
                end_err ();
            }

            add_to_list ((r == -ERR_DEPEND) ? &ga_depend : &ga_failed, si, 0);
            check_essential (si);
        }
    }
    else
        r = 0;

    return r;
}

static int
it_start (direntry *d, void *data)
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
    check_essential (si);
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION...] [service...]",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -r, --repodir DIR             Use DIR as repository directory\n"
            " -l, --listdir DIR             Use DIR to list services to start\n"
            " -W, --no-wants                Don't auto-start services from 'wants'\n"
            " -t, --timeout SECS            Use SECS seconds as default timeout\n"
            " -n, --dry-list                Only show service names (don't start anything)\n"
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

int
main (int argc, char * const argv[])
{
    PROG = "aa-start";
    stralloc sacwd = STRALLOC_ZERO;
    const char *path_repo = aa_get_repodir ();
    const char *path_list = NULL;
    int i;

    aa_secs_timeout = DEFAULT_TIMEOUT_SECS;
    for (;;)
    {
        struct option longopts[] = {
            { "double-output",      no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "listdir",            required_argument,  NULL,   'l' },
            { "dry-list",           no_argument,        NULL,   'n' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "repodir",            required_argument,  NULL,   'r' },
            { "timeout",            required_argument,  NULL,   't' },
            { "version",            no_argument,        NULL,   'V' },
            { "verbose",            no_argument,        NULL,   'v' },
            { "no-wants",           no_argument,        NULL,   'W' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "Dhl:nO:r:t:VvW", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                aa_set_double_output (1);
                break;

            case 'h':
                dieusage (RC_OK);

            case 'l':
                unslash (optarg);
                /* if relative path (starts with '.') and we don't have cwd yet,
                 * get it now -- i.e. before init_repo() */
                if (*optarg == '.' && sacwd.len == 0 && sagetcwd (&sacwd) < 0)
                    aa_strerr_diefu1sys (RC_FATAL_IO, "get current working directory");
                path_list = optarg;
                break;

            case 'n':
                if (mode & AA_MODE_IS_DRY)
                    mode |= AA_MODE_IS_DRY_FULL;
                else
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
                    aa_strerr_diefu2sys (RC_FATAL_USAGE, "set default timeout to ", optarg);
                break;

            case 'V':
                aa_die_version ();

            case 'v':
                verbose = 1;
                break;

            case 'W':
                no_wants = 1;
                break;

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    cols = get_cols (1);
    is_utf8 = is_locale_utf8 ();

    if (!path_list && argc < 1)
        dieusage (RC_FATAL_USAGE);

    if (aa_init_repo (path_repo, (mode & AA_MODE_IS_DRY) ? AA_REPO_READ : AA_REPO_WRITE) < 0)
        aa_strerr_diefu2sys (RC_FATAL_INIT_REPO, "init repository ", path_repo);

    if (path_list)
    {
        int r;

        /* relative: cwd already there, just add a slash */
        if (*path_list == '.')
            stralloc_cats (&sacwd, "/");
        /* neither relative nor absolute: prefix w/ default listdir path */
        else if (*path_list != '/')
            stralloc_cats (&sacwd, LISTDIR_PREFIX);
        stralloc_catb (&sacwd, path_list, strlen (path_list) + 1);
        r = aa_scan_dir (&sacwd, 1, it_start, NULL);
        if (r < 0)
            aa_strerr_diefu2sys (RC_FATAL_IO, "read list directory ", sacwd.s);
    }

    stralloc_free (&sacwd);
    tain_now_g ();

    for (i = 0; i < argc; ++i)
        if (str_equal (argv[i], "-"))
        {
            if (process_names_from_stdin ((names_cb) add_service, NULL) < 0)
                aa_strerr_diefu1sys (RC_FATAL_IO, "process names from stdin");
        }
        else
            add_service (argv[i], NULL);

    mainloop (mode, scan_cb);

    if (!(mode & AA_MODE_IS_DRY))
    {
        aa_bs (AA_OUT, "\n");
        put_title (1, PROG, "Completed.", 1);
        aa_show_stat_nb (nb_already, "Already up", ANSI_HIGHLIGHT_GREEN_ON);
        aa_show_stat_nb (nb_done, "Started", ANSI_HIGHLIGHT_GREEN_ON);
        show_stat_service_names (&ga_timedout, "Timed out", ANSI_HIGHLIGHT_RED_ON);
        show_stat_service_names (&ga_failed, "Failed", ANSI_HIGHLIGHT_RED_ON);
        show_stat_service_names (&ga_depend, "Dependency failed", ANSI_HIGHLIGHT_RED_ON);
        aa_show_stat_names (aa_names.s, &ga_io, "I/O error", ANSI_HIGHLIGHT_RED_ON);
        aa_show_stat_names (aa_names.s, &ga_unknown, "Unknown", ANSI_HIGHLIGHT_RED_ON);
        aa_show_stat_names (aa_names.s, &ga_skipped, "Skipped", ANSI_HIGHLIGHT_YELLOW_ON);
    }

    if (ga_timedout.len + ga_failed.len + ga_depend.len + ga_io.len > 0)
        rc |= RC_ST_FAILED;
    if (ga_unknown.len > 0)
        rc |= RC_ST_UNKNOWN;
    if (ga_skipped.len > 0)
        rc |= RC_ST_SKIPPED;

    genalloc_free (int, &ga_timedout);
    genalloc_free (int, &ga_failed);
    genalloc_free (int, &ga_depend);
    genalloc_free (size_t, &ga_io);
    genalloc_free (size_t, &ga_unknown);
    genalloc_free (size_t, &ga_skipped);
    genalloc_free (pid_t, &ga_pid);
    genalloc_free (int, &aa_tmp_list);
    genalloc_free (int, &aa_main_list);
    stralloc_free (&aa_names);
    genalloc_deepfree (struct progress, &ga_progress, free_progress);
    aa_free_services (close_fd);
    genalloc_free (iopause_fd, &ga_iop);
    return rc;
}
