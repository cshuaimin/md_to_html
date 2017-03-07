#define _GNU_SOURCE /* for asprintf() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include "fake_dict.h" /* ~_~ */

#define is_ulist_sign(line) \
    ((line[0] == '*' || line[0] == '+' || line[0] == '-') && line[1] == ' ')
#define is_olist_sign(line) \
    (isdigit(line[0]) && line[1] == '.' && line[2] == ' ')
#define is_division(c) \
    (c == '*' || c == '-' || c == '_')
#define is_blockquotes(line) \
    (line[0] == '>' && line[1] == ' ')

#define trim(s) \
({ \
    /* Ensure that we evaluate `s` for once, because `s` may be \
     * a function call which have side effect - such as fgets().\
     */ \
    char *_s = (s); \
    char *end = _s + strlen(_s) - 1; \
    while (isblank(*end) || *end == '\n') \
        end--; \
    *(end + 1) = 0; \
    while (isblank(*_s)) \
        _s++; \
    _s; \
})

#define remove_newline(s) \
({ \
    char *_s = (s); \
    char *end = _s + strlen(_s) - 1; \
    if (*end == '\n') \
        *end = 0; \
    _s; \
})

/* Same as asprintf(), but append fmt to str instead of repleacing. */
#define astrcatf(str, fmt, args...) \
({ \
    if (str == NULL) \
        asprintf(&(str), fmt, args); \
    else { \
        char *_buf = NULL; \
        asprintf(&_buf, "%s", str); \
        asprintf(&(str), "%s" fmt, _buf, ## args); \
        free(_buf); \
    } \
})

#define astrcat(str, src) \
({ \
    if (str == NULL) \
        asprintf(&(str), "%s", src); \
    else { \
        char *_buf = NULL; \
        asprintf(&_buf, "%s", str); \
        asprintf(&(str), "%s%s", _buf, src); \
        free(_buf); \
    } \
})

char *get_next_line();
char *get_quoted_str(char *s, char left, char right);
char *handle_link(char *text);
char *handle_line_attributes(char *line);
void handle_list(char *line, char type);

struct url_table *named_urls = NULL;
FILE *in_file;
char *out = NULL;

#define copy_line_to_out() \
({ \
    astrcatf(out, "%s%s\n",in_para ? "" : "<p>", \
            handle_line_attributes(line)); \
    in_para = true; \
})

int main(int argc, char *argv[])
{
    FILE *out_file;
    if (argc == 2)
        out_file = stdout;
    else if (argc != 3) {
        fprintf(stderr, "Usage: %s INPUT [OUTPUT]\n", argv[0]);
        return 1;
    }
    else {
        out_file = fopen(argv[2], "w");
        if (out_file == NULL) {
            perror("Failed to open output file");
            return 1;
        }
    }
    in_file = fopen(argv[1], "r");
    if (in_file == NULL) {
        perror("Failed to open input file");
        return 1;
    }

    bool in_para = false;
    char *line;

    while ((line = get_next_line()) != NULL) {
        int ending_space_num = 0;
        int i;
        remove_newline(line);
        for (i = strlen(line) - 1; i >= 0; i--) {
            if (line[i] != ' ')
                break;
            ending_space_num++;
        }

        line = trim(line);
        if (*line == 0) {
            if (in_para) {
                remove_newline(out);
                astrcat(out, "</p>\n");
                in_para = false;
            }
            continue;
        }

        if (line[0] == '#') {
            int title_level = 1;
            while (line[title_level] == '#')
                title_level++;
            char *title_text = trim(line + title_level);
            char *verbose_chars = strchr(title_text, '#');
            if (verbose_chars != NULL) {
                if (verbose_chars[-1] == ' ')
                    verbose_chars--;
                *verbose_chars = 0;
            }
            astrcatf(out, "<h%d>%s</h%d>\n",
                    title_level, title_text, title_level);
        }
        else if(is_ulist_sign(line)) {
            handle_list(line, 'u');
        }
        else if (is_olist_sign(line)) {
            handle_list(line, 'o');
        }
        else if (is_division(line[0])) {
            bool division = false;
            for (int i = 1; i < 3; i++) {
                if (!is_division(trim(line)[i])) {
                    division = false;
                    break;
                }
                division = true;
            }
            if (division)
                astrcat(out, "<hr />\n");
            else
                copy_line_to_out();
        }
        else if (line[0] == '[') {
            char *next = strstr(line, "]:");
            if (next != NULL) {
                *next = 0;
                named_urls = set(named_urls, line + 1, trim(next + 2));
            }
            else
                copy_line_to_out();
        }
        else if (is_blockquotes(line)) {
            /* Nested function - a GCC extension. */
            void handle_blockquotes(char *line)
            {
                while (*line != 0) {
                    if (is_blockquotes(line)) {
                        line += 2;
                        remove_newline(out);
                        astrcat(out, "\n<blockquotes>\n");
                        handle_blockquotes(line);
                        remove_newline(out);
                        astrcat(out, "\n</blockquotes>\n");
                        break;
                    }
                    else
                        *out++ = *line++;
                }
                *out = 0;
            }

            remove_newline(out);
            astrcat(out, "\n<blockquotes>\n");
            do {
                if (is_blockquotes(line))
                    line = trim(line + 2);
                handle_blockquotes(handle_line_attributes(line));
                line = get_next_line();
                if (line == NULL)
                    break;
                line = trim(line);
            } while (*line != 0);
            astrcat(out, "\n</blockquotes>\n");
        }
        else
            copy_line_to_out();

        if (ending_space_num >= 2) {
            remove_newline(out);
            astrcat(out, "<br />\n");
        }
    }
    if (in_para) {
        remove_newline(out);
        astrcat(out, "</p>");
    }

    fclose(in_file);

    char *result = handle_link(out);
    free(out);
    remove_newline(result);
    strcat(result, "\n");
    fputs(result, out_file);
    fclose(out_file);

    return 0;
}

char *get_next_line()
{
    static char *line = NULL;
    static size_t len = 0;
    ssize_t read = getline(&line, &len, in_file);
    if (read == -1) {
        free(line);
        line = NULL;
        return NULL;
    }
    return line;
}

void handle_list(char *line, char type)
{
    astrcatf(out, "<%cl>\n", type);
    do {
        astrcatf(out, "<li>%s</li>\n",
                handle_line_attributes(trim(line+2)));
        line = trim(get_next_line());
    } while (type == 'u' ? is_ulist_sign(line) : is_olist_sign(line));
    astrcatf(out, "</%cl>\n", type);
}

char *get_quoted_str(char *s, char left, char right)
{
    char *start = strchr(s, left);
    if (start == NULL)
        return NULL;
    start++;
    char *finish = strchr(start, right);
    if (finish == NULL)
        return NULL;
    *finish = 0;
    return start;
}

#define deal_with_common_chars() \
({ \
    strcpy(buf, text); \
    return handle_link_buf; \
})
#define OUT_LEN 512
char handle_link_buf[OUT_LEN];
char *handle_link(char *text)
{
    char *buf = handle_link_buf;
    while (*text != 0) {
        char *link_text = get_quoted_str(text, '[', ']');
        if (link_text == NULL)
            deal_with_common_chars();
        *(link_text - 1) = 0;
        buf += sprintf(buf, "%s", text);
        char *link = trim(link_text + strlen(link_text) + 1);
        char *title;
        if (*link == '(') {
            link++;
            char *link_finish = strchr(link, ')');
            if (link_finish == NULL)
                deal_with_common_chars();
            *link_finish = 0;
            title = get_quoted_str(link, '\"', '\"');
            if (title != NULL)
                *(title - 1) = 0;
            link = trim(link);
            text = link_finish + 1;
        }
        else if (*link == '[') {
            char *id_finish = strchr(link, ']');
            if (id_finish == NULL)
                deal_with_common_chars();
            link++;
            if (link == id_finish)
                link = link_text;
            else
                *id_finish = 0;
            link = get(named_urls, link);
            if (link == NULL)
                deal_with_common_chars();
            title = get_quoted_str(link, '"', '"');
            if (title != NULL)
                *(title - 1) = 0;
            link = trim(link);
            text = id_finish + 1;
        }
        if (title)
            buf += sprintf(buf, "<a href=\"%s\" title=\"%s\">%s</a>",
                    link, title, link_text);
        else
            buf += sprintf(buf, "<a href=\"%s\">%s</a>",
                    link, link_text);
    }
    *buf = 0;
    return handle_link_buf;
}


#define LINE_ATTRS_NUM 5
struct {
    char md[3];
    int mdlen;
    char htmlleft[9];
    char htmlright[10];
} md_and_html[LINE_ATTRS_NUM] = {
    {"**", 2, "<strong>", "</strong>"},
    {"__", 2, "<strong>", "</strong>"},
    {"*", 1, "<em>", "</em>"},
    {"_", 1, "<em>", "</em>"},
    {"`", 1, "<code>", "</code>"}
};

#define LINE_LEN 128
char handle_line_attributes_buf[LINE_LEN * 2];
char *handle_line_attributes(char *line)
{
    char *buf = handle_line_attributes_buf;
    while (*line != 0) {
        bool matched = false;
        for (int i = 0; i < LINE_ATTRS_NUM; i++) {
            if (strncmp(line, md_and_html[i].md,
                        md_and_html[i].mdlen) == 0)
            {
                line += md_and_html[i].mdlen;
                char *end = strstr(line, md_and_html[i].md);
                assert(end);
                *end = 0;
                buf += sprintf(buf, "%s%s%s", md_and_html[i].htmlleft,
                        line, md_and_html[i].htmlright);
                line = end + md_and_html[i].mdlen;

                matched = true;
                break;
            }
        }
        if (!matched)
            *buf++ = *line++;
    }
    *buf = 0;
    return handle_line_attributes_buf;
}
