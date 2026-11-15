#include <time.h>
#include <sys/time.h>

char *
ctime(const time_t *timep)
{
  time_t offt;
  struct tm tmb;
  static char cbuf[30];

  /* the kernel timezone was never used here: it was overwritten by TZ */
  if (!_tz_is_set)
	tzset();
  offt = -(timezone / 60) * 60L;
  
  /* tmb.tm_isdst = ? */
  __tm_conv(&tmb, timep, offt);
  
  __asctime(cbuf, &tmb);
  
  return cbuf;
}
