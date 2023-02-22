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

//SIS DEBUG
#define SIS_ERR                 -1


//SIS UPDATE FW
#define SIS_REPORTID            0x09
#define BUF_ACK_PLACE_L         4
#define BUF_ACK_PLACE_H         5
#define BUF_ACK_L				0xEF
#define BUF_ACK_H				0xBE
#define BUF_NACK_L				0xAD
#define BUF_NACK_H				0xDE

//GR UART CMD FORMAT
#define GR_UART_ID              0x12

//GR OP CODE
#define GR_OP_MSB               0x80
#define GR_OP_INIT              0x01
#define GR_OP_W                 0x02
#define GR_OP_R                 0x03
#define GR_OP_WR                0x04




#endif // DOFIRMWAREUPDATE_H
