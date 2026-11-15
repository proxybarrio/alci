#include <time.h>
#include <sys/time.h>

struct tm * localtime (const time_t * timep)
{
   static struct tm tmb;
   time_t offt;

   /* the kernel timezone was never used here: it was overwritten by TZ */
   if (!_tz_is_set)
	tzset();
   offt = -(timezone / 60) * 60L;

   /* tmb.tm_isdst = ? */
   __tm_conv(&tmb, timep, offt);

   return &tmb;
}
