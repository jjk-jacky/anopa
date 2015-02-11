
#include <sys/types.h>
#include <sys/stat.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/uint32.h>
#include <skalibs/tai.h>
#include <anopa/service_status.h>


void
aa_service_status_free (aa_service_status *svst)
{
    stralloc_free (&svst->sa);
}

int
aa_service_status_read (aa_service_status *svst, const char *dir)
{
    unsigned int len = strlen (dir);
    char file[len + 1 + sizeof (AA_SVST_FILENAME)];
    uint32 u;

    byte_copy (file, len, dir);
    byte_copy (file + len, 1 + sizeof (AA_SVST_FILENAME), "/" AA_SVST_FILENAME);

    if (!openreadfileclose (file, &svst->sa, AA_SVST_FIXED_SIZE + AA_SVST_MAX_MSG_SIZE + 1)
            || svst->sa.len < AA_SVST_FIXED_SIZE)
    {
        tain_now_g ();
        return -1;
    }
    tain_now_g ();

    svst->sa.s[svst->sa.len] = '\0';
    if (svst->sa.len < AA_SVST_FIXED_SIZE + AA_SVST_MAX_MSG_SIZE + 1)
        svst->sa.len++;

    tain_unpack (svst->sa.s, &svst->stamp);
    uint32_unpack (svst->sa.s + 12, &u);
    svst->event = (unsigned int) u;
    uint32_unpack (svst->sa.s + 16, &u);
    svst->code = (int) u;

    return 0;
}

int
aa_service_status_write (aa_service_status *svst, const char *dir)
{
    unsigned int len = strlen (dir);
    char file[len + 1 + sizeof (AA_SVST_FILENAME)];
    mode_t mask;
    int r;

    if (!stralloc_ready_tuned (&svst->sa, AA_SVST_FIXED_SIZE, 0, 0, 1))
        return -1;

    tain_pack (svst->sa.s, &svst->stamp);
    uint32_pack (svst->sa.s + 12, (uint32) svst->event);
    uint32_pack (svst->sa.s + 16, (uint32) svst->code);
    if (svst->sa.len < AA_SVST_FIXED_SIZE)
        svst->sa.len = AA_SVST_FIXED_SIZE;

    byte_copy (file, len, dir);
    byte_copy (file + len, 1 + sizeof (AA_SVST_FILENAME), "/" AA_SVST_FILENAME);

    mask = umask (0033);
    if (!openwritenclose_suffix (file, svst->sa.s,
                svst->sa.len + ((svst->sa.len > AA_SVST_FIXED_SIZE) ? -1 : 0), ".new"))
        r = -1;
    else
        r = 0;
    umask (mask);

    tain_now_g ();
    return r;
}

int
aa_service_status_set_msg (aa_service_status *svst, const char *msg)
{
    int len;

    len = strlen (msg);
    if (len > AA_SVST_MAX_MSG_SIZE)
        len = AA_SVST_MAX_MSG_SIZE;

    if (!stralloc_ready_tuned (&svst->sa, AA_SVST_FIXED_SIZE + len + 1, 0, 0, 1))
        return -1;

    svst->sa.len = AA_SVST_FIXED_SIZE;
    stralloc_catb (&svst->sa, msg, len);
    stralloc_0 (&svst->sa);
    return 0;
}

int
aa_service_status_set_err (aa_service_status *svst, int err, const char *msg)
{
    svst->event = AA_EVT_ERROR;
    svst->code = err;
    tain_copynow (&svst->stamp);
    return aa_service_status_set_msg (svst, (msg) ? msg : "");
}
