#include "crc.h"

/*
 * USAGE CRC16
 * unsigned char crc16 = crc_check(((unsigned char *)&I2cTxPacket) + 4, m_HostPkgLen, 1)
 */

unsigned short crc16_mmc_upd( unsigned char * buf, unsigned int len, unsigned short crc_old )
{
    unsigned char cnt_bits, flag_xor, data;
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        data = *buf;
        for (cnt_bits = 8; cnt_bits; cnt_bits--) {
            flag_xor = ((crc_old >> 8) & 0x80) ^ (data & 0x80);
            data <<= 1;
            crc_old <<= 1;
            if (flag_xor)
                crc_old ^= 0x1021;
        }
        buf++;
    }
    return (crc_old);
}

unsigned short  crc_check(unsigned char* buf,unsigned char len,unsigned char swap  )
{
    unsigned char   lenght_cnt = 0;
    unsigned char   i = 0;
    unsigned short  crc16_check = 0;
    unsigned char  *buf_crc = buf;


    if ( swap == 0x1 ) {
        buf++;
        crc16_check = crc16_mmc_upd ( buf, 1, 0 );
        lenght_cnt = ( len - 3 );
        buf = buf + 3;

        for ( i = 0; i < lenght_cnt; i++ ) {
            if ( ( i & 0x3 ) == 0x0 ) {
                buf_crc = buf + 3;
            }

            crc16_check = crc16_mmc_upd ( buf_crc, 1, crc16_check );

            buf++;
            buf_crc = buf_crc - 1;
        }
    }

    else if ( swap == 0x0 ) {
        buf = buf + 2;
        crc16_check = crc16_mmc_upd ( buf, 1, 0 );
        lenght_cnt = ( len - 3 );
        buf = buf + 2;

        for ( i = 0; i < lenght_cnt; i++ ) {
            crc16_check = crc16_mmc_upd ( buf, 1, crc16_check );
            buf++;
        }

    }

    return crc16_check;
}


