
#include <getopt.h>
#include <skalibs/strerr2.h>
#include <anopa/common.h>

#ifndef NULL
#define NULL    (void *) 0
#endif

extern int pivot_root (char const *new_root, char const *old_root);

static void
dieusage (int rc)
{
    aa_die_usage (rc, "NEWROOT OLDROOT",
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-pivot";

    for (;;)
    {
        struct option longopts[] = {
            { "help",               no_argument,        NULL,   'h' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "hV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'h':
                dieusage (0);

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 2)
        dieusage (1);

    if (pivot_root (argv[0], argv[1]) < 0)
        strerr_diefu2sys (2, "pivot into ", argv[0]);
    return 0;
}
