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

#endif // DOFIRMWAREUPDATE_H
