#ifndef DOFIRMWAREUPDATE_H
#define DOFIRMWAREUPDATE_H

#include <QStringList>
#include <QTextStream>
#include <QSerialPort>
#include <QDebug>
#include <stdio.h>
#include "ExitStatus.h"

#define MAX_BYTE    64


enum SIS_817_POWER_MODE {
    POWER_MODE_ERR = EXIT_ERR,
    POWER_MODE_FWCTRL = 0x50,
    POWER_MODE_ACTIVE = 0x51,
    POWER_MODE_SLEEP  = 0x52
};

enum SIS_817_POWER_MODE sis_get_fw_mode();

int SISUpdateFlow();
void sis_fw_softreset();
bool sis_switch_to_cmd_mode();
bool sis_switch_to_work_mode();
bool sis_check_fw_ready();
bool sis_flash_rom();
bool sis_clear_bootflag();
bool sis_reset_cmd();
bool sis_change_fw_mode(enum SIS_817_POWER_MODE);
static bool sis_get_bootflag(quint32 *);
static bool sis_get_fw_id(quint16 *);
static bool sis_get_fw_info(quint8 *, quint32 *, quint32 *, quint16 *, quint8 *, quint16 *);
static bool sis_write_fw_info(unsigned int, int);
static bool sis_write_fw_payload(const quint8 *, unsigned int);
static bool sis_update_block(quint8 *, unsigned int, unsigned int);
static bool sis_update_fw(quint8 *, bool);
static bool sis_get_bootloader_id_crc(quint32 *, quint32 *);
static int sis_command_for_write(int , unsigned char *);
static int sis_command_for_read(int , unsigned char *);

extern QSerialPort serial;
extern void print_sep();
//extern unsigned short crc_check(unsigned char *buf, unsigned char len, unsigned char swap);
extern QByteArray FirmwareString;

//SIS DEBUG
#define SIS_ERR                 -1

//SIS UPDATE FW
#define RAM_SIZE		8*1024
#define PACK_SIZE		52


//SIS CMD DEFINE
#define CMD_SISFLASH           0x81
#define CMD_SISRESET           0x82
#define CMD_SISUPDATE          0x83
#define CMD_SISWRITE           0x84
#define CMD_SISXMODE           0x85
#define CMD_SISREAD            0x86
#define CMD_SISBRIDGE          0x01

#define CMD_SZ_FLASH           0x03 /* Cmd 0x81's Size */
#define CMD_SZ_RESET           0x03 /* Cmd 0x82's Size */
#define CMD_SZ_UPDATE          0x09 /* Cmd 0x83's Size */
#define CMD_SZ_WRITE           0x3B /* Cmd 0x84's Size */
#define CMD_SZ_XMODE           0x05 /* Cmd 0x85's Size */
#define CMD_SZ_READ            0X09 /* Cmd 0x86's Size */
#define CMD_SZ_BRIDGE          0X05 /* Cmd 0x01's Size */

#define READ_SIZE              52

//FW ADDRESS
#define ADDR_FW_INFO            0xA0004000



#define SIS_REPORTID            0x09
#define BUF_ACK_LSB             0x09
#define BUF_ACK_MSB             0x0A
#define BUF_ACK_L				0xEF
#define BUF_ACK_H				0xBE
#define BUF_NACK_L				0xAD
#define BUF_NACK_H				0xDE


#define BUF_PAYLOAD_PLACE       8
#define INT_OUT_REPORT          0x09


#define SIS_GENERAL_TIMEOUT     1000
#define SIS_BOOTFLAG_P810       0x50383130

//GR UART CMD FORMAT
#define GR_UART_ID              0x12
//GR OP CODE
#define BIT_UART_ID             0
#define BIT_OP_LSB              1
#define BIT_OP_MSB              2
#define BIT_SIZE_LSB            3
#define BIT_SIZE_MSB            4

//SIS OP CODE
#define BIT_SISID               0
#define BIT_CRC                 1
#define BIT_CMD                 2
#define BIT_PALD                3


#define GR_OP                   0x80
#define GR_OP_INIT              0x01
#define GR_OP_W                 0x02
#define GR_OP_R                 0x03
#define GR_OP_WR                0x04
#define GR_OP_DISABLE_DEBUG     0x05
#define GR_OP_ENABLE_DEBUG      0x06




#endif // DOFIRMWAREUPDATE_H
