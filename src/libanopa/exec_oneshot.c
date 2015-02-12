
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/djbunix.h>
#include <skalibs/selfpipe.h>
#include <skalibs/bytestr.h>
#include <skalibs/tai.h>
#include <skalibs/uint32.h>
#include <skalibs/error.h>
#include <skalibs/strerr2.h>
#include <anopa/service.h>
#include <anopa/err.h>
#include "service_internal.h"

int
_exec_oneshot (int si, aa_mode mode)
{
    aa_service *s = aa_service (si);
    int is_start = (mode == AA_MODE_START) ? 1 : 0;
    char * const filename = (is_start) ? AA_START_FILENAME : AA_STOP_FILENAME;
    int l_fn = sizeof ((is_start) ? AA_START_FILENAME : AA_STOP_FILENAME);
    int l_sn = strlen (aa_service_name (s));
    char buf[l_sn + 1 +l_fn];
    struct stat st;
    const char *_err;
    int _errno;
    int p_int[2];
    int p_in[2];
    int p_out[2];
    int p_prg[2];
    pid_t pid;
    char c;

    byte_copy (buf, l_sn, aa_service_name (s));
    byte_copy (buf + l_sn, 1, "/");
    byte_copy (buf + l_sn + 1, l_fn, filename);

    if (stat (buf, &st) < 0)
    {
        tain_now_g ();

        if (errno != ENOENT)
        {
            _errno = errno;
            _err = "stat ";
            goto err;
        }

        /* nothing to do, just set it done */
        s->st.event = (is_start) ? AA_EVT_STARTED : AA_EVT_STOPPED;
        tain_copynow (&s->st.stamp);
        if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
            strerr_warnwu2sys ("write service status file for ", aa_service_name (s));

        if (_exec_cb)
            _exec_cb (si, s->st.event, 0);

        /* this was not a failure, but we return -1 to trigger a
         * aa_scan_mainlist() anyways, since the service changed state */
        return -1;
    }

    s->st.event = (is_start) ? AA_EVT_STARTING : AA_EVT_STOPPING;
    tain_copynow (&s->st.stamp);
    aa_service_status_set_msg (&s->st, "");
    if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
    {
        s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
        s->st.code = ERR_WRITE_STATUS;
        aa_service_status_set_msg (&s->st, error_str (errno));

        if (_exec_cb)
            _exec_cb (si, s->st.event, 0);
        return -1;
    }

    if (pipecoe (p_int) < 0)
    {
        _errno = errno;
        _err = "set up pipes";
        goto err;
    }
    if (pipenb (p_in) < 0)
    {
        _errno = errno;
        _err = "set up pipes";

        fd_close (p_int[0]);
        fd_close (p_int[1]);

        goto err;
    }
    else
    {
        int flags = fcntl (p_in[0], F_GETFL, 0);
        if (flags < 0 || fcntl (p_in[0], F_SETFL, flags & ~O_NONBLOCK) < 0)
        {
            _errno = errno;
            _err = "set up pipes";

            fd_close (p_int[0]);
            fd_close (p_int[1]);
            fd_close (p_in[0]);
            fd_close (p_in[1]);

            goto err;
        }
    }
    if (pipenb (p_out) < 0)
    {
        _errno = errno;
        _err = "set up pipes";

        fd_close (p_int[0]);
        fd_close (p_int[1]);
        fd_close (p_in[0]);
        fd_close (p_in[1]);

        goto err;
    }
    if (pipenb (p_prg) < 0)
    {
        _errno = errno;
        _err = "set up pipes";

        fd_close (p_int[0]);
        fd_close (p_int[1]);
        fd_close (p_in[0]);
        fd_close (p_in[1]);
        fd_close (p_out[0]);
        fd_close (p_out[1]);

        goto err;
    }

    pid = fork ();
    if (pid < 0)
    {
        _errno = errno;
        _err = "fork";

        fd_close (p_int[1]);
        fd_close (p_int[0]);
        fd_close (p_in[0]);
        fd_close (p_in[1]);
        fd_close (p_out[1]);
        fd_close (p_out[0]);
        fd_close (p_prg[1]);
        fd_close (p_prg[0]);

        goto err;
    }
    else if (pid == 0)
    {
        char * const argv[] = { filename, NULL };
        PROG = aa_service_name (s);
        char buf_e[UINT32_FMT];
        uint32 e;

        selfpipe_finish ();
        fd_close (p_int[0]);
        fd_close (p_in[1]);
        fd_close (p_out[0]);
        fd_close (p_prg[0]);
        fd_close (0);
        fd_close (1);
        fd_close (2);
        if (fd_move (0, p_in[0]) < 0 || fd_move (1, p_out[1]) < 0
                || fd_copy (2, 1) < 0 || fd_move (3, p_prg[1]) < 0)
        {
            e = (uint32) errno;
            fd_write (p_int[1], "p", 1);
            uint32_pack (buf_e, e);
            fd_write (p_int[1], buf_e, UINT32_FMT);
            strerr_diefu1sys (ERR_IO, "set up pipes");
        }

        if (chdir (PROG) < 0)
        {
            e = (uint32) errno;
            fd_write (p_int[1], "c", 1);
            uint32_pack (buf_e, e);
            fd_write (p_int[1], buf_e, UINT32_FMT);
            strerr_diefu1sys (ERR_IO, "get into service directory");
        }

        buf[l_sn - 1] = '.';
        execv (buf + l_sn - 1, argv);
        /* if it fails... */
        e = (uint32) errno;
        fd_write (p_int[1], "e", 1);
        uint32_pack (buf_e, e);
        fd_write (p_int[1], buf_e, UINT32_FMT);
        strerr_dieexec (ERR_IO, filename);
    }

    fd_close (p_int[1]);
    fd_close (p_in[0]);
    fd_close (p_out[1]);
    fd_close (p_prg[1]);
    switch (fd_read (p_int[0], &c, 1))
    {
        case 0:     /* it worked */
            {
                fd_close (p_int[0]);
                s->fd_in = p_in[1];
                s->fd_out = p_out[0];
                s->fd_progress = p_prg[0];

                tain_now_g ();

                if (_exec_cb)
                    _exec_cb (si, s->st.event, pid);
                return 0;
            }

        case 1:     /* child failed to exec */
            {
                char msg[l_fn + 260];
                char buf[UINT32_FMT];
                uint32 e = 0;
                int p = 0;
                int l;

                tain_now_g ();

                if (fd_read (p_int[0], buf, UINT32_FMT) == UINT32_FMT)
                    uint32_unpack (buf, &e);
                fd_close (p_int[0]);
                fd_close (p_in[1]);
                fd_close (p_out[0]);
                fd_close (p_prg[0]);

                if (c == 'e')
                {
                    s->st.code = ERR_EXEC;
                    byte_copy (msg, 1, " ");
                    byte_copy (msg + 1, l_fn, filename);
                    p += 1 + l_fn;
                }
                else if (c == 'p')
                    s->st.code = ERR_PIPES;
                else /* 'c' */
                    s->st.code = ERR_CHDIR;

                if (e > 0)
                {
                    if (c == 'e')
                    {
                        l = 2;
                        byte_copy (msg + p, l, ": ");
                        p += l;
                    }
                    l = strlen (error_str (e));
                    if (p + l >= 260)
                        l = 260 - p - 1;
                    byte_copy (msg + p, l, error_str (e));
                    p += l;
                }
                byte_copy (msg + p, 1, "");

                s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
                tain_copynow (&s->st.stamp);
                aa_service_status_set_msg (&s->st, msg);
                if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
                    strerr_warnwu2sys ("write service status file for ", aa_service_name (s));

                if (_exec_cb)
                    _exec_cb (si, s->st.event, 0);
                return -1;
            }

        case -1:    /* internal failure */
            _errno = errno;
            _err = "read pipe";

            fd_close (p_int[0]);
            fd_close (p_in[1]);
            fd_close (p_out[0]);
            fd_close (p_prg[0]);
    }

err:
    tain_now_g ();
    s->st.event = (is_start) ? AA_EVT_STARTING_FAILED : AA_EVT_STOPPING_FAILED;
    s->st.code = ERR_IO;
    tain_copynow (&s->st.stamp);
    {
        int l_ft  = strlen ("Failed to ");
        int l_err = strlen (_err);
        int l_buf = strlen (buf);
        const char *errstr = error_str (_errno);
        int l_es = strlen (errstr);
        char msg[l_ft + l_err + l_buf + 2 + l_es + 1];

        byte_copy (msg, l_ft, "Failed to ");
        byte_copy (msg + l_ft, l_err, _err);
        if (*_err == 's') /* stat */
            byte_copy (msg + l_ft + l_err, l_buf, buf);
        else
            l_buf = 0;
        byte_copy (msg + l_ft + l_err + l_buf, 2, ": ");
        byte_copy (msg + l_ft + l_err + l_buf + 2, l_es + 1, errstr);
        aa_service_status_set_msg (&s->st, msg);
    }
    if (aa_service_status_write (&s->st, aa_service_name (s)) < 0)
        strerr_warnwu2sys ("write service status file for ", aa_service_name (s));

    if (_exec_cb)
        _exec_cb (si, s->st.event, 0);
    return -1;
}
