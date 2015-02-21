
#include <unistd.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>

const char *PROG;

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION]",
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-sync";

    if (argc == 1)
    {
        sync ();
        return 0;
    }

    if (argc == 2 && (str_equal (argv[1], "-V") || str_equal (argv[1], "--version")))
        aa_die_version ();
    dieusage ((argc == 2 && (str_equal (argv[1], "-h") || str_equal (argv[1], "--help"))) ? 0 : 1);
}
