#include "sis_command.h"
#include <QtGlobal>

void sis_Make_83_Buffer( quint8 *buf, unsigned int addr, int pack_num );
void sis_Make_84_Buffer( quint8 *buf, const quint8 *val, int count );
extern quint8 sis_Calculate_Output_Crc( quint8* buf, int len );

void sis_Make_83_Buffer( quint8 *buf, unsigned int addr, int pack_num )
{
    int len = CMD_SZ_UPDATE;

    *buf = SIS_REPORTID;
    //*(buf + BIT_CRC) = 0x0;/* CRC*/
    *(buf + BIT_CMD) = CMD_SISUPDATE;
    *(buf + 3) = addr & 0xff;
    *(buf + 4) = (addr >> 8) & 0xff;
    *(buf + 5) = (addr >> 16) & 0xff;
    *(buf + 6) = (addr >> 24) & 0xff;
    *(buf + 7) = pack_num & 0xff;
    *(buf + 8) = (pack_num >> 8) & 0xff;

    // crc
    *(buf + BIT_CRC) = sis_Calculate_Output_Crc( buf, len );
}

void sis_Make_84_Buffer( quint8 *buf, const quint8 *val, int count )
{
    int i, j, k;
    int len = BIT_PALD + count;

    *buf = SIS_REPORTID;
    //*(buf + BIT_CRC) = 0x0;/* CRC*/
    *(buf + BIT_CMD) = CMD_SISWRITE;

    for (i = 0; i < count; i+=4) {
        k = i + 3;
        for (j = i; j < i+4; j++) {
            *(buf + BIT_PALD + j) = *(val + k) & 0xff;
            k--;
        }
    }
    if ((count % 4) != 0) {
        k = count / 4 * 4;
        for (j = count-1; j >= (count/4*4); j--) {
            *(buf + BIT_PALD + j) = *(val + k) & 0xff;
            k++;
        }
    }
    // crc
    *(buf + BIT_CRC) = sis_Calculate_Output_Crc( buf, len );
}

