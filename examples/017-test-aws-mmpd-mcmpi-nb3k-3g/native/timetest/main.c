#include <stdio.h>
#include <time.h>

struct timespec begin, b0, b1, b2, b3; 
struct timespec end, e0, e1, e2, e3; 

#define myclock(a) clock_gettime(CLOCK_REALTIME, a)
#define elap(a,b) (b.tv_sec - a.tv_sec)

int main () {
    double sum = 0;
    double add = 1;

    // Start measuring time
    //clock_gettime(CLOCK_REALTIME, &begin);
    myclock(&begin);
    
    int iterations = 1010*1300*1200;
    for (int i=0; i<iterations; i++) {
        sum += add;
        add /= 2.0;
    }
    
    // Stop measuring time and calculate the elapsed time
    //clock_gettime(CLOCK_REALTIME, &end);
    myclock(&end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    
    printf("Result: %.20f\n", sum);
    
    printf("Time measured: %.3f seconds.\n", elapsed);
    printf("2Time measured: %.3f seconds.\n", elap(begin, end));
    
    return 0;
}
