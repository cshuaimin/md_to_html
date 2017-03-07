#include <stdio.h> /* for perror() */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fake_dict.h"

struct url_table *set(struct url_table *table, char *id, char *url)
{
    struct url_table *end;
    if (table == NULL)
        end = table = new_table();
    else {
        for (struct url_table *p = table; p != NULL; p = p->next) {
            if (p->next == NULL)
                end = p;
            if (strcasecmp(p->id, id) == 0) {
                end = p;
                goto outer;
            }
        }
        end->next = new_table();
        end = end->next;
        end->next = NULL;
    }
outer:
    strcpy(end->id, id);
    strcpy(end->url, url);
    return table;
}

char *get(struct url_table *table, char *id)
{
    for (; table != NULL; table = table->next)
        if (strcasecmp(table->id, id) == 0)
            return table->url;
    return NULL;
}

/*
int main()
{
    struct url_table *d = NULL;
    d = set(d, "hello", "world");
    d = set(d, "YCM", "Vim");
    printf("%s\n%s\n", get(d, "hello"), get(d, "YCM"));
    return 0;
}
*/
