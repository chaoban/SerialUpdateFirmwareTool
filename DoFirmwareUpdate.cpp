#include "DoFirmwareUpdate.h"
#include "QDebug"


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

/*
 * I2C Read Write Commands
 */
//static int sis_command_for_write(int wlength, unsigned char *wdata)
int sis_command_for_write(int wlength, unsigned char *wdata)
{
    QTextStream standardOutput(stdout);
    QByteArray writeData, appendData;
    int ret = SIS_ERR;

/*
    for (int i=0; i < wlength; i++) {
        printf("wdata[%i] = %x\n", i, wdata[i]);
    }
*/

    //appendData = QByteArray((char*)wdata, wlength);
    appendData = QByteArray(reinterpret_cast<char*>(wdata), wlength);

    writeData.resize(5);
    writeData[0] = GR_UART_ID;
    writeData[1] = GR_OP_WR;
    writeData[2] = 0x80;
    writeData[3] = 0x05;
    writeData[4] = 0x00;

    writeData.append(appendData);

/*
    for (int i =0; i < writeData.length(); i++) {
        qDebug() << writeData[i];
    }
*/
    const qint64 bytesWritten = serial.write(writeData);

    if (bytesWritten == -1) {
        standardOutput << QObject::tr("Failed to send the command to port %1, error: %2")
                          .arg(serial.portName(), serial.errorString()) << Qt::endl;
        ret = EXIT_ERR;
    } else if (bytesWritten != writeData.size()) {
        standardOutput << QObject::tr("Failed to send all the command to port %1, error: %2")
                          .arg(serial.portName(), serial.errorString()) << Qt::endl;
        ret = EXIT_ERR;
    } else if (!serial.waitForBytesWritten(5000)) {
        standardOutput << QObject::tr("Operation timed out or an error "
                                      "occurred for port %1, error: %2")
                          .arg(serial.portName(), serial.errorString()) << Qt::endl;
        ret = EXIT_ERR;
    }

    return ret;
}

//static int sis_command_for_read(int rlength, unsigned char *rdata)
int sis_command_for_read(int rlength, unsigned char *rdata)
{
    int ret = SIS_ERR;
/*
    struct i2c_msg msg[1];
    msg[0].addr = client->addr;
    msg[0].flags = I2C_M_RD;//Read
    msg[0].len = rlength;
    msg[0].buf = rdata;
    ret = i2c_transfer(client->adapter, msg, 1);
*/




    return ret;
}


/*
 * Change Mode
 */

bool sis_switch_to_cmd_mode()
{
    int ret = -1;
    uint8_t tmpbuf[MAX_BYTE] = {0};
    uint8_t sis817_cmd_active[5] = {SIS_REPORTID, 0x00/*CRC16*/, 0x85, 0x51, 0x09};
    //uint8_t test_cmd[4] = {0x50, 0x38, 0x31, 0x30};
    uint8_t sis817_cmd_enable_diagnosis[5] = {SIS_REPORTID, 0x00/*CRC16*/, 0x85, 0x21, 0x01};

    //Send 85 CMD - PWR_CMD_ACTIVE
    //ret = sis_command_for_write(sizeof(test_cmd), test_cmd);
    ret = sis_command_for_write(sizeof(sis817_cmd_active), sis817_cmd_active);
    if(ret < 0){
        qDebug() << "SiS SEND Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n";
        return false;
    }

    ret = sis_command_for_read(sizeof(tmpbuf), tmpbuf);
    if(ret < 0){
        qDebug() <<"SiS READ Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n";
        return false;
    }

    if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
        qDebug() << "SiS SEND Switch CMD Return NACK - 85(PWR_CMD_ACTIVE)\n";
        return false;
     }
     else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
        qDebug() << "SiS SEND Switch CMD Return Unknow- 85(PWR_CMD_ACTIVE)\n";
        return false;
     }

     //msleep(100);
     memset(tmpbuf, 0, sizeof(tmpbuf));

     //Send 85 CMD - ENABLE_DIAGNOSIS_MODE
     ret = sis_command_for_write(sizeof(sis817_cmd_enable_diagnosis), sis817_cmd_enable_diagnosis);
     if(ret < 0){
        qDebug() << "SiS SEND Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
      }

     ret = sis_command_for_read(sizeof(tmpbuf), tmpbuf);
     if(ret < 0){
        qDebug() << "SiS READ Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
     }

     if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
        qDebug() << "SiS SEND Switch CMD Return NACK - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
     }else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
        qDebug() << "SiS SEND Switch CMD Return Unknow- 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
     }

    //msleep(50);
    return true;
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
     */
    printf("Switch FW Mode\n");
    print_sep();

    if (!sis_switch_to_cmd_mode())
    {
        return EXIT_ERR;
    }




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
