
#include <locale.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <langinfo.h>
#include <errno.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/tai.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/iopause.h>
#include <skalibs/selfpipe.h>
#include <skalibs/sig.h>
#include <skalibs/uint.h>
#include <skalibs/uint16.h>
#include <skalibs/strerr2.h>
#include <anopa/service.h>
#include <anopa/ga_int_list.h>
#include <anopa/output.h>
#include <anopa/err.h>
#include "start-stop.h"

genalloc ga_iop = GENALLOC_ZERO;
genalloc ga_progress = GENALLOC_ZERO;
genalloc ga_pid = GENALLOC_ZERO;
tain_t iol_deadline;
unsigned int draw = 0;
int nb_already = 0;
int nb_done = 0;
int nb_wait_longrun = 0;
genalloc ga_failed = GENALLOC_ZERO;
genalloc ga_timedout = GENALLOC_ZERO;
int cols = 80;
int is_utf8 = 0;
int ioloop = 1;
int si_password = -1;

/* aa-start.c */
void check_essential (int si);

void
free_progress (struct progress *pg)
{
    aa_progress_free (&pg->aa_pg);
}

void
clear_draw ()
{
    if (draw & DRAW_CUR_PASSWORD)
        aa_is_flush (AA_OUT, ANSI_CLEAR_BEFORE ANSI_START_LINE);

    if (draw & DRAW_CUR_WAITING)
    {
        aa_is_flush (AA_OUT, ANSI_CLEAR_BEFORE ANSI_START_LINE);
        draw &= ~DRAW_NEED_WAITING;
    }

    if (draw & DRAW_CUR_PROGRESS)
    {
        int i;

        for (i = 0; i < genalloc_len (struct progress, &ga_progress); ++i)
        {
            struct progress *pg = &genalloc_s (struct progress, &ga_progress)[i];

            if (pg->is_drawn > 0)
            {
                aa_is_flush (AA_OUT, ANSI_PREV_LINE ANSI_CLEAR_AFTER);
                pg->is_drawn = 0;
            }
        }
    }

    draw &= ~DRAW_HAS_CUR;
}

void
draw_progress_for (int si)
{
    struct progress *pg;

    pg = &genalloc_s (struct progress, &ga_progress)[aa_service (si)->pi];
    aa_progress_draw (&pg->aa_pg, aa_service_name (aa_service (si)), cols, is_utf8);
    pg->is_drawn = 1;
    draw |= DRAW_CUR_PROGRESS;
}

void
draw_password ()
{
    aa_service *s = aa_service (si_password);
    struct progress *pg = &genalloc_s (struct progress, &ga_progress)[s->pi];

    if (pg->is_drawn == DRAWN_PASSWORD_READY)
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    aa_is_noflush (AA_OUT, aa_service_name (s));
    if (pg->is_drawn == DRAWN_PASSWORD_READY)
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_OFF);
    aa_is_noflush (AA_OUT, ": ");
    aa_is_noflush (AA_OUT, pg->aa_pg.sa.s);
    aa_is_flush (AA_OUT, " : ");
    draw |= DRAW_CUR_PASSWORD;
}

static void
is_noflush_time (int secs)
{
    char buf[UINT_FMT];
    int mins;

    mins = secs / 60;
    secs -= 60 * mins;

    if (mins > 0)
    {
        buf[uint_fmt (buf, mins)] = '\0';
        aa_is_noflush (AA_OUT, buf);
        aa_is_noflush (AA_OUT, "m");
    }
    if (secs > 0)
    {
        buf[uint_fmt (buf, secs)] = '\0';
        aa_is_noflush (AA_OUT, buf);
        aa_is_noflush (AA_OUT, "s");
    }
}

void
draw_waiting (int already_drawn)
{
    static int n = 1;
    static int tick = -1;
    char buf[UINT_FMT];
    int nb;
    int si;
    tain_t ts;
    int secs;

    if (already_drawn)
        aa_is_noflush (AA_OUT, ANSI_CLEAR_BEFORE ANSI_START_LINE);

    nb = genalloc_len (pid_t, &ga_pid) + nb_wait_longrun;
    if (nb <= 0)
        return;
    else if (n > nb)
    {
        n = 1;
        tick = 0;
    }
    else if (tick > 1)
    {
        tick = 0;
        if (++n > nb)
            n = 1;
    }
    else
        ++tick;

    if (n <= genalloc_len (pid_t, &ga_pid))
        si = list_get (&aa_tmp_list, n - 1);
    else
    {
        int l = genalloc_len (int, &aa_main_list);
        int i;
        int j;

        j = n - genalloc_len (pid_t, &ga_pid);
        for (i = 0; i < l && j < n; ++i)
            if (aa_service (list_get (&aa_main_list, i))->ft_id > 0)
                ++j;
        if (j < n)
            return;
        si = list_get (&aa_main_list, i);
    }

    if (!tain_sub (&ts, &STAMP, &aa_service (si)->ts_exec))
        secs = -1;
    else
    {
        secs = tain_to_millisecs (&ts);
        if (secs > 0)
            secs /= 1000;
    }

    if (nb > 1 || secs >= 0)
        aa_is_noflush (AA_OUT, "[");

    if (nb > 1)
    {
        buf[uint_fmt (buf, n)] = '\0';
        aa_is_noflush (AA_OUT, buf);
        aa_is_noflush (AA_OUT, "/");
        buf[uint_fmt (buf, nb)] = '\0';
        aa_is_noflush (AA_OUT, buf);
        if (secs >= 0)
            aa_is_noflush (AA_OUT, "; ");
    }

    if (secs >= 0)
    {
        is_noflush_time (secs);
        aa_is_noflush (AA_OUT, "/");
        if (aa_service (si)->secs_timeout > 0)
            is_noflush_time (aa_service (si)->secs_timeout);
        else if (is_utf8)
            aa_is_noflush (AA_OUT, "\u221e"); /* infinity sign */
        else
            aa_is_noflush (AA_OUT, "Inf");
    }

    if (nb > 1 || secs >= 0)
        aa_is_noflush (AA_OUT, "] ");

    aa_is_flush (AA_OUT, aa_service_name (aa_service (si)));

    draw |= DRAW_CUR_WAITING;
}

static int
term_set_echo (int on)
{
    struct termios termios;
    int r;

    r = tcgetattr (0, &termios);
    if (r < 0)
        return r;

    if (on)
    {
        if (termios.c_lflag & ECHO)
            return 0;
        termios.c_lflag |= ECHO;
    }
    else
    {
        if (!(termios.c_lflag & ECHO))
            return 0;
        termios.c_lflag &= ~ECHO;
    }

    return tcsetattr (0, TCSANOW, &termios);
}

int
refresh_draw ()
{
    unsigned int old_draw = draw;

    if ((!(draw & DRAW_NEED_WAITING) && (draw & DRAW_CUR_WAITING))
            || (draw & (DRAW_CUR_PROGRESS | DRAW_CUR_PASSWORD)))
    {
        clear_draw ();
        if (old_draw & DRAW_NEED_WAITING)
            draw |= DRAW_NEED_WAITING;
    }

    if ((draw & DRAW_NEED_PASSWORD) && si_password < 0)
    {
        int i;

        for (i = 0; i < genalloc_len (struct progress, &ga_progress); ++i)
        {
            struct progress *pg = &genalloc_s (struct progress, &ga_progress)[i];

            if (pg->si >= 0 && pg->is_drawn == DRAWN_PASSWORD_READY)
            {
                iopause_fd iop;
                int r;

                r = term_set_echo (0);
                if (r < 0)
                {
                    strerr_warnwu2sys ("set terminal attributes; "
                            "can't ask for password for service ",
                            aa_service_name (aa_service (pg->si)));
                    break;
                }

                iop.fd = 0;
                iop.events = IOPAUSE_READ;
                genalloc_append (iopause_fd, &ga_iop, &iop);

                si_password = pg->si;
                break;
            }
        }

        if (si_password < 0)
            draw &= ~DRAW_NEED_PASSWORD;
    }

    if (draw & DRAW_NEED_PROGRESS)
    {
        int i;

        for (i = 0; i < genalloc_len (struct progress, &ga_progress); ++i)
        {
            struct progress *pg = &genalloc_s (struct progress, &ga_progress)[i];

            if (pg->si >= 0 && pg->is_drawn >= 0)
                draw_progress_for (pg->si);
        }

        if (!(draw & DRAW_CUR_PROGRESS))
            draw &= ~DRAW_NEED_PROGRESS;
    }

    if (draw & DRAW_NEED_PASSWORD)
    {
        draw_password ();
        draw &= ~DRAW_NEED_WAITING;
    }

    if (draw & DRAW_NEED_WAITING)
        draw_waiting ((old_draw & DRAW_CUR_WAITING) && !(draw & DRAW_CUR_PROGRESS));

    return 1000 * ((draw & DRAW_CUR_WAITING) ? 1 : SECS_BEFORE_WAITING);
}

void
add_name_to_ga (const char *name, genalloc *ga)
{
    int offset = aa_add_name (name);
    genalloc_append (int, ga, &offset);
}

void
remove_fd_from_iop (int fd)
{
    int i;

    for (i = 0; i < genalloc_len (iopause_fd, &ga_iop); ++i)
        if (genalloc_s (iopause_fd, &ga_iop)[i].fd == fd)
        {
            ga_remove (iopause_fd, &ga_iop, i);
            break;
        }
}

void
close_fd_for (int fd, int si)
{
    if (si < 0)
    {
        int i;

        for (i = 0; i < genalloc_len (int, &aa_tmp_list); ++i)
            if (aa_service (list_get (&aa_tmp_list, i))->fd_in == fd
                    || aa_service (list_get (&aa_tmp_list, i))->fd_out == fd
                    || aa_service (list_get (&aa_tmp_list, i))->fd_progress == fd)
            {
                si = list_get (&aa_tmp_list, i);
                break;
            }
    }

    fd_close (fd);
    remove_fd_from_iop (fd);
    if (si >= 0)
    {
        if (aa_service (si)->fd_in == fd)
            aa_service (si)->fd_in = -1;
        else if (aa_service (si)->fd_out == fd)
            aa_service (si)->fd_out = -1;
        else if (aa_service (si)->fd_progress == fd)
        {
            if (aa_service (si)->pi >= 0)
            {
                struct progress *pg;

                pg = &genalloc_s (struct progress, &ga_progress)[aa_service (si)->pi];
                if (pg->is_drawn)
                    clear_draw ();
                pg->si = -1;
                pg->aa_pg.sa.len = 0;
            }
            aa_service (si)->fd_progress = -1;
        }
    }
}

int
handle_fd_out (int si)
{
    aa_service *s = aa_service (si);

    for (;;)
    {
        char buf[256];
        int r;

        r = fd_read (s->fd_out, buf, 256);
        if (r < 0)
            return (errno == EAGAIN) ? 0 : r;
        else if (r == 0)
        {
            close_fd_for (s->fd_out, si);
            return 0;
        }

        if (!stralloc_catb (&s->sa_out, buf, r))
            return -1;

        for (;;)
        {
            int len;

            len = byte_chr (s->sa_out.s, s->sa_out.len, '\n');
            if (len >= s->sa_out.len)
                break;

            ++len;
            clear_draw ();
            aa_bs_noflush (AA_OUT, aa_service_name (s));
            aa_bs_noflush (AA_OUT, ": ");
            aa_bb_flush (AA_OUT, s->sa_out.s, len);

            memmove (s->sa_out.s, s->sa_out.s + len, s->sa_out.len - len);
            s->sa_out.len -= len;
        }

        if (r < 256)
            return 0;
    }
}

int
handle_fd_progress (int si)
{
    aa_service *s = aa_service (si);
    struct progress *pg;
    char buf[256];
    int i;
    int r;

    if (s->pi < 0)
    {
        for (i = 0; i < genalloc_len (struct progress, &ga_progress); ++i)
        {
            pg = &genalloc_s (struct progress, &ga_progress)[i];
            if (pg->si < 0)
            {
                pg->si = si;
                s->pi = i;
                break;
            }
        }

        if (s->pi < 0)
        {
            struct progress _pg = {
                .si = si,
                .is_drawn = 0,
                .aa_pg.sa = STRALLOC_ZERO
            };
            genalloc_append (struct progress, &ga_progress, &_pg);
            s->pi = genalloc_len (struct progress, &ga_progress) - 1;
        }
    }
    pg = &genalloc_s (struct progress, &ga_progress)[s->pi];

    r = fd_read (s->fd_progress, buf + 1, 255);
    if (r < 0)
        return r;
    else if (r == 0)
    {
        close_fd_for (s->fd_progress, si);
        return 0;
    }

    /* PASSWORD */
    if ((r > 3 && buf[1] == '<' && buf[2] == ' ') || pg->is_drawn == DRAWN_PASSWORD_WAITMSG)
    {
        int rr;

        if (pg->is_drawn != DRAWN_PASSWORD_WAITMSG)
        {
            pg->aa_pg.sa.len = 0;
            i = 3;
        }
        else
            i = 1;

        rr = byte_rchr (buf + i, r, '\n');
        if (rr == r)
        {
            if (!stralloc_catb (&pg->aa_pg.sa, buf + i, rr))
                return -1;
            pg->is_drawn = DRAWN_PASSWORD_WAITMSG;
            return 0;
        }

        buf[i + rr] = '\0';
        if (!stralloc_catb (&pg->aa_pg.sa, buf + i, rr + 1))
            return -1;

        pg->is_drawn = DRAWN_PASSWORD_READY;
        draw |= DRAW_NEED_PASSWORD;
        /* clear in order to "reset" any WAITING */
        clear_draw ();
        /* store timeout and disable it for now */
        pg->secs_timeout = s->secs_timeout;
        s->secs_timeout = 0;
        return 0;
    }
    else if (pg->is_drawn < 0)
        /* no progress during a password prompt */
        return -1;


    /* PROGRESS */

    /* if sa is empty (i.e. we just created pg) we need to keep it consistent
     * with our expectations: it starts with the msg before the
     * buffered/unprocessed data. So we'll add a NUL (i.e. no msg) */
    if (pg->aa_pg.sa.len == 0)
    {
        *buf = '\0';
        if (!stralloc_catb (&pg->aa_pg.sa, buf, r + 1))
            return -1;
    }
    else
        if (!stralloc_catb (&pg->aa_pg.sa, buf + 1, r))
            return -1;

    if (aa_progress_update (&pg->aa_pg) == 0)
    {
        draw |= DRAW_NEED_PROGRESS;
        /* clear in order to "reset" any WAITING */
        clear_draw ();
    }

    return 0;
}

int
handle_fd_in (void)
{
    aa_service *s;
    struct progress *pg;
    char buf[256];
    iopause_fd iop;
    int r;

    r = fd_read (0, buf, 256);
    if (r < 0)
        return r;
    else if (r == 0 || si_password < 0)
        goto done;

    s = aa_service (si_password);
    pg = &genalloc_s (struct progress, &ga_progress)[s->pi];
    if (pg->si != si_password)
    {
        r = -1;
        goto done;
    }

    if (!stralloc_catb (&pg->aa_pg.sa, buf, r))
        return -1;

    iop.fd = s->fd_in;
    iop.events = IOPAUSE_WRITE;
    genalloc_append (iopause_fd, &ga_iop, &iop);
    pg->is_drawn = DRAWN_PASSWORD_WRITING;
    r = 0;

done:
    remove_fd_from_iop (0);
    return r;
}

int
handle_fd (int fd)
{
    int si;
    int i;

    if (fd == 0 && si_password >= 0)
        return handle_fd_in ();

    for (i = 0; i < genalloc_len (int, &aa_tmp_list); ++i)
    {
        si = list_get (&aa_tmp_list, i);
        if (aa_service (si)->fd_out == fd)
            return handle_fd_out (si);
        else if (aa_service (si)->fd_progress == fd)
            return handle_fd_progress (si);
    }

    errno = ENOENT;
    return -1;
}

static void
end_si_password (void)
{
    aa_service *s = aa_service (si_password);
    struct progress *pg = &genalloc_s (struct progress, &ga_progress)[s->pi];
    int r;

    clear_draw ();
    /* we put the message into the service output */
    aa_bs_noflush (AA_OUT, aa_service_name (s));
    aa_bs_noflush (AA_OUT, ": ");
    aa_bs_noflush (AA_OUT, pg->aa_pg.sa.s);
    aa_bs_flush (AA_OUT, "\n");

    remove_fd_from_iop (s->fd_in);
    pg->si = -1;
    pg->is_drawn = 0;
    pg->aa_pg.sa.len = 0;
    s->pi = -1;
    si_password = -1;
    /* restore timeout */
    s->secs_timeout = pg->secs_timeout;
    pg->secs_timeout = 0;
    /* reset ts */
    tain_copynow (&s->ts_exec);

    r = term_set_echo (1);
    if (r < 0)
        strerr_warnwu1sys ("reset terminal attributes");

    /* to get to the next one, if any */
    draw |= DRAW_NEED_PASSWORD;
}

int
handle_fdw (int fd)
{
    aa_service *s;
    struct progress *pg;
    int offset;
    int len;
    int r;

    if (si_password < 0 || aa_service (si_password)->fd_in != fd)
        return (errno = ENOENT, -1);

    s = aa_service (si_password);
    pg = &genalloc_s (struct progress, &ga_progress)[s->pi];

    offset = byte_chr (pg->aa_pg.sa.s, pg->aa_pg.sa.len, '\0') + 1;
    len = pg->aa_pg.sa.len - offset;

    r = fd_write (fd, pg->aa_pg.sa.s + offset, len);
    if (r < 0)
    {
        strerr_warnwu2sys ("write to fd_in of service ", aa_service_name (s));
        return r;
    }
    else if (r < len)
    {
        memmove (pg->aa_pg.sa.s + offset, pg->aa_pg.sa.s + offset + r, len - r);
        pg->aa_pg.sa.len -= r;
    }
    else
        end_si_password ();

    return 0;
}

static int
handle_oneshot (int is_start)
{
    int si;
    int r;
    int wstat;

    r = wait_pids_nohang ((pid_t const *) ga_pid.s, genalloc_len (pid_t, &ga_pid), &wstat);
    if (r < 0)
    {
        if (errno != ECHILD)
            strerr_warnwu1sys ("wait_pids_nohang");
        return r;
    }
    else if (r == 0)
        return r;

    /* get the si; same index in tmp_list except we start at 0 */
    si = list_get (&aa_tmp_list, r - 1);

    remove_from_list (&aa_tmp_list, si);
    ga_remove (pid_t, &ga_pid, r - 1);
    if (si == si_password)
        end_si_password ();
    if (aa_service (si)->fd_in > 0)
        close_fd_for (aa_service (si)->fd_in, si);
    if (aa_service (si)->fd_out > 0)
        close_fd_for (aa_service (si)->fd_out, si);
    if (aa_service (si)->fd_progress > 0)
        close_fd_for (aa_service (si)->fd_progress, si);

    if (WIFEXITED (wstat) && WEXITSTATUS (wstat) == 0)
    {
        aa_service_status *svst = &aa_service (si)->st;

        svst->event = (is_start) ? AA_EVT_STARTED : AA_EVT_STOPPED;
        tain_copynow (&svst->stamp);
        if (aa_service_status_write (svst, aa_service_name (aa_service (si))) < 0)
            strerr_warnwu2sys ("write service status file for ", aa_service_name (aa_service (si)));

        put_title (1, aa_service_name (aa_service (si)),
                (is_start) ? "Started" : "Stopped", 1);
        ++nb_done;
    }
    else
    {
        aa_service_status *svst = &aa_service (si)->st;

        /* if this is the SIGTERM we sent on timeout, treat it as timed out */
        if (aa_service (si)->timedout && !WIFEXITED (wstat) && WTERMSIG (wstat) == SIGTERM)
        {
            svst->event = (is_start) ? AA_EVT_STARTING_FAILED: AA_EVT_STOPPING_FAILED;
            svst->code = ERR_TIMEDOUT;
            tain_copynow (&svst->stamp);
            aa_service_status_set_msg (svst, "");
            if (aa_service_status_write (svst, aa_service_name (aa_service (si))) < 0)
                strerr_warnwu2sys ("write service status file for ", aa_service_name (aa_service (si)));

            put_err_service (aa_service_name (aa_service (si)), ERR_TIMEDOUT, 1);
            genalloc_append (int, &ga_timedout, &si);
        }
        else
        {
            char buf[20];

            svst->event = (is_start) ? AA_EVT_START_FAILED: AA_EVT_STOP_FAILED;
            svst->code = wstat;
            tain_copynow (&svst->stamp);
            aa_service_status_set_msg (svst, "");
            if (aa_service_status_write (svst, aa_service_name (aa_service (si))) < 0)
                strerr_warnwu2sys ("write service status file for ", aa_service_name (aa_service (si)));

            if (WIFEXITED (wstat))
            {
                byte_copy (buf, 9, "exitcode ");
                buf[9 + uint_fmt (buf + 9, WEXITSTATUS (wstat))] = '\0';
            }
            else
            {
                const char *name;

                name = sig_name (WTERMSIG (wstat));
                byte_copy (buf, 10, "signal SIG");
                byte_copy (buf + 10, strlen (name) + 1, name);
            }

            put_err_service (aa_service_name (aa_service (si)), ERR_FAILED, 0);
            add_err (": ");
            add_err (buf);
            end_err ();
            genalloc_append (int, &ga_failed, &si);
        }

        if (is_start)
            check_essential (si);
    }

    remove_from_list (&aa_main_list, si);
    return 1;
}

int
handle_longrun (aa_mode mode, uint16 id, char event)
{
    int si;
    int l = genalloc_len (int, &aa_main_list);
    int i;

    for (i = 0; i < l; ++i)
        if (aa_service (list_get (&aa_main_list, i))->ft_id == id)
            break;

    if (i >= l)
    {
        char buf[UINT16_FMT];
        buf[uint16_fmt (buf, id)] = '\0';
        strerr_warnwu2x ("find longrun service for id#", buf);
        return -1;
    }

    si = list_get (&aa_main_list, i);
    if (mode == AA_MODE_START && aa_service (si)->gets_ready)
    {
        if (event == 'u' || event == 'd')
        {
            clear_draw ();
            aa_bs_noflush (AA_OUT, aa_service_name (aa_service (si)));
            aa_bs_noflush (AA_OUT, ": ");
            aa_bs_flush (AA_OUT, (event == 'u')
                    ? "Started; Getting ready...\n"
                    : "Down; Will restart...\n");
            return 0;
        }
        /* event == 'U' */
        aa_unsubscribe_for (id);
    }

    aa_service (si)->ft_id = 0;
    put_title (1, aa_service_name (aa_service (si)),
            (mode == AA_MODE_START) ?
            ((aa_service (si)->gets_ready) ? "Ready" : "Started")
            : "Stopped", 1);
    ++nb_done;
    --nb_wait_longrun;

    remove_from_list (&aa_main_list, si);
    return 1;
}

int
is_locale_utf8 (void)
{
    const char *set;

    setlocale (LC_CTYPE, "");
    set = nl_langinfo (CODESET);
    if (set && strcmp (set, "UTF-8") == 0)
        return 1;
    else
        return 0;
}

int
get_cols (int fd)
{
    struct winsize win;

    if (isatty (fd) && ioctl (fd, TIOCGWINSZ, &win) == 0)
        return win.ws_col;
    else
        return 80;
}

int
handle_signals (aa_mode mode)
{
    int r = 0;

    for (;;)
    {
        char c;

        c = selfpipe_read ();
        switch (c)
        {
            case -1:
                strerr_diefu1sys (ERR_IO, "selfpipe_read");

            case 0:
                return r;

            case SIGCHLD:
                {
                    for (;;)
                    {
                        int rr = handle_oneshot (mode == AA_MODE_START);
                        if (rr > 0)
                            r += rr;
                        else
                            break;
                    }
                    break;
                }

            case SIGTERM:
            case SIGQUIT:
                ioloop = 0;
                break;

            case SIGWINCH:
                cols = get_cols (1);
                break;

            default:
                strerr_dief1x (ERR_IO, "internal error: invalid selfpipe_read value");
        }
    }
}

void
prepare_cb (int cur, int next, int is_needs, int first)
{
    int l = genalloc_len (int, &aa_tmp_list);
    int i;

    if (is_needs)
    {
        put_err (aa_service_name (aa_service (cur)), "remove ", 0);
        add_err (aa_service_name (aa_service (cur)));
        add_err (" needs/after ");
        add_err (aa_service_name (aa_service (next)));
        add_err (" to break dependency loop: ");
        for (i = first; i < l; ++i)
        {
            add_err (aa_service_name (aa_service (list_get (&aa_tmp_list, i))));
            if (i < l - 1)
                add_err (" -> ");
        }
        end_err ();
    }
    else
    {
        put_warn (aa_service_name (aa_service (cur)), "remove ", 0);
        add_warn (aa_service_name (aa_service (cur)));
        add_warn (" after ");
        add_warn (aa_service_name (aa_service (next)));
        add_warn (" to break loop: ");
        for (i = first; i < l; ++i)
        {
            add_warn (aa_service_name (aa_service (list_get (&aa_tmp_list, i))));
            if (i < l - 1)
                add_warn (" -> ");
        }
        end_warn ();
    }
}

void
exec_cb (int si, aa_evt evt, pid_t pid)
{
    aa_service *s = aa_service (si);

    switch ((int) evt)
    {
        /* ugly hack thing; see aa_exec_service() */
        case 0:
            clear_draw ();
            aa_bs_noflush (AA_OUT,
                    (pid == AA_MODE_START) ? "Starting " : "Stopping ");
            aa_bs_noflush (AA_OUT, aa_service_name (aa_service (si)));
            aa_bs_flush (AA_OUT, "...\n");
            break;

        case AA_EVT_STARTING:
        case AA_EVT_STOPPING:
            if (s->st.type == AA_TYPE_ONESHOT)
            {
                iopause_fd iop;

                iop.fd = s->fd_out;
                iop.events = IOPAUSE_READ;
                genalloc_append (iopause_fd, &ga_iop, &iop);
                iop.fd = s->fd_progress;
                genalloc_append (iopause_fd, &ga_iop, &iop);

                add_to_list (&aa_tmp_list, si, 0);
                genalloc_append (pid_t, &ga_pid, &pid);
            }
            else
                ++nb_wait_longrun;
            break;

        case AA_EVT_STARTED:
        case AA_EVT_STOPPED:
            put_title (1, aa_service_name (s),
                    (evt == AA_EVT_STARTED) ? "Started" : "Stopped", 1);
            ++nb_done;
            break;

        case AA_EVT_STARTING_FAILED:
        case AA_EVT_STOPPING_FAILED:
            {
                const char *msg;

                msg = aa_service_status_get_msg (&s->st);
                put_err_service (aa_service_name (s), s->st.code, !msg);
                if (msg)
                {
                    add_err (": ");
                    add_err (msg);
                    end_err ();
                }
                genalloc_append (int, &ga_failed, &si);
                check_essential (si);
                break;
            }

        case -ERR_ALREADY_UP: /* could happen w/ longrun */
            put_title (1, aa_service_name (s), errmsg[ERR_ALREADY_UP], 1);
            ++nb_already;
            break;

        case -ERR_NOT_UP:
            put_title (1, aa_service_name (s), errmsg[ERR_NOT_UP], 1);
            ++nb_already;
            break;
    }
}

int
process_timeouts (aa_mode mode, aa_scan_cb scan_cb)
{
    int si;
    int l;
    int i;
    tain_t ts_timeout;
    tain_t ts;
    tain_t tms;
    int ms = -1;
    int scan = 0;

    l = genalloc_len (int, &aa_tmp_list);
    for (i = 0; i < l; ++i)
    {
        si = list_get (&aa_tmp_list, i);
        /* no limit? */
        if (aa_service (si)->secs_timeout == 0)
            continue;

        tain_from_millisecs (&ts_timeout, 1000 * aa_service (si)->secs_timeout);
        tain_add (&ts, &aa_service (si)->ts_exec, &ts_timeout);
        /* timeout expired? */
        if (tain_less (&ts, &STAMP))
        {
            /* not yet signaled? */
            if (!aa_service (si)->timedout)
            {
                kill (genalloc_s (pid_t, &ga_pid)[i], SIGTERM);
                aa_service (si)->timedout = 1;
            }

            tain_addsec (&tms, &ts, 2);
            if (!tain_less (&tms, &STAMP))
            {
                aa_service_status *svst = &aa_service (si)->st;

                kill (genalloc_s (pid_t, &ga_pid)[i], SIGKILL);

                remove_from_list (&aa_tmp_list, si);
                ga_remove (pid_t, &ga_pid, i);
                if (si == si_password)
                    end_si_password ();
                if (aa_service (si)->fd_in > 0)
                    close_fd_for (aa_service (si)->fd_in, si);
                if (aa_service (si)->fd_out > 0)
                    close_fd_for (aa_service (si)->fd_out, si);
                if (aa_service (si)->fd_progress > 0)
                    close_fd_for (aa_service (si)->fd_progress, si);

                svst->event = (mode == AA_MODE_START) ? AA_EVT_STARTING_FAILED: AA_EVT_STOPPING_FAILED;
                svst->code = ERR_TIMEDOUT;
                tain_copynow (&svst->stamp);
                aa_service_status_set_msg (svst, "");
                if (aa_service_status_write (svst, aa_service_name (aa_service (si))) < 0)
                    strerr_warnwu2sys ("write service status file for ", aa_service_name (aa_service (si)));

                put_err_service (aa_service_name (aa_service (si)), ERR_TIMEDOUT, 1);
                genalloc_append (int, &ga_timedout, &si);
                if (mode == AA_MODE_START)
                    check_essential (si);

                remove_from_list (&aa_main_list, si);
                scan = 1;
            }
            else
            {
                int _ms;

                ts = tms;
                tain_sub (&tms, &ts, &STAMP);
                _ms = tain_to_millisecs (&tms);
                if (_ms > 0 && (ms < 0 || _ms < ms))
                    ms = _ms;
            }
        }
        else
        {
            int _ms;

            tain_sub (&tms, &ts, &STAMP);
            _ms = tain_to_millisecs (&tms);
            if (ms < 0 || _ms < ms)
                ms = _ms;
        }
    }

    if (nb_wait_longrun > 0)
    {
        int j = 0;

        l = genalloc_len (int, &aa_main_list);

        for (i = 0; i < l && j < nb_wait_longrun; ++i)
            if (aa_service (list_get (&aa_main_list, i))->ft_id > 0)
            {
                ++j;
                si = list_get (&aa_main_list, i);
                /* no limit? */
                if (aa_service (si)->secs_timeout == 0)
                    continue;

                tain_from_millisecs (&ts_timeout, 1000 * aa_service (si)->secs_timeout);
                tain_add (&ts, &aa_service (si)->ts_exec, &ts_timeout);
                /* timeout expired? */
                if (tain_less (&ts, &STAMP))
                {
                    aa_service_status *svst = &aa_service (si)->st;

                    aa_unsubscribe_for (aa_service (si)->ft_id);
                    aa_service (si)->ft_id = 0;
                    --nb_wait_longrun;

                    svst->event = (mode == AA_MODE_START) ? AA_EVT_STARTING_FAILED: AA_EVT_STOPPING_FAILED;
                    svst->code = ERR_TIMEDOUT;
                    tain_copynow (&svst->stamp);
                    aa_service_status_set_msg (svst, "");
                    if (aa_service_status_write (svst, aa_service_name (aa_service (si))) < 0)
                        strerr_warnwu2sys ("write service status file for ", aa_service_name (aa_service (si)));

                    put_err_service (aa_service_name (aa_service (si)), ERR_TIMEDOUT, 1);
                    genalloc_append (int, &ga_timedout, &si);
                    if (mode == AA_MODE_START)
                        check_essential (si);

                    remove_from_list (&aa_main_list, si);
                    scan = 1;
                }
                else
                {
                    int _ms;

                    tain_sub (&tms, &ts, &STAMP);
                    _ms = tain_to_millisecs (&tms);
                    if (ms < 0 || _ms < ms)
                        ms = _ms;
                }
            }
    }

    if (scan)
        aa_scan_mainlist (scan_cb, mode);

    return ms;
}

void
mainloop (aa_mode mode, aa_scan_cb scan_cb)
{
    sigset_t set;
    iopause_fd iop;
    int i;

    if (!genalloc_ready_tuned (iopause_fd, &ga_iop, 2, 0, 0, 1))
        strerr_diefu1sys (ERR_IO, "allocate iopause_fd");

    iop.fd = selfpipe_init ();
    if (iop.fd == -1)
        strerr_diefu1sys (ERR_IO, "init selfpipe");
    iop.events = IOPAUSE_READ;
    genalloc_append (iopause_fd, &ga_iop, &iop);

    iop.fd = aa_prepare_mainlist (prepare_cb, exec_cb);
    if (iop.fd < 0)
        strerr_diefu1sys (ERR_IO, "prepare mainlist");
    else if (iop.fd == 0)
        iop.fd = -1;
    genalloc_append (iopause_fd, &ga_iop, &iop);

    sigemptyset (&set);
    sigaddset (&set, SIGCHLD);
    sigaddset (&set, SIGTERM);
    sigaddset (&set, SIGQUIT);
    sigaddset (&set, SIGWINCH);
    if (selfpipe_trapset (&set) < 0)
        strerr_diefu1sys (ERR_IO, "trap signals");

    /* start what we can */
    for (i = 0; i < genalloc_len (int, &aa_main_list); ++i)
        if (genalloc_len (int, &aa_service (list_get (&aa_main_list, i))->after) == 0)
            if (aa_exec_service (list_get (&aa_main_list, i), mode) < 0)
            {
                aa_scan_mainlist (scan_cb, mode);
                break;
            }

    while (ioloop && (genalloc_len (int, &aa_main_list) > 0))
    {
        int nb_iop;
        int r;
        int ms1, ms2;
        tain_t tms;

        ms1 = process_timeouts (mode, scan_cb);
        ms2 = refresh_draw ();
        tain_from_millisecs (&tms, (ms1 < 0 || ms2 < ms1) ? ms2 : ms1);
        tain_add (&iol_deadline, &STAMP, &tms);

        nb_iop = genalloc_len (iopause_fd, &ga_iop);
        r = iopause_g (genalloc_s (iopause_fd, &ga_iop), nb_iop, &iol_deadline);
        if (r < 0)
            strerr_diefu1sys (ERR_IO, "iopause");
        else if (r == 0)
        {
            if (ms1 < 0 || ms2 < ms1)
                draw |= DRAW_NEED_WAITING;
        }
        else
        {
            iopause_fd *iofd;
            int scan = 0;

            for (--nb_iop; nb_iop > 1; --nb_iop)
            {
                iofd = &genalloc_s (iopause_fd, &ga_iop)[nb_iop];
                if (iofd->revents & IOPAUSE_READ)
                {
                    r = handle_fd (iofd->fd);
                    if (r < 0)
                        strerr_warnwu1sys ("handle fd");
                }
                else if (iofd->revents & IOPAUSE_WRITE)
                {
                    r = handle_fdw (iofd->fd);
                    if (r < 0)
                        strerr_warnwu1sys ("handle fdw");
                }
                else if (iofd->revents & IOPAUSE_EXCEPT)
                    close_fd_for (iofd->fd, -1);
            }

            iofd = &genalloc_s (iopause_fd, &ga_iop)[0];
            if (iofd->revents & IOPAUSE_READ)
                scan += handle_signals (mode);
            else if (iofd->revents & IOPAUSE_EXCEPT)
                strerr_diefu1sys (ERR_IO, "iopause: selfpipe error");

            iofd = &genalloc_s (iopause_fd, &ga_iop)[1];
            if (iofd->fd <= 0)
                goto scan;
            if (iofd->revents & IOPAUSE_READ)
            {
                for (;;)
                {
                    uint16 id;
                    char event;

                    r = aa_get_longrun_info (&id, &event);
                    if (r < 0)
                    {
                        strerr_warnwu1sys ("get longrun information");
                        break;
                    }
                    else if (r == 0)
                        break;

                    r = handle_longrun (mode, id, event);
                    if (r > 0)
                        scan += r;
                }
            }
            else if (iofd->revents & IOPAUSE_EXCEPT)
                strerr_diefu1sys (ERR_IO, "iopause: longrun pipe error");

scan:
            if (scan > 0)
                aa_scan_mainlist (scan_cb, mode);
        }
    }
}

void
show_stat_service_names (genalloc *ga, const char *title, const char *ansi_color)
{
    int i;

    if (genalloc_len (int, ga) <= 0)
        return;

    put_title (0, title, "", 0);
    for (i = 0; i < genalloc_len (int, ga); ++i)
    {
        if (i > 0)
            aa_bs_noflush (AA_OUT, "; ");
        aa_is_noflush (AA_OUT, ansi_color);
        aa_bs_noflush (AA_OUT, aa_service_name (aa_service (list_get (ga, i))));
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    }
    end_title ();
}
