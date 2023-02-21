#include "DoFirmwareUpdate.h"

/**********************************************
**　Update Flow
**
** 1.	Switch FW Mode
** 2.	Get FW Information
** 3.	Check FW Info
** 4.	Check BootFlag
** 5.	Check Bootloader ID and Bootloader CRC
** 6.	Update FW
**
**********************************************/


bool sis_switch_to_cmd_mode()
{
    return EXIT_OK;
}

bool sis_switch_to_work_mode()
{
    return EXIT_OK;
}

/*
 * Check Chip Status
 * Return: Ture is chip on the work function, else is chip not ready.
 */
bool sis_check_fw_ready()
{
    return true;
}


/*
 * Change Chip to Power Mode
 * Use this function to change chip power mode.
 * mode:POWER_MODE_FWCTRL, power control by FW.
 * POWER_MODE_ACTIVE, chip always work on time.
 * POWER_MODE_SLEEP,  chip on the sleep mode.
 * Return:Ture is change power mode success.
 */
bool sis_change_fw_mode(enum SIS_817_POWER_MODE mode)
{
    switch(mode)
    {
        case POWER_MODE_FWCTRL:
            break;
        case POWER_MODE_ACTIVE:
            break;
        case POWER_MODE_SLEEP:
            break;
        default:
            break;
    }

    return EXIT_OK;
}


/*
 * Get Chip status
 * Use this function to:
 * Return:-1 is get firmware work status error.
 * POWER_MODE_FWCTRL, power control by FW.
 * POWER_MODE_ACTIVE, chip always work on time.
 * POWER_MODE_SLEEP,  chip on the sleep mode.
 */
enum SIS_817_POWER_MODE sis_get_fw_mode()
{
    uint8_t tmpbuf[MAX_BYTE] = {0};
    switch(tmpbuf[10])
    {
        case POWER_MODE_FWCTRL:
            return POWER_MODE_FWCTRL;
        case POWER_MODE_ACTIVE:
            return POWER_MODE_ACTIVE;
        case POWER_MODE_SLEEP:
            return POWER_MODE_SLEEP;
        default:
            return POWER_MODE_ERR;
            break;
     }
}

/*
 * Software Reset
 */
void sis_fw_softreset()
{

}

//static bool sis_get_bootflag(quint32 *bootflag)
bool sis_get_bootflag(quint32 *bootflag)
{
    quint8 tmpbuf[MAX_BYTE] = {0};
    *bootflag = (tmpbuf[8] << 24) | (tmpbuf[9] << 16) | (tmpbuf[10] << 8) | (tmpbuf[11]);
    return EXIT_OK;
}

//static bool sis_get_fw_id(quint16 *fw_version)
static bool sis_get_fw_id(quint16 *fw_version)
{
    quint8 tmpbuf[MAX_BYTE] = {0};
    *fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
    return EXIT_OK;
}

//static bool sis_get_fw_info(quint8 *chip_id, quint32 *tp_size, quint32 *tp_vendor_id, quint16 *task_id, quint8 *chip_type, quint16 *fw_version)
bool sis_get_fw_info(quint8 *chip_id, quint32 *tp_size, quint32 *tp_vendor_id, quint16 *task_id, quint8 *chip_type, quint16 *fw_version)
{
    return EXIT_OK;
}

//static bool sis_write_fw_info(unsigned int addr, int pack_num)
bool sis_write_fw_info(unsigned int addr, int pack_num)
{
    return EXIT_OK;
}

//static bool sis_write_fw_payload(const quint8 *val, unsigned int count)
bool sis_write_fw_payload(const quint8 *val, unsigned int count)
{
    return EXIT_OK;
}

//static bool sis_flash_rom()
bool sis_flash_rom()
{
    return EXIT_OK;
}

//static bool sis_clear_bootflag()
bool sis_clear_bootflag()
{
    return EXIT_OK;
}

//static bool sis_update_block(quint8 *data, unsigned int addr, unsigned int count)
bool sis_update_block(quint8 *data, unsigned int addr, unsigned int count)
{
    return EXIT_OK;
}

//static bool sis_reset_cmd()
bool sis_reset_cmd()
{
    return EXIT_OK;
}

//static bool sis_update_fw(quint8 *fn, bool update_boot)
bool sis_update_fw(quint8 *fn, bool update_boot)
{
    return EXIT_OK;
}

int Do_Update()
{
    qDebug() << "Update Firmware by" <<  serial.portName() << "port";
    /*
     * Switch FW Mode
     * sis_switch_to_cmd_mode
     */
    printf("Switch FW Mode\n");
    print_sep();




    /*
     * Get FW Information and Check FW Info
     * sis_get_fw_info
     */
    printf("Get FW Information and Check FW Info\n");
    print_sep();


    /*
     * Check FW Info
     */
    printf("Check FW Info\n");
    print_sep();



    /*
     * Check BootFlag
     * sis_get_bootflag
     */
    printf("Check BootFlag\n");
    print_sep();





    /*
     * Check Bootloader ID and Bootloader CRC
     * sis_get_bootloader_id_crc
     */
    printf("Check Bootloader ID and Bootloader CRC\n");
    print_sep();




    /*
     * Update FW
     * sis_update_fw
     */
    printf("START FIRMWARE UPDATE!!!\n");
    print_sep();



    return EXIT_OK;
}
