/**********************************************
**　Update Flow
**
** 1.	Switch FW Mode
** 2.	Check FW Info
** 3.	Check BootFlag
** 4.	Check Bootloader ID and Bootloader CRC
** 5.	Update FW
**
**********************************************/

#include "DoFirmwareUpdate.h"
#include "QDebug"


extern quint8 sis_calculate_output_crc( quint8* buf, int len );
//QByteArray sis_fw_data;
uint8_t sis_fw_data[] = {
//TODO: BINARY FILE
};

//#define CHAOBAN_TEST    1

/*
 * I2C Read Write Commands
 */
//static int sis_command_for_write(int wlength, unsigned char *wdata)
int sis_command_for_write(int wlength, unsigned char *wdata)
{
    QTextStream standardOutput(stdout);
    QByteArray writeData, appendData;
    int ret = EXIT_OK;

/*
    for (int i=0; i < wlength; i++) {
        printf("wdata[%i] = %x\n", i, wdata[i]);
    }
*/

    appendData = QByteArray::fromRawData((char*)wdata, wlength);
    //appendData = QByteArray((char*)wdata, wlength);
    //appendData = QByteArray(reinterpret_cast<char*>(wdata), wlength);

    // chaoban test
    //wlength = 300;//2C 01

#ifdef CHAOBAN_TEST
    writeData.resize(4);
    writeData[0] = 0x50;
    writeData[1] = 0x38;
    writeData[2] = 0x31;
    writeData[3] = 0x30;
#else
    writeData.resize(5);
    writeData[BIT_UART_ID] = GR_UART_ID;
    writeData[BIT_OP_LSB] = GR_OP_WR;
    writeData[BIT_OP_MSB] = GR_OP;
    writeData[BIT_SIZE_LSB] = (wlength & 0xff);
    writeData[BIT_SIZE_MSB] = ((wlength >> 8 ) & 0xff);
#endif

    writeData.append(appendData);

#ifdef CHAOBAN_TEST
    for (int i =0; i < writeData.length(); i++) {
        qDebug() << writeData[i] << " ";
    }
#endif

#ifdef CHAOBAN_TEST
    qDebug() << "Write:" << writeData;
#endif

    const qint32 bytesWritten = serial.write(writeData);

    if (bytesWritten == -1) {
        standardOutput << QObject::tr("Failed to send the command to port %1, error: %2")
                          .arg(serial.portName(), serial.errorString()) << Qt::endl;
        ret = SIS_ERR;
    } else if (bytesWritten != writeData.size()) {
        standardOutput << QObject::tr("Failed to send all the command to port %1, error: %2")
                          .arg(serial.portName(), serial.errorString()) << Qt::endl;
        ret = SIS_ERR;
    } else if (!serial.waitForBytesWritten(5000)) {
        standardOutput << QObject::tr("Operation timed out or an error "
                                      "occurred for port %1, error: %2")
                          .arg(serial.portName(), serial.errorString()) << Qt::endl;
        ret = SIS_ERR;
    }

#ifdef CHAOBAN_TEST
    qDebug() << "bytesWritten=" << bytesWritten;
#endif
    return ret;
}

//static int sis_command_for_read(int rlength, unsigned char *rdata)
int sis_command_for_read(int rlength, unsigned char *rdata)
{
    int ret = EXIT_OK;

    //const QByteArray rbuffer = serial.readAll();

    if(!serial.waitForReadyRead(-1)) { //block until new data arrives
        qDebug() << "error: " << serial.errorString();
        ret = SIS_ERR;
    }
    else {
        qDebug() << "New data available: " << serial.bytesAvailable();
        QByteArray rbuffer = serial.readAll();

        rdata = (unsigned char *)rbuffer.data();
        rlength = rbuffer.size();

#ifdef CHAOBAN_TEST
        qDebug() << "Read:" << rbuffer;

        printf("rdata:");
        for (int i=0; i < rlength; i++) {
            printf("%x ",rdata[i]);
        }
        printf("\n");
#endif
    }

    return ret;
}


/*
 * Change Mode
 */

bool sis_switch_to_cmd_mode()
{
    int ret = -1;
    uint8_t tmpbuf[MAX_BYTE] = {0};
//chaoban test
//    uint8_t sis817_cmd_active[12] = {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44};
    uint8_t sis817_cmd_active[CMD_SZ_XMODE] = {SIS_REPORTID, 0x00/*CRC16*/, CMD_SISXMODE, 0x51, 0x09};
    uint8_t sis817_cmd_enable_diagnosis[CMD_SZ_XMODE] = {SIS_REPORTID, 0x00/*CRC16*/, CMD_SISXMODE, 0x21, 0x01};


/* 計算CRC並填入適當欄位內
 * 使用 sis_calculate_output_crc( u8* buf, int len )
 * buf: 要計算的command封包
 * len: command封包的長度
 * 以Change mode為例:
 * [0x09, CRC, 0x85, 0x51, 0x09]
 */
    sis817_cmd_active[BIT_CRC] = sis_calculate_output_crc( sis817_cmd_active, sizeof(sis817_cmd_active) );
    sis817_cmd_enable_diagnosis[BIT_CRC] = sis_calculate_output_crc( sis817_cmd_enable_diagnosis, sizeof(sis817_cmd_enable_diagnosis) );
    //printf("CRC=%x\n", sis817_cmd_active[BIT_CRC]);
    //printf("CRC=%x\n", sis817_cmd_enable_diagnosis[BIT_CRC]);

    //Send 85 CMD - PWR_CMD_ACTIVE
    ret = sis_command_for_write(sizeof(sis817_cmd_active), sis817_cmd_active);
    if (ret < 0) {
        qDebug() << "SiS SEND Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n";
        return false;
    }


//CHAOBAN TEST 為了驗證後續流程，先暫時關掉
#if 0
    ret = sis_command_for_read(sizeof(tmpbuf), tmpbuf);
    if (ret < 0) {
        qDebug() <<"SiS READ Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n";
        return false;
    }


    if ((tmpbuf[BUF_ACK_LSB] == BUF_NACK_L) && (tmpbuf[BUF_ACK_MSB] == BUF_NACK_H)) {
        qDebug() << "SiS SEND Switch CMD Return NACK - 85(PWR_CMD_ACTIVE)\n";
        return false;
     }
     else if ((tmpbuf[BUF_ACK_LSB] != BUF_ACK_L) || (tmpbuf[BUF_ACK_MSB] != BUF_ACK_H)) {
        qDebug() << "SiS SEND Switch CMD Return Unknow- 85(PWR_CMD_ACTIVE)\n";
        return false;
     }
#endif
     //msleep(100);
     memset(tmpbuf, 0, sizeof(tmpbuf));

     //Send 85 CMD - ENABLE_DIAGNOSIS_MODE
     ret = sis_command_for_write(sizeof(sis817_cmd_enable_diagnosis), sis817_cmd_enable_diagnosis);
     if (ret < 0) {
        qDebug() << "SiS SEND Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
      }

//CHAOBAN TEST 為了驗證後續流程，先暫時關掉
#if 0
     ret = sis_command_for_read(sizeof(tmpbuf), tmpbuf);
     if (ret < 0) {
        qDebug() << "SiS READ Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
     }

     if ((tmpbuf[BUF_ACK_LSB] == BUF_NACK_L) && (tmpbuf[BUF_ACK_MSB] == BUF_NACK_H)) {
        qDebug() << "SiS SEND Switch CMD Return NACK - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
     }else if ((tmpbuf[BUF_ACK_LSB] != BUF_ACK_L) || (tmpbuf[BUF_ACK_MSB] != BUF_ACK_H)) {
        qDebug() << "SiS SEND Switch CMD Return Unknow- 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
     }
#endif

    //msleep(50);
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

//static bool sis_get_bootflag(quint32 *bootflag)
bool sis_get_bootflag(quint32 *bootflag)
{
    int ret = 0;
    uint8_t tmpbuf[MAX_BYTE] = {0};
    uint8_t sis_cmd_get_bootflag[CMD_SZ_READ] = {SIS_REPORTID,0x00/*CRC*/,
            CMD_SISREAD, 0xf0, 0xef, 0x01, 0xa0, 0x34, 0x00};
        sis_cmd_get_bootflag[BIT_CRC] = sis_calculate_output_crc(sis_cmd_get_bootflag,
                                                                 sizeof(sis_cmd_get_bootflag) );
    // write
    ret = sis_command_for_write(sizeof(sis_cmd_get_bootflag), sis_cmd_get_bootflag);
    if (ret < 0) {
        printf("sis SEND Get Bootloader ID CMD Failed - 86 %d\n", ret);
        return -1;
    }

    // read
    //TODO: Chaoban test: What is the reaf buf format ??
    ret = sis_command_for_read(sizeof(tmpbuf), tmpbuf);

    //pr_err("sis_get_bootflag read data:\n");
    //PrintBuffer(0, MAX_BYTE, tmpbuf);

    *bootflag = (tmpbuf[8] << 24) | (tmpbuf[9] << 16) | (tmpbuf[10] << 8) | (tmpbuf[11]);
    return EXIT_OK;
}

//static bool sis_get_fw_id(quint16 *fw_version)
bool sis_get_fw_id(quint16 *fw_version)
{
    //TODO
    quint8 tmpbuf[MAX_BYTE] = {0};
    *fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
    return EXIT_OK;
}

//static bool sis_get_fw_info(quint8 *chip_id, quint32 *tp_size, quint32 *tp_vendor_id, quint16 *task_id, quint8 *chip_type, quint16 *fw_version)
bool sis_get_fw_info(quint8 *chip_id, quint32 *tp_size, quint32 *tp_vendor_id, quint16 *task_id, quint8 *chip_type, quint16 *fw_version)
{
    int ret = 0;
    uint8_t tmpbuf[MAX_BYTE] = {0};
//    uint8_t sis_cmd_get_FW_INFO[14] = {0x12, 0x04, 0x80, 0x09, 0x00,
//            0x09, 0x00, 0x86, 0x00, 0x40, 0x00, 0xa0, 0x34, 0x00};
    int rlength = READ_SIZE;
    uint8_t R_SIZE_LSB = rlength & 0xff;
    uint8_t R_SIZE_MSB = (rlength >> 8) & 0xff;

    uint64_t addr = ADDR_FW_INFO;//A0004000

    uint8_t sis_cmd_get_FW_INFO[CMD_SZ_READ] = {SIS_REPORTID, 0x00,/*CRC*/CMD_SISREAD,
                                               (ADDR_FW_INFO & 0xff),
                                               ((ADDR_FW_INFO >> 8) & 0xff),
                                               ((ADDR_FW_INFO >> 16) & 0xff),
                                               ((ADDR_FW_INFO >> 24) & 0xff),
                                              R_SIZE_LSB, R_SIZE_MSB};
    //計算及填入CRC
    sis_cmd_get_FW_INFO[BIT_CRC] = sis_calculate_output_crc(sis_cmd_get_FW_INFO,
                                                            sizeof(sis_cmd_get_FW_INFO));

    // write
    ret = sis_command_for_write(sizeof(sis_cmd_get_FW_INFO), sis_cmd_get_FW_INFO);
    if (ret < 0) {
        printf("sis SEND Get FW ID CMD Failed - 86 %d\n", ret);
        return -1;
    }

//CHAOBAN TEST 為了驗證後續流程，先暫時關掉
#if 0
    // read
    ret = sis_command_for_read(sizeof(tmpbuf), tmpbuf);

    /*pr_err("sis_get_fw_info read data:\n");
    PrintBuffer(0, MAX_BYTE, tmpbuf);*/

    *chip_id = tmpbuf[10];
    *tp_size = (tmpbuf[11] << 16) | (tmpbuf[12] << 8) | (tmpbuf[13]);
    *tp_vendor_id = (tmpbuf[14] << 24) | (tmpbuf[15] << 16) | (tmpbuf[16] << 8) | (tmpbuf[17]);
    *task_id = (tmpbuf[18] << 8) | (tmpbuf[19]);
    *chip_type = tmpbuf[21];
    *fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
#endif
    return EXIT_OK;
}

//static bool sis_reset_cmd()
bool sis_reset_cmd()
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
    /* sis_write_fw_info
     * sis_write_fw_payload
     * sis_flash_rom
     */
    return EXIT_OK;
}

//static bool sis_update_block(quint8 *data, unsigned int addr, unsigned int count)
bool sis_update_block(quint8 *data, unsigned int addr, unsigned int count)
{
    int i, ret, block_retry;
	unsigned int end = addr + count;
	unsigned int count_83 = 0, size_83 = 0; // count_83: address, size_83: length
	unsigned int count_84 = 0, size_84 = 0; // count_84: address, size_84: length
	unsigned int pack_num = 0;
    /*
     * sis_write_fw_info
     * sis_write_fw_payload
     * sis_flash_rom
    */
    count_83 = addr;
    while (count_83 < end) {
        size_83 = end > (count_83 + RAM_SIZE)? RAM_SIZE : (end - count_83);
        //printf("sis_update_block size83 = %d, count_83 = %d\n", size_83, count_83);
        pack_num = (size_83 / PACK_SIZE) + (((size_83 % PACK_SIZE) == 0)? 0 : 1);
        for (block_retry = 0; block_retry < 3; block_retry++) {
            printf("Write to addr = %08x pack_num=%d \n", count_83, pack_num);
            ret = sis_write_fw_info(count_83, pack_num);
            if (ret) {
                printf("sis Write FW info (0x83) error.\n");
                continue;
            }
            count_84 = count_83;
            for (i = 0; i < pack_num; i++) {
                size_84 = (count_83 + size_83) > (count_84 + PACK_SIZE)? 
                    PACK_SIZE : (count_83 + size_83 - count_84);
                //printf("sis_update_block size84 = %d, count_84 = %d\n", size_84, count_84);
                ret = sis_write_fw_payload(data + count_84, size_84);
                if (ret)
                    break;
                count_84 += size_84;
            }
            if (ret) {
                printf("sis Write FW payload (0x84) error.\n");
                continue;
            }
            //msleep(1000);
            ret = sis_flash_rom();
            if (ret) {
                printf("sis Flash ROM (0x81) error.\n");
                continue;
            } else {
                printf("sis update block success.\n");
                break;
            }	
        }
        if (ret < 0) {
            printf("Retry timeout\n");
            return -1;
        }
        count_83 += size_83;
        if (count_83 == count_84) {
            printf("sis count_83 == count_84.\n");
        }
    }

    return EXIT_OK;
}

//static bool sis_update_fw(quint8 *fn, bool update_boot)
bool sis_update_fw(quint8 *fn, bool update_boot)
{
    int ret = 0;

//FOR DEBUG FIRMWARE DATA
#if 1
    printf("Enter sis_update_fw()\n");
    qDebug() << FirmwareString[0];
    qDebug() << FirmwareString[1];
    qDebug() << FirmwareString[2];
    qDebug() << FirmwareString[3];
#endif

    /* (1) Clear boot flag */
    ret = sis_clear_bootflag();
    if (ret) {
        printf("sis Update fw fail at (1).");
	    //TODO:
        //goto release_firmware;
    }


    /* (2) Update main code 1 */
    /* sis_update_block */
    /* 0x00007000 - 0x00016000 */
    ret = sis_update_block(fn, 0x00007000, 0x00016000);
    if (ret) {
	    printf("sis Update fw fail at (2).");
	    //TODO:
        //goto release_firmware;
    }

    /* (3) Update main code 2 */
    /* 0x00006000 - 0x00001000 */
    ret = sis_update_block(fn, 0x00006000, 0x00001000);
    if (ret) {
	    printf("sis Update fw fail at (3).");
	    //TODO:
        //goto release_firmware;
    }

    /* (4) Update fwinfo, regmem, defmem, THQAmem, hidDevDesc, hidRptDesc */
    /* 0x00004000 - 0x00002000 */
    ret = sis_update_block(fn, 0x00004000, 0x00002000);
    if (ret) {
        printf("sis Update fw fail at (4).");
	    //TODO:
        //goto release_firmware;
    }

    /* if need update_boot */
    /* (5) Update boot code */
    /* 0x00000000 - 0x00004000 */
    if (update_boot) {
	    ret = sis_update_block(fn, 0x00000000, 0x00004000);
	    if (ret) {
		    printf("sis Update fw fail at (5).");
		    //TODO:
            //goto release_firmware;
	    }
}

    /* (6) Update rodata */
    /* 0x0001d000 - 0x00002000 */
    ret = sis_update_block(fn, 0x0001d000, 0x00002000);
    if (ret) {
        printf("sis Update fw fail at (6).");
	    //TODO:
        //goto release_firmware;
    }


    return EXIT_OK;
}

int SISUpdateFlow()
{
    quint8 chip_id = 0x00;
    quint8 bin_chip_id = 0x00;
    quint32 tp_size = 0x00000000;
    quint32 bin_tp_size = 0x00000000;
    quint32 tp_vendor_id = 0x00000000;
    quint32 bin_tp_vendor_id = 0x00000000;
    quint16 task_id = 0x0000;
    quint16 bin_task_id = 0x0000;
    quint8 chip_type = 0x00;
    quint8 bin_chip_type = 0x00;
    quint16 fw_version = 0x0000;
    quint16 bin_fw_version = 0x0000;
    quint32 bootloader_version = 0x00000000;
    quint32 bin_bootloader_version = 0x00000000;
    quint32 bootloader_crc_version = 0x00000000;
    quint32 bin_bootloader_crc_version = 0x00000000;
    quint32 bootflag = 0x00000000;
    quint32 bin_bootflag = 0x00000000;

    bool update_boot = false;
    bool force_update = false;

    int ret = -1;

    qDebug() << "Update Firmware by" <<  serial.portName() << "port";

    /*
     * Switch FW Mode
     */
    printf("Switch FW Mode\n");

#if 1
    if (!sis_switch_to_cmd_mode()) {
        qDebug() << "Error: sis_switch_to_cmd_mode Fails";
        return EXIT_ERR;
    }
#endif

    print_sep();


    /*
     * Get FW Information and Check FW Info
     * sis_get_fw_info
     */
    printf("Get FW Information\n");

    ret = sis_get_fw_info(&chip_id, &tp_size, &tp_vendor_id, &task_id, &chip_type, &fw_version);

    if (ret) {
        printf("sis get fw info failed %d\n", ret);
    }

    /*
     * Check FW Info
     */
    printf("Check FW Info\n");
//TODO
//CHAOBAN TEST
#if 0
    //chip id
    bin_chip_id = sis_fw_data[0x4002];
    printf("sis chip id = %02x, bin = %02x\n", chip_id, bin_chip_id);

    //tp vendor id
    bin_tp_vendor_id = (sis_fw_data[0x4006] << 24) | (sis_fw_data[0x4007] << 16) | (sis_fw_data[0x4008] << 8) | (sis_fw_data[0x4009]);
    printf("sis tp vendor id = %08x, bin = %08x\n", tp_vendor_id, bin_tp_vendor_id);

    //task id
    bin_task_id = (sis_fw_data[0x400a] << 8) | (sis_fw_data[0x400b]);
    printf("sis task id = %04x, bin = %04x\n", task_id, bin_task_id);

    //0x400c reserved

    //chip type
    bin_chip_type = sis_fw_data[0x400d];
    printf("sis chip type = %02x, bin = %02x\n", chip_type, bin_chip_type);

    //fw version
    bin_fw_version = (sis_fw_data[0x400e] << 8) | (sis_fw_data[0x400f]);
    printf("sis fw version = %04x, bin = %04x\n", fw_version, bin_fw_version);

    //check fw info
    if ( (chip_id != bin_chip_id) || (tp_size != bin_tp_size) || (tp_vendor_id != bin_tp_vendor_id) || (task_id != bin_task_id) || (chip_type != bin_chip_type) ) {
        printf("fw info not match, stop update fw.");
        return EXIT_FAIL;
    }

    msleep(2000);
#endif

    print_sep();

    /*
     * Get BootFlag
     * sis_get_bootflag()
     */
    ret = sis_get_bootflag(&bootflag);
    if (ret) {
        printf("sis get bootflag failed %d\n", ret);
    }

    /*
     * Check BootFlag
     */
    printf("Check BootFlag\n");
//TODO
//CHAOBAN TEST
#if 0
    bin_bootflag = (sis_fw_data[0x1eff0] << 24) | (sis_fw_data[0x1eff1] << 16) | (sis_fw_data[0x1eff2] << 8) | (sis_fw_data[0x1eff3]);
    printf("sis bootflag = %08x, bin = %08x\n", bootflag, bin_bootflag);

    if (bin_bootflag != SIS_BOOTFLAG_P810) {
        printf("bin file broken, stop update fw.\n");
        return EXIT_FAIL;
    }

    if (bootflag != SIS_BOOTFLAG_P810) {
        printf("fw broken, force update fw.\n");
        force_update = true;
    }

    msleep(2000);
#endif

    print_sep();

    /*
     * Get Bootloader ID and Bootloader CRC
     * sis_get_bootloader_id_crc
     */
    
    /*
     * Check Bootloader ID and Bootloader CRC
     */
    printf("Check Bootloader ID and Bootloader CRC\n");
//TODO
//CHAOBAN TEST
#if 0
    ret = sis_get_bootloader_id_crc(&bootloader_version, &bootloader_crc_version);
    if (ret) {
        printf("sis get bootloader id or crc failed %d\n", ret);
    }
#endif
    print_sep();

    /*
     * Update FW
     * sis_update_fw
     */
    printf("START FIRMWARE UPDATE!!!\n");
    //ret = sis_update_fw(FirmwareString, update_boot);
    ret = sis_update_fw(sis_fw_data, update_boot);
	if (ret) {
		printf("sis update fw failed %d\n", ret);
		return ret;
	}

    print_sep();

    /* Reset */
    ret = sis_reset_cmd();
    printf("Reset SIS Device\n");
    if (ret) {
		printf("sis RESET failed %d\n", ret);
		return ret;
	}

    print_sep();

    /*
     * Release FW Bin File
    */

    return EXIT_OK;
}
