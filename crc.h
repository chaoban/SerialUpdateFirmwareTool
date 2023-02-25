#ifndef _H_CRC
#define _H_CRC
unsigned short crc16_mmc_upd( unsigned char * buf, unsigned int len, unsigned short crc_old );
unsigned short crc_check(unsigned char *buf, unsigned char len, unsigned char swap);
#endif
