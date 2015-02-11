
#include <anopa/ga_int_list.h>

int
add_to_list (genalloc *list, int si, int check_for_dupes)
{
    if (check_for_dupes)
    {
        int len = genalloc_len (int, list);
        int i;

        for (i = 0; i < len; ++i)
            if (list_get (list, i) == si)
                return 0;
    }

    genalloc_append (int, list, &si);
    return 1;
}

int
remove_from_list (genalloc *list, int si)
{
    int len = genalloc_len (int, list);
    int i;

    for (i = 0; i < len; ++i)
        if (list_get (list, i) == si)
        {
            ga_remove (int, list, i);
            return 1;
        }

    return 0;
}

int
is_in_list (genalloc *list, int si)
{
    int len = genalloc_len (int, list);
    int i;

    for (i = 0; i < len; ++i)
        if (list_get (list, i) == si)
            return 1;
    return 0;
}
