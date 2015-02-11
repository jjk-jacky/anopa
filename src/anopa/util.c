
#include <string.h>

void
unslash (char *s)
{
    int l = strlen (s) - 1;
    if (l <= 0)
        return;
    if (s[l] == '/')
        s[l] = '\0';
}
