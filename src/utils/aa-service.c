/*
 * anopa - Copyright (C) 2015-2017 Olivier Brunel
 *
 * aa-service.c
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

#include <getopt.h>
#include <unistd.h>
#include <skalibs/djbunix.h>
#include <skalibs/env.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/genalloc.h>
#include <skalibs/stralloc.h>
#include <execline/execline.h>
#include <anopa/common.h>
#include <anopa/output.h>

#define RC_ST_FAIL      (1 << 1)

typedef struct exlsn_s exlsn_t;
struct exlsn_s
{
  stralloc vars;
  stralloc values;
  genalloc data; /* array of elsubst */
  stralloc modifs;
};
#define EXLSN_ZERO { .vars = STRALLOC_ZERO, .values = STRALLOC_ZERO, .data = GENALLOC_ZERO, .modifs = STRALLOC_ZERO }


enum
{
    ERR_BAD_KEY = 1,
    ERR_ADDVAR,
    ERR_DIRNAME,
    ERR_NOT_LOG
};

static char islog = 0;

static int
addvar (const char *name, const char *value, exlsn_t *info)
{
    eltransforminfo_t si = ELTRANSFORMINFO_ZERO;
    elsubst_t blah;

    blah.var = info->vars.len;
    blah.value = info->values.len;

    if (el_vardupl (name, info->vars.s, info->vars.len))
        return -ERR_BAD_KEY;
    if (!stralloc_catb (&info->vars, name, strlen (name) + 1))
        return -ERR_ADDVAR;
    if (!stralloc_cats (&info->values, value))
        goto err;

    {
        register int r;

        r = el_transform (&info->values, blah.value, &si);
        if (r < 0)
            goto err;
        blah.n = r ;
    }

    if (!genalloc_append (elsubst_t, &info->data, &blah))
        goto err;

    return 0;

err:
    info->vars.len = blah.var;
    info->values.len = blah.value;
    return -ERR_ADDVAR;
}

static int
aa_service (exlsn_t *info)
{
    stralloc sa = STRALLOC_ZERO;
    char *s;
    size_t len;
    size_t n;
    int r;

    if (sagetcwd (&sa) < 0)
        return -ERR_DIRNAME;

    n = byte_rchr (sa.s, sa.len, '/');
    if (n == sa.len)
    {
        r = -ERR_DIRNAME;
        goto err;
    }
    /* current dirname only */
    s = sa.s + n + 1;

    if (islog)
    {
        if (str_diff (s, "log"))
        {
            r = -ERR_NOT_LOG;
            goto err;
        }

        /* i.e. sa.s = "/log" */
        if (n <= 0)
        {
            r = -ERR_NOT_LOG;
            goto err;
        }

        /* use parent's dirname instead, i.e. the service we're logger of */
        sa.s[n] = '\0';
        n = byte_rchr (sa.s, n - 1, '/');
        s = sa.s + n + 1;

        if (s > sa.s + sa.len)
        {
            r = -ERR_DIRNAME;
            goto err;
        }
    }

    r = addvar ("SERVICE_NAME", s, info);
    if (r < 0)
        goto err;

    len = strlen (s);
    n = byte_chr (s, len, '@');
    if (n < len)
        s[n] = '\0';
    r = addvar ("SERVICE", s, info);
    if (r < 0)
        goto err;

    r = addvar ("INSTANCE", (n < len) ? s + n + 1 : "", info);
    if (r < 0)
        goto err;

err:
    stralloc_free (&sa);
    return r;
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION] PROG...",
            " -D, --double-output           Enable double-output mode\n"
            " -O, --log-file FILE|FD        Write log to FILE|FD\n"
            " -l, --log                     Use parent directory as servicedir\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char const **argv, char const *const *envp)
{
    PROG = "aa_service";
    exlsn_t info = EXLSN_ZERO;
    stralloc sa = STRALLOC_ZERO;
    stralloc dst = STRALLOC_ZERO;
    int r;

    for (;;)
    {
        struct option longopts[] = {
            { "double-output",      no_argument,        NULL,   'D' },
            { "help",               no_argument,        NULL,   'h' },
            { "log",                no_argument,        NULL,   'l' },
            { "log-file",           required_argument,  NULL,   'O' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, (char * const *) argv, "+DhlO:V", longopts, NULL);
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
                islog = 1;
                break;

            case 'O':
                aa_set_log_file_or_die (optarg);
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (RC_FATAL_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    r = aa_service (&info);
    if (r < 0)
        switch (-r)
        {
            case ERR_NOT_LOG:
                aa_strerr_dief1x (RC_FATAL_USAGE, "option --log used while not in a subfolder 'log'");

            case ERR_DIRNAME:
                aa_strerr_diefu1x (RC_FATAL_IO, "get current dirname");

            case ERR_BAD_KEY:
                aa_strerr_dief1x (RC_ST_FAIL, "bad substitution key");

            case ERR_ADDVAR:
                aa_strerr_diefu1sys (RC_ST_FAIL, "complete addvar function");

            default:
                aa_strerr_diefu2x (RC_ST_FAIL, "complete addvar function", ": unknown error");
        }

    if (!env_string (&sa, argv, (unsigned int) argc))
        aa_strerr_diefu1sys (RC_FATAL_MEMORY, "env_string");

    r = el_substitute (&dst, sa.s, sa.len, info.vars.s, info.values.s,
            genalloc_s (elsubst_t const, &info.data),
            genalloc_len (elsubst_t const, &info.data));
    if (r < 0)
        aa_strerr_diefu1sys (RC_ST_FAIL, "el_substitute");
    else if (r == 0)
        _exit (0);

    stralloc_free (&sa);

    {
        char const *v[r + 1];

        if (!env_make (v, r, dst.s, dst.len))
            aa_strerr_diefu1sys (RC_ST_FAIL, "env_make");
        v[r] = 0;
        pathexec_r (v, envp, env_len (envp), info.modifs.s, info.modifs.len);
    }

    aa_strerr_dieexec (RC_FATAL_EXEC, dst.s);
}
