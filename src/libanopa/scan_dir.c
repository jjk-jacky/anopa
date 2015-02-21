
#define _BSD_SOURCE

#include <sys/stat.h>
#include <errno.h>
#include <skalibs/direntry.h>
#include <skalibs/stralloc.h>
#include <anopa/err.h>
#include <anopa/scan_dir.h>


/* breaking the rule here: we get a stralloc* but we don't own it, it's just so
 * we can use it if needed to stat() */
int
aa_scan_dir (stralloc *sa, int files_only, aa_sd_it_fn iterator, void *data)
{
    DIR *dir;
    int e = 0;
    int r = 0;

    dir = opendir (sa->s);
    if (!dir)
        return -ERR_IO;

    for (;;)
    {
        direntry *d;

        errno = 0;
        d = readdir (dir);
        if (!d)
        {
            e = errno;
            break;
        }
        if (d->d_name[0] == '.'
                && (d->d_name[1] == '\0' || (d->d_name[1] == '.' && d->d_name[2] == '\0')))
            continue;
        if (d->d_type == DT_UNKNOWN)
        {
            struct stat st;
            int l;
            int rr;

            l = sa->len;
            sa->s[l - 1] = '/';
            stralloc_catb (sa, d->d_name, str_len (d->d_name) + 1);
            rr = stat (sa->s, &st);
            sa->len = l;
            sa->s[l - 1] = '\0';
            if (rr != 0)
                continue;
            if (S_ISREG (st.st_mode))
                d->d_type = DT_REG;
            else if (S_ISDIR (st.st_mode))
                d->d_type = DT_DIR;
        }
        if (d->d_type != DT_REG && (files_only || d->d_type != DT_DIR))
            continue;

        r = iterator (d, data);
        if (r < 0)
            break;
    }
    dir_close (dir);

    if (e > 0)
    {
        r = -ERR_IO;
        errno = e;
    }
    return r;
}
