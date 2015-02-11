
#include <unistd.h>
#include <anopa/output.h>

extern char const *PROG;

void
aa_die_usage (const char *usage, const char *details)
{
    aa_init_output (0);
    aa_bs_noflush (AA_OUT, "Usage: ");
    aa_bs_noflush (AA_OUT, PROG);
    aa_bs_noflush (AA_OUT, " ");
    aa_bs_noflush (AA_OUT, usage);
    aa_bs_noflush (AA_OUT, "\n\n");
    aa_bs_flush (AA_OUT, details);
    _exit (0);
}
