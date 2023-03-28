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
    quint32 mmddHHMM = 0;

	// Get time as 64-bit integer.
    _time64( &long_time );
    // Convert to local time.
    err = _localtime64_s( &newtime, &long_time );
    if (err)
    {
        printf("Invalid argument to _localtime64_s.");
        return 1;
    }
	
	//printf("%ld\n", (long)long_time);

    // Convert to an ASCII representation.	
    mmddHHMM  = BCD(static_cast<unsigned char>(newtime.tm_mon +1)) << 24;
    mmddHHMM |= BCD(static_cast<unsigned char>(newtime.tm_mday)) << 16;
    mmddHHMM |= BCD(static_cast<unsigned char>(newtime.tm_hour)) << 8;
    mmddHHMM |= BCD(static_cast<unsigned char>(newtime.tm_min ));

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

        // 將 datetime 轉換為 ASCII 碼，並存儲到 sis_fw_data 中
        quint8 *p = (quint8 *)(sis_fw_data + 0x1e000);
        *p++ = datetime_str[0];
        *p++ = datetime_str[1];
        *p++ = datetime_str[2];
        *p++ = datetime_str[3];
        *p++ = datetime_str[4];
        *p++ = datetime_str[5];
        *p++ = datetime_str[6];
        *p++ = datetime_str[7];
}

void print_sep()
{
    printf( "---------------\n" );
}
