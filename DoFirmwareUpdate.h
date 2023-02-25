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


int Do_Update();
void sis_fw_softreset();
bool sis_switch_to_cmd_mode();
bool sis_switch_to_work_mode();
bool sis_check_fw_ready();
bool sis_flash_rom();
bool sis_clear_bootflag();
bool sis_reset_cmd();
bool sis_change_fw_mode(enum SIS_817_POWER_MODE);
bool sis_get_bootflag(quint32 *);
bool sis_get_fw_id(quint16 *);
bool sis_get_fw_info(quint8 *, quint32 *, quint32 *, quint16 *, quint8 *, quint16 *);
bool sis_write_fw_info(unsigned int, int);
bool sis_write_fw_payload(const quint8 *, unsigned int);
bool sis_update_block(quint8 *, unsigned int, unsigned int);
bool sis_update_fw(quint8 *, bool);
enum SIS_817_POWER_MODE sis_get_fw_mode();

int sis_command_for_write(int , unsigned char *);
int sis_command_for_read(int , unsigned char *);

extern QSerialPort serial;
extern void print_sep();
extern unsigned short crc_check(unsigned char *buf, unsigned char len, unsigned char swap);

//SIS DEBUG
#define SIS_ERR                 -1


//SIS CMD DEFINE
#define SIS_REPORTID            0x09
#define BUF_ACK_LSB             0x09
#define BUF_ACK_MSB             0x0A
#define BUF_ACK_L				0xEF
#define BUF_ACK_H				0xBE
#define BUF_NACK_L				0xAD
#define BUF_NACK_H				0xDE

#define BUF_CRC_PLACE           6
#define BUF_CMD_PLACE           7
#define BUF_PAYLOAD_PLACE       8
#define INT_OUT_REPORT		0x09
#define CMD_81_BYTE             8
#define CMD_82_BYTE             8
#define CMD_85_BYTE             10
#define CMD_83_BYTE             14
#define CMD_86_BYTE             14
#define SIS_GENERAL_TIMEOUT     1000
#define SIS_BOOTFLAG_P810       0x50383130

//GR UART CMD FORMAT
#define GR_UART_ID              0x12
//GR OP CODE
#define GR_OP_MSB               0x80
#define GR_OP_INIT              0x01
#define GR_OP_W                 0x02
#define GR_OP_R                 0x03
#define GR_OP_WR                0x04




#endif // DOFIRMWAREUPDATE_H
