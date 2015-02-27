
#include <getopt.h>
#include <skalibs/djbunix.h>
#include <skalibs/stralloc.h>
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <anopa/common.h>

const char *PROG;

static void
dieusage (int rc)
{
    aa_die_usage (rc, "[OPTION] NAME",
            " -f, --file FILE               Use FILE instead of /proc/cmdline\n"
            " -q, --quiet                   Don't write value (if any) to stdout\n"
            " -s, --safe[=C]                Ignore argument if value contain C (default: '/')\n"
            " -r, --required                Ignore argument if no value specified\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-incmdline";
    stralloc sa = STRALLOC_ZERO;
    const char *file = "/proc/cmdline";
    int quiet = 0;
    int req = 0;
    char safe = '\0';
    int len_arg;
    int start;
    int i;

    for (;;)
    {
        struct option longopts[] = {
            { "file",               no_argument,        NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "quiet",              no_argument,        NULL,   'q' },
            { "required",           no_argument,        NULL,   'r' },
            { "safe",               optional_argument,  NULL,   's' },
            { "version",            no_argument,        NULL,   'V' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "f:hqrs::V", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'f':
                file = optarg;
                break;

            case 'h':
                dieusage (0);

            case 'q':
                quiet = 1;
                break;

            case 'r':
                req = 1;
                break;

            case 's':
                if (!optarg)
                    safe = '/';
                else if (!*optarg || optarg[1])
                    dieusage (1);
                else
                    safe = *optarg;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
        dieusage (1);

    if (!openslurpclose (&sa, file))
            strerr_diefu2sys (2, "read ", file);

    len_arg = strlen (argv[0]);
    for (start = i = 0; i < sa.len; ++i)
    {
        if (sa.s[i] == '=' || sa.s[i] == ' ' || sa.s[i] == '\t'
                || sa.s[i] == '\n' || sa.s[i] == '\0')
        {
            int found = (i - start == len_arg && !str_diffn (sa.s + start, argv[0], len_arg));
            int len;

            if (sa.s[i] != '=')
            {
                if (found)
                    return (req) ? 3 : 0;
                start = ++i;
                goto next;
            }
            else if (found && quiet && !safe)
                return (req) ? 3 : 0;

            start = ++i;
            if (sa.s[start] != '"')
                for (len = 0;
                        start + len < sa.len
                        && sa.s[start + len] != ' ' && sa.s[start + len] != '\t';
                        ++len)
                    ;
            else
            {
                ++start;
                len = byte_chr (sa.s + start, sa.len - start, '"');
            }

            if (found)
            {
                if (len == sa.len - start)
                    --len;
                if (safe && byte_chr (sa.s + start, len, safe) < len)
                    return 3;
                if (req && len == 0)
                    return 3;
                else if (!quiet)
                {
                    buffer_putnoflush (buffer_1small, sa.s + start, len);
                    buffer_putsflush (buffer_1small, "\n");
                }
                return 0;
            }

            start += len;
            i = ++start;
next:
            while (i < sa.len && (sa.s[i] == ' ' || sa.s[i] == '\t'))
                start = ++i;
        }
    }

    return 3;
}
