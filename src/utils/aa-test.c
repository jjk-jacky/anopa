
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <anopa/common.h>

static int
is_group_member (gid_t gid)
{
    int nb = getgroups (0, NULL);
    gid_t groups[nb];
    int i;

    if (gid == getgid () || gid == getegid())
        return 1;

    if (getgroups (nb, groups) < 0)
        return 0;

    for (i = 0; i < nb; ++i)
        if (gid == groups[i])
            return 1;

    return 0;
}

static void
dieusage (int rc)
{
    aa_die_usage (rc, "OPTION FILE",
            " -b, --block                   Test whether FILE is a block special\n"
            " -d, --directory               Test whether FILE is a directory\n"
            " -e, --exists                  Test whether FILE exists\n"
            " -f, --file                    Test whether FILE is a regular file\n"
            " -L, --symlink                 Test whether FILE is a symbolic link\n"
            " -p, --pipe                    Test whether FILE is a named pipe\n"
            " -S, --socket                  Test whether FILE is a socket\n"
            " -r, --read                    Test for read permission on FILE\n"
            " -w, --write                   Test for write permission on FILE\n"
            " -x, --execute                 Test for execute permission on FILE\n"
            " -R, --repeat[=TIMES]          Repeat test every second up to TIMES times\n"
            "\n"
            " -h, --help                    Show this help screen and exit\n"
            " -V, --version                 Show version information and exit\n"
            );
}

int
main (int argc, char * const argv[])
{
    PROG = "aa-test";
    struct stat st;
    uid_t euid;
    int mode;
    char test = 0;
    int repeat = -1;

    for (;;)
    {
        struct option longopts[] = {
            { "block",              no_argument,        NULL,   'b' },
            { "directory",          no_argument,        NULL,   'd' },
            { "exists",             no_argument,        NULL,   'e' },
            { "file",               no_argument,        NULL,   'f' },
            { "help",               no_argument,        NULL,   'h' },
            { "symlink",            no_argument,        NULL,   'L' },
            { "pipe",               no_argument,        NULL,   'p' },
            { "read",               no_argument,        NULL,   'r' },
            { "repeat",             optional_argument,  NULL,   'R' },
            { "socket",             no_argument,        NULL,   'S' },
            { "version",            no_argument,        NULL,   'V' },
            { "write",              no_argument,        NULL,   'w' },
            { "execute",            no_argument,        NULL,   'x' },
            { NULL, 0, 0, 0 }
        };
        int c;

        c = getopt_long (argc, argv, "bdefhLprR::SVwx", longopts, NULL);
        if (c == -1)
            break;
        switch (c)
        {
            case 'b':
            case 'd':
            case 'e':
            case 'f':
            case 'L':
            case 'p':
            case 'r':
            case 'S':
            case 'w':
            case 'x':
                test = c;
                break;

            case 'h':
                dieusage (0);

            case 'R':
                if (optarg && !uint0_scan (optarg, &repeat))
                    strerr_diefu2sys (1, "set repeat counter to ", optarg);
                else if (!optarg)
                    repeat = 0;
                break;

            case 'V':
                aa_die_version ();

            default:
                dieusage (1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1 || test == 0)
        dieusage (1);

    if (repeat > 0)
        ++repeat;

again:
    if (lstat (argv[0], &st) < 0)
    {
        if (errno != ENOENT)
            strerr_diefu2sys (2, "stat ", argv[0]);
        else if (repeat >= 0)
        {
            if (repeat > 1)
                --repeat;
            else if (repeat == 1)
                return 3;
            sleep (1);
            goto again;
        }
        else
            return 3;
    }

    switch (test)
    {
        case 'b':
            return (S_ISBLK (st.st_mode)) ? 0 : 4;

        case 'd':
            return (S_ISDIR (st.st_mode)) ? 0 : 4;

        case 'e':
            return 0;

        case 'f':
            return (S_ISREG (st.st_mode)) ? 0 : 4;

        case 'L':
            return (S_ISLNK (st.st_mode)) ? 0 : 4;

        case 'p':
            return (S_ISFIFO (st.st_mode)) ? 0 : 4;

        case 'r':
            mode = R_OK;
            break;

        case 'S':
            return (S_ISSOCK (st.st_mode)) ? 0 : 4;

        case 'w':
            mode = W_OK;
            break;

        case 'x':
            mode = X_OK;
            break;
    }

    euid = geteuid ();
    if (euid == 0)
    {
        /* root can read/write any file */
        if (mode != X_OK)
            return 0;
        /* and execute anything any execute permission set */
        else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
            return 0;
        else
            return 4;
    }

    if (st.st_uid == euid)
        mode <<= 6;
    else if (is_group_member (st.st_gid))
        mode <<= 3;

    return (st.st_mode & mode) ? 0 : 4;
}
