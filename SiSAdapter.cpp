#include <QtGlobal>
#include <time.h>
#include <stdio.h>
#include "SiSAdapter.h"

QByteArray FirmwareString;
DateTime getCurrentDateTime();

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

DateTime getCurrentDateTime()
{
    // 取得當前日期時間
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 取得年、月、日、時、分
    DateTime dateTime;
    dateTime.year = static_cast<quint16>(currentDateTime.date().year());
    dateTime.month = static_cast<quint8>(currentDateTime.date().month());
    dateTime.day = static_cast<quint8>(currentDateTime.date().day());
    dateTime.hour = static_cast<quint8>(currentDateTime.time().hour());
    dateTime.minute = static_cast<quint8>(currentDateTime.time().minute());

    // 回傳日期時間
    return dateTime;
}

/*
 * 列印特定位址起的數值
 * quint8* sis_fw_data: // FW資料
 * const char* str,     // 要輸出的說明
 * quint32 address,     // 列印開始位置
 * int length,          // 列印長度(Bytes)
 * bool bcb             // 是否轉為BCB。true: BCB; false: 16進制
 */
void printAddrData(quint8* sis_fw_data, const char* str, quint32 address, int length, bool bcb) {

    assert((address + length) <= (unsigned int)FirmwareString.length());
    if ((address + length) > (unsigned int)FirmwareString.length()) {
        return;
    }

    quint8 data[length];

    printf("%s: ", str);
    for (int i = 0; i < length; i++) {
        data[i] = *(sis_fw_data + address + i);
        if (bcb)
            printf("%c", isprint(data[i]) ? data[i] : '.');
        else
            printf("%02x ", data[i]);
    }

    printf("\n");
}

void print_sep()
{
    printf( "---------------\n" );
}
