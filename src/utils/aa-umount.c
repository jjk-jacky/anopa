
#include <getopt.h>
#include <sys/mount.h>
#include <skalibs/strerr2.h>
#include <anopa/common.h>

#ifndef NULL
#define NULL    (void *) 0
#endif

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTIONS...] MOUNTPOINT",
            " -f, --force                   Force unmount even if busy (NFS only)\n"
            " -l, --lazy                    Perform lazy unmounting\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-umount";
    int flags = 0;

    for (;;)
    {
        struct option longopts[] = {
            { "force",              no_argument,        NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "lazy",               no_argument,        NULL,   'l' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "fhlV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'f':
                flags = MNT_FORCE;
                break;

            case 'h':
                dieusage (0);

            case 'l':
                flags = MNT_DETACH;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        dieusage (1);

    if (umount2 (argv[0], flags) < 0)
        strerr_diefu2sys (3, "unmount ", argv[0]);

    return 0;
}
