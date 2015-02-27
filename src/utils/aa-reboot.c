
#include <getopt.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <skalibs/strerr2.h>
#include <anopa/common.h>

const char *PROG;

static void
dieusage (int rc)
{
    aa_die_usage (rc, "OPTION",
            " -r, --reboot                  Reboot the machine NOW\n"
            " -H, --halt                    Halt the machine NOW\n"
            " -p, --poweroff                Power off the machine NOW\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-reboot";
    struct
    {
        int cmd;
        const char *desc;
    } cmd[3] = {
        { .cmd = RB_HALT_SYSTEM, .desc = "halt" },
        { .cmd = RB_POWER_OFF,   .desc = "power off" },
        { .cmd = RB_AUTOBOOT,    .desc = "reboot" }
    };
    int i = -1;

    for (;;)
    {
        struct option longopts[] = {
            { "halt",               no_argument,        NULL,   'H' },
            { "help",               no_argument,        NULL,   'h' },
            { "poweroff",           no_argument,        NULL,   'p' },
            { "reboot",             no_argument,        NULL,   'r' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "HhprV", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'H':
                i = 0;
                break;

            case 'h':
                dieusage (0);

            case 'p':
                i = 1;
                break;

            case 'r':
                i = 2;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 0 || i < 0)
        dieusage (1);

    if (reboot (cmd[i].cmd) < 0)
        strerr_diefu2sys (2, cmd[i].desc, " the machine");

    /* unlikely :p */
    return 0;
}
