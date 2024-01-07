#include <sys/times.h>
struct timeval tv1, tv2;
#define TIMER_CLEAR   (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec=0)
#define TIMER_START   gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_STOP    gettimeofday(&tv2, (struct timezone*)0)
#define TIMER_ELAPSED (tv2.tv_sec-tv1.tv_sec+(tv2.tv_usec-tv1.tv_usec)*1.E-6)
