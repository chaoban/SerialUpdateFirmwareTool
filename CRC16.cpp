#include<QDebug>
#include"CRC16.h"
#include"DoFirmwareUpdate.h"

#define u8 quint8
#define u16 quint16

quint8 sis_calculate_output_crc( u8* buf, int len );

uint16_t cal_crc(unsigned char *cmd, int start, int end)
{
    int i = 0;
    uint16_t crc = 0;
    for (i = start; i <= end ; i++)
        crc = (crc<<8) ^ crc16tab[((crc>>8) ^ cmd[i])&0x00FF];
    return crc;
}

uint16_t cal_crc_with_cmd (char* data, int start, int end, uint8_t cmd)
{
    int i = 0;
    uint16_t crc = 0;

    crc = (crc<<8) ^ crc16tab[((crc>>8) ^ cmd)&0x00FF];
    for (i = start; i <= end ; i++)
        crc = (crc<<8) ^ crc16tab[((crc>>8) ^ data[i])&0x00FF];
    return crc;

}

void write_crc(unsigned char *buf, int start, int end)
{
    uint16_t crc = 0;
    crc = cal_crc(buf, start , end);
    buf[end+1] = (crc >> 8) & 0xff;
    buf[end+2] = crc & 0xff;
}

quint8 sis_calculate_output_crc( u8* buf, int len )
{
    u16 crc;
    u8 *cmd, *payload;
    cmd = (buf + BIT_CMD);
        payload = (buf + BUF_PAYLOAD_PLACE);
        crc = crc_itu_t(0x0000, cmd, 1);
        crc = crc_itu_t(crc, payload, len - BUF_PAYLOAD_PLACE);
    crc = crc & 0xff;
        return crc;
}
