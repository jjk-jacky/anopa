
#include <getopt.h>
#include <skalibs/bytestr.h>
#include <anopa/common.h>
#include <anopa/output.h>

char const *PROG;
typedef void (*put_fn) (const char *name, const char *msg, int end);

static void
put_title (const char *name, const char *msg, int end)
{
    aa_put_title (1, name, msg, end);
}

static void
put_title2 (const char *name, const char *msg, int end)
{
    aa_put_title (0, name, msg, end);
}

static void
dieusage (void)
{
    aa_die_usage ("[OPTION...] MESSAGE...",
            " -D, --double-output           Enable double-output mode\n"
            " -T, --title                   Show a main title (default)\n"
            " -t, --title2                  Show a secondary title\n"
            " -w, --warning                 Show a warning\n"
            " -e, --error                   Show an error\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            "\n"
            "MESSAGE can be used to set the text color:\n"
            "\n"
            " +g, +green                    Set color to green\n"
            " +b, +blue                     Set color to blue\n"
            " +y, +yellow                   Set color to yellow\n"
            " +r, +red                      Set color to red\n"
            " +n, +normal                   Set color to normal (white)\n"
            " ++TEXT                        To just print +TEXT\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-echo";
    int mode_both = 0;
    put_fn put = put_title;
    int where = AA_OUT;
    int i;

    for (;;)
    {
        struct option longopts[] = {
            { "help",               no_argument,        NULL,   'h' },
            { "version",            no_argument,        NULL,   'V' },
            { "double-output",      no_argument,        NULL,   'D' },
            { "title",              no_argument,        NULL,   'T' },
            { "title2",             no_argument,        NULL,   't' },
            { "warning",            no_argument,        NULL,   'w' },
            { "error",              no_argument,        NULL,   'e' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "hVDTtwe", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'D':
                mode_both = 1;
                break;

            case 'T':
                put = put_title;
                where = AA_OUT;
                break;

            case 't':
                put = put_title2;
                where = AA_OUT;
                break;

            case 'w':
                put = aa_put_warn;
                where = AA_ERR;
                break;

            case 'e':
                put = aa_put_err;
                where = AA_ERR;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage ();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
        dieusage ();

    aa_init_output (mode_both);
    put ("", NULL, 0);
    for (i = 0; i < argc; ++i)
    {
        if (*argv[i] == '+')
        {
            if (str_equal (argv[i], "+g") || str_equal (argv[i], "+green"))
                aa_is_noflush (where, ANSI_HIGHLIGHT_GREEN_ON);
            else if (str_equal (argv[i], "+b") || str_equal (argv[i], "+blue"))
                aa_is_noflush (where, ANSI_HIGHLIGHT_BLUE_ON);
            else if (str_equal (argv[i], "+y") || str_equal (argv[i], "+yellow"))
                aa_is_noflush (where, ANSI_HIGHLIGHT_YELLOW_ON);
            else if (str_equal (argv[i], "+r") || str_equal (argv[i], "+red"))
                aa_is_noflush (where, ANSI_HIGHLIGHT_RED_ON);
            else if (str_equal (argv[i], "+n") || str_equal (argv[i], "+normal"))
                aa_is_noflush (where, ANSI_HIGHLIGHT_ON);
            else
                aa_bs_noflush (where, argv[i] + 1);
        }
        else
            aa_bs_noflush (where, argv[i]);
    }
    aa_bs_flush (where, "\n");

    return 0;
}