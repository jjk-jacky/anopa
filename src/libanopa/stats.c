
#include <skalibs/uint.h>
#include <skalibs/buffer.h>
#include <skalibs/genalloc.h>
#include <anopa/output.h>
#include <anopa/ga_int_list.h>

void
aa_show_stat_nb (int nb, const char *title, const char *ansi_color)
{
    char buf[UINT_FMT];

    if (nb <= 0)
        return;

    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_BLUE_ON);
    aa_bs_noflush (AA_OUT, "  -> ");
    aa_is_noflush (AA_OUT, ansi_color);
    aa_bs_noflush (AA_OUT, title);
    aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    aa_bs_noflush (AA_OUT, ": ");
    buf[uint_fmt (buf, nb)] = '\0';
    aa_bs_noflush (AA_OUT, buf);
    aa_end_title ();
}

void
aa_show_stat_names (const char  *names,
                    genalloc    *ga_offets,
                    const char  *title,
                    const char  *ansi_color)
{
    int i;

    if (genalloc_len (int, ga_offets) <= 0)
        return;

    aa_put_title (0, title, "", 0);
    for (i = 0; i < genalloc_len (int, ga_offets); ++i)
    {
        if (i > 0)
            aa_bs_noflush (AA_OUT, "; ");
        aa_is_noflush (AA_OUT, ansi_color);
        aa_bs_noflush (AA_OUT, names + list_get (ga_offets, i));
        aa_is_noflush (AA_OUT, ANSI_HIGHLIGHT_ON);
    }
    aa_end_title ();
}
