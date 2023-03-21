#ifndef DOFIRMWAREUPDATE_H
#define DOFIRMWAREUPDATE_H

#include <QTextStream>
#include <QSerialPort>
#include "ExitStatus.h"

//#define _CHAOBAN_TEST               1
#define _DBG_DISABLE_READCMD        1
#define _PROCESSBAR                 1

enum SIS_817_POWER_MODE {
    POWER_MODE_ERR = EXIT_ERR,
    POWER_MODE_FWCTRL = 0x50,
    POWER_MODE_ACTIVE = 0x51,
    POWER_MODE_SLEEP  = 0x52
};

enum SIS_817_POWER_MODE sis_get_fw_mode();
int verifyRxData(uint8_t *buffer);
int sisUpdateFlow(QSerialPort* serial, quint8 *sis_fw_data, bool bUpdateBootloader, bool bUpdateBootloader_auto, bool bForceUpdate, bool bjump_check);
bool sisSwitchCmdMode(QSerialPort* serial);
bool sisFlashRom(QSerialPort* serial);
bool sisClearBootflag(QSerialPort* serial);
int sisResetCmd(QSerialPort* serial);
bool sis_Change_Mode(enum SIS_817_POWER_MODE);
int sisGetBootflag(QSerialPort* serial, quint32 *);
bool sisGetFwId(QSerialPort* serial, quint16 *);
int sisGetFwInfo(QSerialPort* serial, quint8 *, quint32 *, quint32 *, quint16 *, quint8 *, quint16 *);
bool sisUpdateCmd(QSerialPort* serial, unsigned int, int);
bool sisWriteDataCmd(QSerialPort* serial, const quint8 *, unsigned int);
bool sisUpdateBlock(QSerialPort* serial, quint8 *, unsigned int, unsigned int);
bool burningCode(QSerialPort* serial, quint8 *fn, bool bUpdateBootloader);
bool sisGetBootloaderId_Crc(QSerialPort* serial, quint32 *, quint32 *);
int sisCmdTx(QSerialPort* serial, int , unsigned char *);
int sisCmdRx(QSerialPort* serial, int , unsigned char *);
int verifyRxData(uint8_t *buffer);

#endif // DOFIRMWAREUPDATE_H
