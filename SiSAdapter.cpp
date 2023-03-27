#include <QtGlobal>
#include <time.h>
#include <stdio.h>
#include "SiSAdapter.h"

void getCurrentDateTime(quint8* sis_fw_data, quint32 address);

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

void getCurrentDateTime(quint8* sis_fw_data, quint32 address) {
    // 獲取當前日期和時間
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);

    // 將日期和時間格式化為字串
    char datetime_str[20];
    strftime(datetime_str, sizeof(datetime_str), "%m%d%H%M", tm);

    // 將字串轉換為二進制數據並寫入sis_fw_data
    quint32 datetime;
    sscanf(datetime_str, "%u", &datetime);

    quint8 netlong[4];
    netlong[0] = (datetime >> 24) & 0xff;
    netlong[1] = (datetime >> 16) & 0xff;
    netlong[2] = (datetime >> 8) & 0xff;
    netlong[3] = datetime & 0xff;

    netlong[0] = 0x03;
    netlong[1] = 0x27;
    netlong[2] = 0x18;
    netlong[3] = 0x26;

    memcpy(sis_fw_data + address, netlong, sizeof(netlong));
}

void print_sep()
{
    printf( "---------------\n" );
}
