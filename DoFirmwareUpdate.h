#ifndef DOFIRMWAREUPDATE_H
#define DOFIRMWAREUPDATE_H

#include <QTextStream>
#include <QSerialPort>
#include "ExitStatus.h"

//#define _CHAOBAN_TEST               1
#define _DBG_DISABLE_READCMD        1
#define _DBG_DISABLE_CHECKFW        1

enum SIS_817_POWER_MODE {
    POWER_MODE_ERR = EXIT_ERR,
    POWER_MODE_FWCTRL = 0x50,
    POWER_MODE_ACTIVE = 0x51,
    POWER_MODE_SLEEP  = 0x52
};

enum SIS_817_POWER_MODE sis_get_fw_mode();

int SISUpdateFlow(QSerialPort* serial, quint8 *sis_fw_data, bool update_bootloader, bool force_update);
bool sis_Switch_Cmd_Mode(QSerialPort* serial);
bool sis_Flash_Rom(QSerialPort* serial);
bool sis_Clear_Bootflag(QSerialPort* serial);
int sis_Reset_Cmd(QSerialPort* serial);
bool sis_Change_Mode(enum SIS_817_POWER_MODE);
static int sis_Get_Bootflag(QSerialPort* serial, quint32 *);
static bool sis_Get_Fw_Id(QSerialPort* serial, quint16 *);
static int sis_Get_Fw_Info(QSerialPort* serial, quint8 *, quint32 *, quint32 *, quint16 *, quint8 *, quint16 *);
static bool sis_Write_Fw_Info(QSerialPort* serial, unsigned int, int);
static bool sis_Write_Fw_Payload(QSerialPort* serial, const quint8 *, unsigned int);
static bool sis_Update_Block(QSerialPort* serial, quint8 *, unsigned int, unsigned int);
static bool sis_Update_Fw(QSerialPort* serial, quint8 *fn, bool update_bootloader);
static bool sis_Get_Bootloader_Id_Crc(QSerialPort* serial, quint32 *, quint32 *);
static int sisCmdTx(QSerialPort* serial, int , unsigned char *);
static int sisCmdRx(QSerialPort* serial, int , unsigned char *);
int verifyRxData(uint8_t *buffer);

#endif // DOFIRMWAREUPDATE_H
