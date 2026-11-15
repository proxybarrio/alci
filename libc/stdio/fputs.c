#include <stdio.h>
#include <string.h>

/*
 * Copy the string into the stream buffer in blocks with memcpy instead of
 * one putc per character, which is slow on 8086 systems. While copying, the
 * stream is switched to full buffering so the buffer is only flushed when
 * full; fputc() is still used whenever the macro could not write (buffer
 * full, not yet set up, after a read, or text translation), so its checks
 * are kept. Then the buffering mode is restored: unbuffered streams are
 * flushed, line-buffered ones flush if a newline was written. Output is
 * identical.
 */
int
fputs(const char *str, FILE *fp)
{
   size_t len = strlen(str);
   register size_t n = len;
   size_t room;
   int buffer_mode = fp->mode & __MODE_BUF;
   int ret = (int)len;

   fp->mode &= ~__MODE_BUF;
   while (n) {
      if (fp->bufpos >= fp->bufwrite) {
         /* let fputc set up, flush or translate */
         if (fputc(*str, fp) == EOF) {
            ret = EOF;
            break;
         }
         str++;
         n--;
         continue;
      }
      room = fp->bufwrite - fp->bufpos;
      if (room > n)
         room = n;
      memcpy(fp->bufpos, str, room);
      fp->bufpos += room;
      str += room;
      n -= room;
   }
   fp->mode |= buffer_mode;
   if (buffer_mode == _IONBF ||
       (buffer_mode == _IOLBF && memchr(str - (len - n), '\n', len - n))) {
      if (fflush(fp))
         ret = EOF;
   }
   if (buffer_mode & (_IOLBF | _IONBF))
      fp->bufwrite = fp->bufstart;     /* putc must call fputc again */
   return ret;
}
