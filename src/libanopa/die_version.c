
#include "anopa/config.h"

#include <unistd.h>
#include <anopa/output.h>

extern char const *PROG;

void
aa_die_version (void)
{
    aa_init_output (0);
    aa_bs_noflush (AA_OUT, PROG);
    aa_bs_noflush (AA_OUT, " v" ANOPA_VERSION "\n");
    aa_bs_flush (AA_OUT,
            "Copyright (C) 2015 Olivier Brunel - http://jjacky.com/anopa\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n"
            );
    _exit (0);
}
