#include <stdio.h>
#include <string.h>

/*
 * Copy whole runs from the stdio buffer: memchr finds the newline and
 * memcpy moves the line, instead of one getc() per character.
 * The buffer is refilled through fgetc(), as getc() does.
 */
char *
fgets(char *s, size_t count, FILE *f)
{
    char *ret;
    unsigned char *nl;
    register size_t left, n;
    int ch;

    if (count == 0)
        return 0;
    ret = s;
    left = count - 1;
    while (left > 0) {
        if (f->bufpos >= f->bufread) {
            ch = fgetc(f);
            if (ch == EOF) {
                if (s == ret)
                    return 0;
                break;
            }
            *s++ = (char) ch;
            left--;
            if (ch == '\n')
                break;
            continue;
        }
        n = f->bufread - f->bufpos;
        if (n > left)
            n = left;
        nl = memchr(f->bufpos, '\n', n);
        if (nl)
            n = nl - f->bufpos + 1;
        memcpy(s, f->bufpos, n);
        f->bufpos += n;
        s += n;
        left -= n;
        if (nl)
            break;
    }
    *s = 0;

    if (ferror(f))
        return 0;
    return ret;
}
