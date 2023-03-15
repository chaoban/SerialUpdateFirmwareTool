﻿/**********************************************
**　Update Flow
**
** 1.	Switch FW Mode
** 2.	Check FW Info
** 3.	Check BootFlag
** 4.	Check Bootloader ID and Bootloader CRC
** 5.	Update FW
**
**********************************************/
#include <QSerialPort>
#include <QDebug>
#include "DoFirmwareUpdate.h"
#include "sis_command.h"
#include "delay.h"

bool Chect_Read_From_FW(uint8_t *buffer);
//uint8_t sis_fw_data[] = { /*TODO: BINARY FILE*/ };
extern void sis_Make_83_Buffer( quint8 *, unsigned int, int );
extern void sis_Make_84_Buffer( quint8 *, const quint8 *, int);
extern quint8 sis_Calculate_Output_Crc( quint8* buf, int len );
extern void print_sep();
extern unsigned char * fn; /* 讀取韌體檔案用 */
extern QSerialPort serial;
extern QByteArray FirmwareString;
//bool update_fw_initprobe = false;

/*
 * I2C Read Write Commands
 */
static int sis_Command_For_Write(int wlength, unsigned char *wdata)
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

    writeData.resize(5);
    writeData[BIT_UART_ID] = GR_CMD_ID;
    writeData[BIT_OP_LSB] = GR_OP_WR;
    writeData[BIT_OP_MSB] = GR_OP;
    writeData[BIT_SIZE_LSB] = (wlength & 0xff);
    writeData[BIT_SIZE_MSB] = ((wlength >> 8 ) & 0xff);

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

static int sis_Command_For_Read(int rlength, unsigned char *rdata)
{
    int ret = EXIT_OK;

    //const QByteArray rbuffer = serial.readAll();

    if(!serial.waitForReadyRead(-1)) { //block until new data arrives
        qDebug() << "error: " << serial.errorString();
        ret = SIS_ERR;//-1
    }
    else {
        qDebug() << "New data available: " << serial.bytesAvailable();
        QByteArray rbuffer = serial.readAll();

#ifdef CHAOBAN_TEST
        rdata = (unsigned char *)rbuffer.data();
        rlength = rbuffer.size();
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

bool sis_Switch_Cmd_Mode()
{
    int ret = -1;
    uint8_t tmpbuf[MAX_BYTE] = {0};
//chaoban test
//    uint8_t sis817_cmd_active[12] = {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44};
    uint8_t sis817_cmd_active[CMD_SZ_XMODE] = {SIS_REPORTID, 
                                               0x00/*CRC16*/, 
                                               CMD_SISXMODE, 
                                               0x51, 0x09};
    uint8_t sis817_cmd_enable_diagnosis[CMD_SZ_XMODE] = {SIS_REPORTID, 
                                                         0x00/*CRC16*/, 
                                                         CMD_SISXMODE, 
                                                         0x21, 0x01};

/* 計算CRC並填入適當欄位內
 * 使用 sis_Calculate_Output_Crc( u8* buf, int len )
 * buf: 要計算的command封包
 * len: command封包的長度
 * 以Change mode為例:
 * [0x09, CRC, 0x85, 0x51, 0x09]
 */
    sis817_cmd_active[BIT_CRC] = sis_Calculate_Output_Crc( sis817_cmd_active, sizeof(sis817_cmd_active) );
    sis817_cmd_enable_diagnosis[BIT_CRC] = sis_Calculate_Output_Crc( sis817_cmd_enable_diagnosis, sizeof(sis817_cmd_enable_diagnosis) );
    //printf("CRC=%x\n", sis817_cmd_active[BIT_CRC]);
    //printf("CRC=%x\n", sis817_cmd_enable_diagnosis[BIT_CRC]);

    //Send 85 CMD - PWR_CMD_ACTIVE
    ret = sis_Command_For_Write(sizeof(sis817_cmd_active), sis817_cmd_active);
    if (ret < 0) {
        qDebug() << "SiS SEND Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n";
        return false;
    }

//CHAOBAN TEST 為了驗證後續流程，先暫時關掉
#ifndef _DBG_DISABLE_READCMD
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);
    if (ret < 0) {
        qDebug() <<"SiS READ Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n";
        return false;
    }

    printf("SiS Send Switch CMD: ");
    bool result = Chect_Read_From_FW(tmpbuf);
    if (result == true) printf("Success\n");
#endif

    msleep(100);

    memset(tmpbuf, 0, sizeof(tmpbuf));

    //Send 85 CMD - ENABLE_DIAGNOSIS_MODE
    ret = sis_Command_For_Write(sizeof(sis817_cmd_enable_diagnosis), sis817_cmd_enable_diagnosis);
    if (ret < 0) {
        qDebug() << "SiS SEND Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
    }

//CHAOBAN TEST 為了驗證後續流程，先暫時關掉
#ifndef _DBG_DISABLE_READCMD
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);
    if (ret < 0) {
        qDebug() << "SiS READ Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n";
        return false;
    }

    printf("SiS Send Switch CMD: ");
    result = Chect_Read_From_FW(tmpbuf);
    if (result == true) printf("Success\n");
#endif

    msleep(50);
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
bool sis_Change_Mode(enum SIS_817_POWER_MODE mode)
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

static bool sis_Get_Bootflag(quint32 *bootflag)
{
    int ret = 0;
    uint8_t sis_cmd_get_bootflag[CMD_SZ_READ] = {SIS_REPORTID,0x00/*CRC*/,
            CMD_SISREAD, 0xf0, 0xef, 0x01, 0xa0, 0x34, 0x00};
        sis_cmd_get_bootflag[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_bootflag,
                                                                 sizeof(sis_cmd_get_bootflag) );
    // write
    ret = sis_Command_For_Write(sizeof(sis_cmd_get_bootflag), sis_cmd_get_bootflag);
    if (ret < 0) {
        printf("sis SEND Get Bootloader ID CMD Failed - 86 %d\n", ret);
        return -1;
    }

    // read
    //TODO: Chaoban test: What is the read buf format ??
#ifndef _DBG_DISABLE_READCMD
    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);

    //pr_err("sis_Get_Bootflag read data:\n");
    //PrintBuffer(0, MAX_BYTE, tmpbuf);

    *bootflag = (tmpbuf[8] << 24) | (tmpbuf[9] << 16) | (tmpbuf[10] << 8) | (tmpbuf[11]);
#endif
    return EXIT_OK;
}

static bool sis_get_fw_id(quint16 *fw_version)
{
    //TODO
    quint8 tmpbuf[MAX_BYTE] = {0};
    *fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
    return EXIT_OK;
}

static bool sis_get_fw_info(quint8 *chip_id, quint32 *tp_size, quint32 *tp_vendor_id, quint16 *task_id, quint8 *chip_type, quint16 *fw_version)
{
    int ret = 0;
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
    sis_cmd_get_FW_INFO[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_FW_INFO,
                                                            sizeof(sis_cmd_get_FW_INFO));

    // write
    ret = sis_Command_For_Write(sizeof(sis_cmd_get_FW_INFO), sis_cmd_get_FW_INFO);
    if (ret < 0) {
        printf("sis SEND Get FW ID CMD Failed - 86 %d\n", ret);
        return -1;
    }

//CHAOBAN TEST 為了驗證後續流程，先暫時關掉
#ifndef _DBG_DISABLE_READCMD
    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);

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
    int ret = 0;
	uint8_t sis_cmd_82[CMD_SZ_RESET] = {SIS_REPORTID,
                                        0x00, /*CRC*/
                                        CMD_SISRESET};

    sis_cmd_82[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_82, CMD_SZ_RESET);

    ret = sis_Command_For_Write(sizeof(sis_cmd_82), sis_cmd_82);
    if (ret < 0) {
	    printf("sis SEND reset CMD Failed - 82(RESET) %d\n", ret);
	    return -1;
    }

#ifndef _DBG_DISABLE_READCMD
    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);
    if (ret < 0) {
	    printf("sis READ reset CMD Failed - 82(RESET) %d\n", ret);
	    return -1;
    }

    printf("SiS READ reset CMD: ");
    bool result = Chect_Read_From_FW(tmpbuf);
    if (result == true) printf("Success\n");
#endif
    return EXIT_OK;
}

//TODO: CHAOBAN TEST: This function should be rename.
/*
 * 指定燒錄的ROM Address和長度
 * 使用83 command
 * ROM Address為4K Alignment
 * 長度為要使用84 command傳送幾次 Packages number
 * 一個84 command的Payload用每筆52b去切割為多個Packages
 */
static bool sis_write_fw_info(unsigned int addr, int pack_num)
{
    int ret = 0;
    uint8_t sis817_cmd_83[CMD_SZ_UPDATE] = {0};

    //printf("sis_write_fw_info()\n");
    sis_Make_83_Buffer(sis817_cmd_83, addr, pack_num);
	
    ret = sis_Command_For_Write(sizeof(sis817_cmd_83), sis817_cmd_83);
	if (ret < 0) {
        printf("sis SEND write CMD Failed - 83(WRI_FW_DATA_INFO) %d\n", ret);
		return -1;
	}

#ifndef _DBG_DISABLE_READCMD
    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);
	if (ret < 0) {
        printf("sis READ write CMD Failed - 83(WRI_FW_DATA_INFO) %d\n", ret);
		return -1;
	}

    printf("sis READ write CMD: ");
    bool result = Chect_Read_From_FW(tmpbuf);
    if (result == true) printf("Success\n");
#endif
    return EXIT_OK;
}

/*
 * 84 COMMAND: 傳送Firmware Data
 * val: Firmware BIN File內要開始寫入的ADDRESS
 * count: 4Bytes ~ 52Bytes不等
 */
static bool sis_write_fw_payload(const quint8 *val, unsigned int count)
{
    //printf("sis_write_fw_payload()\n");
    int ret = 0;
    quint8 len = BIT_PALD + count;
    quint8 *sis817_cmd_84 = (quint8 *)malloc(len * sizeof(quint8));


    if (!sis817_cmd_84) {
        printf("sis alloc buffer error\n");
        return -1;
    }

    sis_Make_84_Buffer(sis817_cmd_84, val, count);

    ret = sis_Command_For_Write(len, sis817_cmd_84);
    if (ret < 0) {
        printf("sis SEND write CMD Failed - 84(WRI_FW_DATA_PAYL) %d\n", ret);
        return -1;
    }

#ifndef _DBG_DISABLE_READCMD
    quint8 tmpbuf[MAX_BYTE] = {0}; /* MAX_BYTE = 64 */
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);
    if (ret < 0) {
        printf("sis READ write CMD Failed - 84(WRI_FW_DATA_PAYL) %d\n", ret);
        return -1;
    }

    printf("sis READ write CMD: ");
    bool result = Chect_Read_From_FW(tmpbuf);
    if (result == true) printf("Success\n");
#endif

    free(sis817_cmd_84);
    return EXIT_OK;
}

//static bool sis_flash_rom()
bool sis_flash_rom()
{
    //printf("sis_flash_rom()\n");
    int ret = 0;
	uint8_t sis_cmd_81[CMD_SZ_FLASH] = {SIS_REPORTID, 
                                        0x00, /* CRC */ 
                                        CMD_SISFLASH};

    sis_cmd_81[BIT_CRC] = sis_Calculate_Output_Crc( sis_cmd_81, CMD_SZ_FLASH );

    ret = sis_Command_For_Write(sizeof(sis_cmd_81), sis_cmd_81);
    if (ret < 0) {
	    printf("sis SEND flash CMD Failed - 81(FLASH_ROM) %d\n", ret);
	    return -1;
    }

    msleep(2000);
	
#ifndef _DBG_DISABLE_READCMD
    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);
    if (ret < 0) {
	    printf("sis READ flash CMD Failed - 81(FLASH_ROM) %d\n", ret);
	    return -1;
    }

    printf("sis READ flash CMD: ");
    bool result = Chect_Read_From_FW(tmpbuf);
    if (result == true) printf("Success\n");
#endif

    return EXIT_OK;
}

//static bool sis_clear_bootflag()
bool sis_clear_bootflag()
{
    /* sis_write_fw_info
     * sis_write_fw_payload
     * sis_flash_rom
     */
    int ret, retry, i;
    int pack_num = 0;
	unsigned int count_84 = 0, size_84 = 0;
    unsigned char *tmpbuf;
	
    tmpbuf = (unsigned char*) malloc(64 * sizeof(unsigned char));
    if (tmpbuf == NULL) {
        printf("Failed to allocate memory.\n");
        return 1;
    }

    memset(tmpbuf, 0, 64);

    pack_num = ((BOOT_FLAG_SIZE + PACK_SIZE - 1) / PACK_SIZE);
	for (retry = 0; retry < 3; retry++) {
		//ret = sis_write_fw_info(0x0000efcc, 1, id);
		printf("Write to addr = 0001e000 pack_num=%d \n", pack_num);
		ret = sis_write_fw_info(0x0001e000, pack_num);
		if (ret) {
			printf("sis Write FW info (0x83) error.\n");
			continue;
		}
		count_84 = 0x1e000;
		for (i = 0; i < pack_num; i++) {
			size_84 = (0x1f000 > (count_84 + PACK_SIZE))?
				PACK_SIZE : (0x1f000 - count_84);
			ret = sis_write_fw_payload(tmpbuf, size_84);
			if (ret)
				break;
			count_84 += size_84;
		}
		if (ret) {
			printf("sis Write FW payload (0x84) error\n");
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

	free(tmpbuf);
	if (ret < 0) {
		printf("Retry timeout\n");
		return -1;
	}
    return EXIT_OK;
}


static bool sis_update_block(quint8 *data, unsigned int addr, unsigned int count)
{
    int i, ret, block_retry;
	unsigned int end = addr + count;
	unsigned int count_83 = 0, size_83 = 0; // count_83: address, size_83: length
	unsigned int count_84 = 0, size_84 = 0; // count_84: address, size_84: length
    int pack_num = 0;
    /*
     * sis_write_fw_info: Use 83 COMMAND
     * sis_write_fw_payload: Use 84 COMMAND
     * sis_flash_rom
    */
    count_83 = addr;
    while (count_83 < end) {
        size_83 = end > (count_83 + RAM_SIZE)? RAM_SIZE : (end - count_83);
#if 0        
        printf("sis_update_block size83 = %d, count_83 = %d\n", size_83, count_83);
#endif
        pack_num = ((size_83 + PACK_SIZE - 1) / PACK_SIZE);
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
#if 0
                printf("sis_update_block size84 = %d, count_84 = %d\n", size_84, count_84);
#endif
                ret = sis_write_fw_payload(data + count_84, size_84);
                if (ret)
                    break;
                count_84 += size_84;
            }
            if (ret) {
                printf("sis Write FW payload (0x84) error.\n");
                continue;
            }
            
            msleep(1000);//TODO: Need or Not

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

    print_sep();
    return EXIT_OK;
}


static bool sis_Update_Fw(quint8 *fn, bool update_bootloader)
{
    int ret = 0;

//FOR DEBUG FIRMWARE DATA
#if 0
    printf("Enter sis_Update_Fw()\n");
    qDebug() << fn[0];
    qDebug() << fn[1];
    qDebug() << fn[2];
    qDebug() << fn[3];
#endif

    /* (1) Clear boot flag
     *     ADDRESS: 0x1e000, Length=0x1000
     *     Clear boot-flag to "0"
     */
    printf("Clear boot flag\n");
    ret = sis_clear_bootflag();
    if (ret) {
        printf("sis Update fw fail at Clear boot flag.");
	    //TODO:
        //goto release_firmware;
    }


    /* (2) Update main code 1
     *     ADDRESS: 0x4000, Length=0x1A000
     */
    printf("Update main code 1\n");
    ret = sis_update_block(fn, 0x00004000, 0x0001A000);
    if (ret) {
        printf("sis Update fw fail at main code 1.");
	    //TODO:
        //goto release_firmware;
    }

    /* (3) Update main code 2
     *     ADDRESS: 0x6000, Length=0x1000
     */
#if 0
    printf("Update main code 2\n");
    ret = sis_update_block(fn, 0x00006000, 0x00001000);
    if (ret) {
        printf("sis Update fw fail at main code 2.");
	    //TODO:
        //goto release_firmware;
    }
#endif

    /* (4) Update fwinfo, regmem, defmem, THQAmem, hidDevDesc, hidRptDesc
     *     ADDRESS: 0x4000, Length=0x2000
     */
#if 0
    printf("Update fwinfo, regmem, defmem, THQAmem, hidDevDesc, hidRptDesc\n");
    ret = sis_update_block(fn, 0x00004000, 0x00002000);
    if (ret) {
        printf("sis Update fw fail at fwinfo.");
	    //TODO:
        //goto release_firmware;
    }
#endif

    /* (5) Update bootloader code (if need update_booloader)
     *     ADDRESS: 0x0, Length=0x4000
     *     (Notes: if need update_bootloader)
     */
    printf("Update bootloader\n");
    if (update_bootloader) {
	    ret = sis_update_block(fn, 0x00000000, 0x00004000);
	    if (ret) {
            printf("sis Update fw fail at boot code.");
		    //TODO:
            //goto release_firmware;
	    }
    }

    /* (6) Update rodata
     *     ADDRESS: 0x1d000, Length=0x2000
     */
#if 0
    printf("Update rodata\n");
    ret = sis_update_block(fn, 0x0001d000, 0x00002000);
    if (ret) {
        printf("sis Update fw fail at rodata.");
	    //TODO:
        //goto release_firmware;
    }
#endif

    /* (7) Burn Boot Flag
     *     ADDRESS: 0x1e000, Length=0x1000
     */
    printf("Burn Boot Flag\n");
    ret = sis_update_block(fn, 0x0001e000, 0x00001000);
    if (ret) {
        printf("sis Update fw fail at Burn Boot Flag.");
        //TODO:
        //goto release_firmware;
    }

    return EXIT_OK;
}

static bool sis_Get_Bootloader_Id_Crc(quint32 *bootloader_version, quint32 *bootloader_crc)
{
    int ret = 0;
    //int i=0;
    uint8_t tmpbuf[MAX_BYTE] = {0};
    uint8_t sis_cmd_get_bootloader_id_crc[CMD_SZ_READ] = {SIS_REPORTID,
        0x00, CMD_SISREAD, 0x30, 0x02, 0x00, 0xa0, 0x34, 0x00};
    sis_cmd_get_bootloader_id_crc[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_bootloader_id_crc, CMD_SZ_READ );

// write
    ret = sis_Command_For_Write(sizeof(sis_cmd_get_bootloader_id_crc), sis_cmd_get_bootloader_id_crc);
    if (ret < 0) {
        printf("sis SEND Get Bootloader ID CMD Failed - 86 %d\n", ret);
        return false;
    }

#ifndef _DBG_DISABLE_READCMD
    ret = sis_Command_For_Read(sizeof(tmpbuf), tmpbuf);

    //printf("sis_Get_Bootloader_Id_Crc read data:\n");
    //PrintBuffer(0, MAX_BYTE, tmpbuf);

    *bootloader_version = (tmpbuf[8] << 24) | (tmpbuf[9] << 16) | (tmpbuf[10] << 8) | (tmpbuf[11]);
    *bootloader_crc = (tmpbuf[12] << 24) | (tmpbuf[13] << 16) | (tmpbuf[14] << 8) | (tmpbuf[15]);
#endif
    return true;
}

int SISUpdateFlow(quint8 *sis_fw_data, bool update_bootloader, bool force_update)
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

    int ret = -1;

    qDebug() << "Start Update Firmware by" <<  serial.portName() << "port";

    /*
     * Switch FW Mode
     */
#if 1
    printf("Switch Firmware Mode\n");
    if (!sis_Switch_Cmd_Mode()) {
        qDebug() << "Error: sis_Switch_Cmd_Mode Fails";
        return EXIT_ERR;
    }
#else
    printf("Temporarily canceled Switch FW Mode\n");
#endif

    /*
     * Get FW Information and Check FW Info
     * sis_get_fw_info
     */
#if 1
    printf("Get Firmware Information\n");
    ret = sis_get_fw_info(&chip_id, &tp_size, &tp_vendor_id, &task_id, &chip_type, &fw_version);
    if (ret) {
        printf("sis get fw info failed %d\n", ret);
    }
#else
    printf("Temporarily canceled Get FW Information\n");
#endif

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

    print_sep();

    /*
     * Check FW Info
     */
#ifndef _DBG_DISABLE_CHECKFW
    printf("Check Firmware Info\n");
    if ( (chip_id != bin_chip_id) || (tp_size != bin_tp_size) || (tp_vendor_id != bin_tp_vendor_id) || (task_id != bin_task_id) || (chip_type != bin_chip_type) ) {
        printf("Firmware info not match, stop update process!!");
        return EXIT_FAIL;
    }
   
    msleep(2000);
#else
    printf("Temporarily canceled Check Firmware Info\n");
#endif
    print_sep();

    /*
     * Get BootFlag
     * sis_Get_Bootflag()
     */
#if 1
    printf("Get BootFlag\n");
    ret = sis_Get_Bootflag(&bootflag);
    if (ret) {
        printf("sis get bootflag failed: %d\n", ret);
    }
#else
    printf("Temporarily canceled get bootflag\n");
#endif
    print_sep();

    /*
     * Check BootFlag
     */
    //TODO
    //CHAOBAN TEST
    printf("Check BootFlag\n");
    bin_bootflag = (sis_fw_data[0x1eff0] << 24) | (sis_fw_data[0x1eff1] << 16) | (sis_fw_data[0x1eff2] << 8) | (sis_fw_data[0x1eff3]);
    printf("sis bootflag = %08x, bin = %08x\n", bootflag, bin_bootflag);

#ifndef _DBG_DISABLE_CHECKFW
    if (bin_bootflag != SIS_BOOTFLAG_P810) {
        printf("Firmware Binary file broken, stop update process!!\n");
        return EXIT_FAIL;
    }
#else
    printf("Temporarily canceled Check BootFlag\n");
#endif

    if (bootflag != SIS_BOOTFLAG_P810) {
        printf("Firmware broken, force update Firmware.\n");
        force_update = true;
    }

    msleep(2000);

    print_sep();

    /*
     * Get Bootloader ID and Bootloader CRC
     * sis_Get_Bootloader_Id_Crc
     */
#if 1
    printf("Get Bootloader ID and Bootloader CRC\n");
    ret = sis_Get_Bootloader_Id_Crc(&bootloader_version, &bootloader_crc_version);
    if (ret) {
        printf("sis get bootloader id or crc failed %d\n", ret);
    }
#else
    printf("Temporarily canceled Get Bootloader ID and Bootloader CRC\n");
#endif
    print_sep();

    /*
     * Check Bootloader ID and Bootloader CRC
     */
#if 1
    printf("Check Bootloader ID and Bootloader CRC\n");

    //bootloader id
    bin_bootloader_version = (sis_fw_data[0x230] << 24) | (sis_fw_data[0x231] << 16) | (sis_fw_data[0x232] << 8) | (sis_fw_data[0x233]);
    printf("sis bootloader id = %08x, bin = %08x\n", bootloader_version, bin_bootloader_version);

    //bootloader crc
    bin_bootloader_crc_version = (sis_fw_data[0x234] << 24) | (sis_fw_data[0x235] << 16) | (sis_fw_data[0x236] << 8) | (sis_fw_data[0x237]);
    printf("sis bootloader crc = %08x, bin = %08x\n", bootloader_crc_version, bin_bootloader_crc_version);

    if ((bootloader_version != bin_bootloader_version) && (bootloader_crc_version != bin_bootloader_crc_version)) {
        update_bootloader = true;
        printf("bootloader changed. update_bootloader flag on.\n");
    }

    //bin_fw_version = 0xab00; //test for update fw
    //update_bootloader = true; //test for update bootloader

    if ((bin_fw_version & 0xff00) == 0xab00) {
        force_update = true;
    }
#else
    printf("Temporarily canceled Check Bootloader ID and Bootloader CRC\n");
#endif
    print_sep();

    /*
     * Update FW
     * sis_Update_Fw
     */

    printf("START FIRMWARE UPDATE!!!\n");

#ifdef _DBG_DISABLE_CHECKFW
    force_update = true;
    update_bootloader = false;
#endif

    if (((bin_fw_version > fw_version) && (bin_fw_version < 0xab00))
            || force_update == true) { 
        //Special Update Flag : 0x6613: update by serial tool
        sis_fw_data[0x4000] = SERIAL_FLAG >> 8;
        sis_fw_data[0x4001] = SERIAL_FLAG & 0xff;

        ret = sis_Update_Fw(sis_fw_data, update_bootloader);

        if (ret) {
            printf("sis update fw failed %d\n", ret);
            return ret;
        }
        //firmware_id = bin_fw_version;
     }
     else if (bin_fw_version > 0xabff) {
        printf("Unavilable Firmware version.\n");
        //goto work_mode;//TODO: CHAOBAN TEST
     }
     else {
        printf("Current Firmware version is same or later than bin.\n");
        //goto work_mode;//TODO: CHAOBAN TEST
     }

    print_sep();

    /* Reset */
#if 1
    printf("Reset SIS Device\n");
    ret = sis_reset_cmd();
    if (ret) {
		printf("sis RESET failed %d\n", ret);
		return ret;
	}
#else
    printf("Temporarily canceled Reset SIS Device\n");
#endif
    print_sep();

    return EXIT_OK;
}

bool Chect_Read_From_FW(uint8_t *buffer)
{
    if (buffer[BUF_ACK_LSB] == BUF_NACK_L && buffer[BUF_ACK_MSB] == BUF_NACK_H) {
        printf("Return NACK\n");
        return false;
    } else if ((buffer[BUF_ACK_LSB] != BUF_ACK_L) || (buffer[BUF_ACK_MSB] != BUF_ACK_H)) {
        printf("Return Unknow\n");
        return false;
    }

    return true;
}

