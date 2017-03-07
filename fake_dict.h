#define new_table() \
({ \
     struct url_table *p = (struct url_table *) \
            malloc(sizeof(struct url_table)); \
     if (p == NULL) { \
         perror("malloc() failed"); \
         exit(1); \
     } \
     p; \
})

struct url_table {
    char id[20], url[200];
    struct url_table *next;
};

struct url_table *set(struct url_table *table, char *id, char *url);
char *get(struct url_table *table, char *id);
