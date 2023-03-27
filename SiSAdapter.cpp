#include <time.h>
#include <stdio.h>
#include "SiSAdapter.h"

static inline unsigned char BCD (unsigned char x)
{
    return ((x / 10) << 4) | (x % 10);
}

int getTimestamp()
{
    struct tm newtime;
    __time64_t long_time;
    errno_t err;
    int mmddHHMM = 0;

	// Get time as 64-bit integer.
    _time64( &long_time );
    // Convert to local time.
    err = _localtime64_s( &newtime, &long_time );
    if (err)
    {
        printf("Invalid argument to _localtime64_s.");
        return 1;
    }
	
	printf("%ld\n", (long)long_time);

    // Convert to an ASCII representation.	
    mmddHHMM  = BCD(static_cast<unsigned char>(newtime.tm_mon +1))<<24;
    mmddHHMM |= BCD(static_cast<unsigned char>(newtime.tm_mday))<<16;
    mmddHHMM |= BCD(static_cast<unsigned char>(newtime.tm_hour))<<8;
    mmddHHMM |= BCD(static_cast<unsigned char>(newtime.tm_min ));
    //int year = BCD(static_cast<unsigned char>(newtime.tm_year));

    return mmddHHMM;
}

void print_sep()
{
    printf( "---------------\n" );
}
