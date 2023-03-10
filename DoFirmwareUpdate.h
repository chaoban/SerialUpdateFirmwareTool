#ifndef DOFIRMWAREUPDATE_H
#define DOFIRMWAREUPDATE_H

#include <QTextStream>
#include "ExitStatus.h"

enum SIS_817_POWER_MODE {
    POWER_MODE_ERR = EXIT_ERR,
    POWER_MODE_FWCTRL = 0x50,
    POWER_MODE_ACTIVE = 0x51,
    POWER_MODE_SLEEP  = 0x52
};

enum SIS_817_POWER_MODE sis_get_fw_mode();

int SISUpdateFlow();
void sis_fw_softreset();
bool sis_Switch_Cmd_Mode();
bool sis_Switch_Work_Mode();
bool sis_Check_Fw_Ready();
bool sis_Flash_Rom();
bool sis_Clear_Bootflag();
bool sis_Reset_Cmd();
bool sis_Change_Mode(enum SIS_817_POWER_MODE);
static bool sis_Get_Bootflag(quint32 *);
static bool sis_Get_Fw_Id(quint16 *);
static bool sis_Get_Fw_Info(quint8 *, quint32 *, quint32 *, quint16 *, quint8 *, quint16 *);
static bool sis_Write_Fw_Info(unsigned int, int);
static bool sis_Write_Fw_Payload(const quint8 *, unsigned int);
static bool sis_Update_Block(quint8 *, unsigned int, unsigned int);
static bool sis_Update_Fw(quint8 *, bool);
static bool sis_Get_Bootloader_Id_Crc(quint32 *, quint32 *);
static int sis_Command_For_Write(int , unsigned char *);
static int sis_Command_For_Read(int , unsigned char *);

#endif // DOFIRMWAREUPDATE_H
