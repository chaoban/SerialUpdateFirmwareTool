/**********************************************
**　Update Firmware Flow
**
** 1.	Switch Firmware Mode
** 2.	Check Firmware Info
** 3.	Check BootFlag
** 4.	Check Bootloader ID and Bootloader CRC
** 5.	Update Firmware
**
**********************************************/
#include <QSerialPort>
#include <QDebug>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <stdio.h>
#include "delay.h"
#include "DoFirmwareUpdate.h"
#include "sis_command.h"
#include "SiSAdapter.h"
QByteArray buffer;
const unsigned char start[2] = {0x0e, 0x0e};
int waitCount = 0;
int processMyData(QByteArray data);
extern void sis_Make_83_Buffer( quint8 *, unsigned int, int );
extern void sis_Make_84_Buffer( quint8 *, const quint8 *, int);
extern quint8 sis_Calculate_Output_Crc( quint8* buf, int len );
extern int GLOBAL_DEBUG_VERBOSE;
/*
 * Serial Write Commands
 * Return 0 = OK, others failed
 */
int sisCmdTx(QSerialPort* serial, int wlength, unsigned char *wdata)
{
    QTextStream standardOutput(stdout);
    QByteArray writeData, appendData;
    int ret = EXIT_OK;
    // 將 unsigned char 數組轉換為 QByteArray
    appendData = QByteArray::fromRawData(reinterpret_cast<char*>(wdata), wlength);

    writeData.resize(5);
    writeData[BIT_UART_ID] = GR_CMD_ID;
    writeData[BIT_OP_LSB] = GR_OP_WR;
    writeData[BIT_OP_MSB] =  static_cast<byte>(GR_OP);//chaoban test 2023/4/19
    writeData[BIT_SIZE_LSB] = (wlength & 0xff);
    writeData[BIT_SIZE_MSB] = ((wlength >> 8 ) & 0xff);
    writeData.append(appendData);

#ifdef _CHAOBAN_DX
    if (GLOBAL_DEBUG_VERBOSE >= 3)
        qDebug() << "sisCmdTx 發送的資料：" << writeData.toHex();
#endif

    const qint64 bytesWritten = serial->write(writeData);

    if (bytesWritten == -1) {
        standardOutput << QObject::tr("無法發送命令到串列埠 %1，錯誤信息：%2")
                          .arg(serial->portName(), serial->errorString()) << Qt::endl;
        ret = CT_EXIT_CHIP_COMMUNICATION_ERROR;
    } else if (bytesWritten != writeData.size()) {
        standardOutput << QObject::tr("無法全部發送命令到串列埠 %1，錯誤信息：%2")
                          .arg(serial->portName(), serial->errorString()) << Qt::endl;
        ret = CT_EXIT_CHIP_COMMUNICATION_ERROR;
    } else if (!serial->waitForBytesWritten(1000)) {
        standardOutput << QObject::tr("操作超時或者發生錯誤，串列埠：%1，錯誤信息：%2")
                          .arg(serial->portName(), serial->errorString()) << Qt::endl;
        ret = CT_EXIT_CHIP_COMMUNICATION_ERROR;
    }

#ifdef _CHAOBAN_DTX
    if (GLOBAL_DEBUG_VERBOSE >= 3)
        qDebug() << "sisCmdTx Send " << bytesWritten << " byts";
#endif
    return ret;
}

// 處理讀取到的資料
int processMyData(QByteArray data) {
    // 將QByteArray轉換為unsigned char指標
    const unsigned char* myData = reinterpret_cast<const unsigned char*>(data.data());
    //qDebug() << "Process Data: " << data.toHex();
    quint16 status = myData[6] << 8 | myData[5];
#ifdef _CHAOBAN_DRX
//	if (GLOBAL_DEBUG_VERBOSE >= 2)
        printf("Ack: %x\n", status);
#endif
    if (status != 0xbeef)
        return EXIT_FAIL;
    else
        return EXIT_OK;
}

/*
 * Serial Read Commands
 * Return 0 = OK, others failed
 */
int sisCmdRx(QSerialPort* serial, int rlength, unsigned char *rdata)
{
    int ret = EXIT_ERR;

    QByteArray data;

    while (serial->waitForReadyRead(2000)) {
        // 讀取serial port接收到的資料
        //QByteArray data = serial->readAll();
        data += serial->readAll();
        //QByteArray data = serial->read(rlength);//chaoban test 2023.4.7
#if 0
        if (data.isEmpty()) {
            waitCount++;
            if (waitCount >= 10) {
                qDebug() << "Timeout! No data received within X seconds.";
                break;
            }
            continue;
        }
        waitCount = 0;
#endif

#ifdef _CHAOBAN_DRX
        //if (GLOBAL_DEBUG_VERBOSE >= 2)
            qDebug() << "Origin data:" <<data.toHex();
#endif
        // 將接收到的資料轉換為unsigned char型別，然後加入緩衝區
        const unsigned char* p = reinterpret_cast<const unsigned char*>(data.data());
        for (int i = 0; i < data.size(); i++) {
            buffer.append(*(p + i));
        }

#ifdef _CHAOBAN_DRX
        //if (GLOBAL_DEBUG_VERBOSE >= 2)
            qDebug() << "buffer: " << buffer.toHex();
#endif
        // 搜索特殊標識出現的位置
        int index = buffer.indexOf(reinterpret_cast<const char*>(start), 0);
        //qDebug() << "index: " << index;

        while (index >= 0) {
            // 如果找到特殊標識，就根據長度L讀取資料
            //qDebug() << "Get header index in " << index;
            if (buffer.size() >= index + 4) {
                int length = (buffer.at(index + 2)) | (buffer.at(index + 3) << 8);
                if (buffer.size() >= index + 4 + length) {
#if 1 //chaoban test 2023.4.7
                    // 將接收到的資料轉換為unsigned char型別，然後加入緩衝區
                    QByteArray hexData = buffer.toHex();
                    QByteArray tmp;
                    for (int i = 0; i < hexData.size(); i += 2) {
                        QString byte = hexData.mid(i, 2);
                        tmp.append(static_cast<char>(byte.toInt(nullptr, 16)));
                    }
                    memcpy(rdata, tmp.constData(), tmp.size());
#endif
                    //qDebug() << "buffer.size() " << buffer.size();
                    //qDebug() << "index + 4 + length " << index + 4 + length;
                    // 讀取到完整的資料
                    QByteArray myData = buffer.mid(index + 4, length);
                    //qDebug() << "myData: " << myData.toHex();
                    ret = processMyData(myData);
                    // 從緩衝區中刪除已處理的資料
                    buffer.remove(0, index + 4 + length);
                    if (ret == EXIT_OK) {
                        return ret;
                    }else
                        break;
                } else {
                    // 資料還不完整，等待下一次接收
                    qDebug() << "Data is not complete, waiting for next reception #1.";
                    break;
                }
            } else {
                // 資料還不完整，等待下一次接收
                qDebug() << "Data is not complete, waiting for next reception. #2";
                break;
            }
            // 繼續搜索特殊標識
            index = buffer.indexOf(reinterpret_cast<const char*>(start), 0);
        }
    }
    return ret;
}

/*
 * Change Mode
 */
bool sisSwitchCmdMode(QSerialPort* serial)
{
    int ret = EXIT_OK;
    //uint8_t tmpbuf[MAX_BYTE] = {0};
    uint8_t tmpbuf[13] = {0};
    uint8_t sis_cmd_active[CMD_SZ_XMODE] = {SIS_REPORTID,
                                            0x00/*CRC16*/,
                                            CMD_SISXMODE,
                                            0x51, 0x09};
    uint8_t sis_cmd_enable_diagnosis[CMD_SZ_XMODE] = {SIS_REPORTID,
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
    sis_cmd_active[BIT_CRC] = sis_Calculate_Output_Crc( sis_cmd_active, sizeof(sis_cmd_active) );
    sis_cmd_enable_diagnosis[BIT_CRC] = sis_Calculate_Output_Crc( sis_cmd_enable_diagnosis, sizeof(sis_cmd_enable_diagnosis) );
    //printf("CRC=%x\n", sis_cmd_active[BIT_CRC]);
    //printf("CRC=%x\n", sis_cmd_enable_diagnosis[BIT_CRC]);

    //Send 85 CMD - PWR_CMD_ACTIVE
    ret = sisCmdTx(serial, sizeof(sis_cmd_active), sis_cmd_active);
    if (ret != EXIT_OK) {
        printf("SiS SEND Switch CMD Failed - 85(PWR_CMD_ACTIVE), Error code: %d\n", ret);
        return false;
    }

    msleep(10);// ref. benson's doc

    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ Switch CMD Failed - 85(PWR_CMD_ACTIVE), Error code:%d\n", ret);
        return false;
    }
#ifdef VERIFYRX
    //printf("SiS Send Switch CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS Send Switch CMD Error code: %d\n", ret);
        return false;
    }
#endif

    msleep(100);

    memset(tmpbuf, 0, sizeof(tmpbuf));
    //Send 85 CMD - ENABLE_DIAGNOSIS_MODE
    ret = sisCmdTx(serial, sizeof(sis_cmd_enable_diagnosis), sis_cmd_enable_diagnosis);
    if (ret != EXIT_OK) {
        printf("SiS SEND Switch CMD Failed - 85(ENABLE_DIAGNOSIS_MODE), Error code: %d\n", ret);
        return false;
    }

    msleep(_TX_RX_MS_);

    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ Switch CMD Failed - 85(ENABLE_DIAGNOSIS_MODE), Error code: %d\n", ret);
        return false;
    }

#ifdef VERIFYRX
    //printf("SiS Send Switch CMD: ");
    ret = verifyRxData(sizeof(tmpbuf),tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS Send Switch CMD Error code: %d\n", ret);
        return false;
    }
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
bool sisChangeMode(enum SIS_POWER_MODE mode)
{
    bool ret = true;
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
    return ret;
}

/*
 * Get Chip status
 * Use this function to:
 * Return:-1 is get firmware work status error.
 * POWER_MODE_FWCTRL, power control by FW.
 * POWER_MODE_ACTIVE, chip always work on time.
 * POWER_MODE_SLEEP,  chip on the sleep mode.
 */
enum SIS_POWER_MODE sis_get_fw_mode()
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
 * Boot flag: 0x50 0x38 0x31 0x30
 * Address: 0x1eff0 - 0x1eff3
 * Return 0 = OK, others failed
*/
int sisGetBootflag(QSerialPort* serial, quint32 *bootflag)
{
    int ret = EXIT_OK;
    int rlength = R_MAX_SIZE;
    uint8_t R_SIZE_LSB = rlength & 0xff;
    uint8_t R_SIZE_MSB = (rlength >> 8) & 0xff;
    uint8_t sis_cmd_get_bootflag[CMD_SZ_READ] = {SIS_REPORTID, 0x00/*CRC*/,
                                                 CMD_SISREAD,
                                                 (ADDR_BOOT_FLAG & 0xff),
                                                 ((ADDR_BOOT_FLAG >> 8) & 0xff),
                                                 ((ADDR_BOOT_FLAG >> 16) & 0xff),
                                                 ((ADDR_BOOT_FLAG >> 24) & 0xff),
                                                 R_SIZE_LSB,
                                                 R_SIZE_MSB};
        sis_cmd_get_bootflag[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_bootflag,
                                                                 sizeof(sis_cmd_get_bootflag) );
    ret = sisCmdTx(serial, sizeof(sis_cmd_get_bootflag), sis_cmd_get_bootflag);
    if (ret != EXIT_OK) {
        printf("SiS SEND READ CMD Failed - (86), Error code: %d\n", ret);
        return ret;
    }

    msleep(10);//ref bonson's doc

    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ CMD Failed - Read Data (86), Error code: %d\n", ret);
        return ret;
    }

    //printf("SiS Send Switch CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS Get Boot flag CMD Error code: %d\n", ret);
        return ret;
    }

#ifdef _CHAOBAN_DRX //chaoban test 2023.4.7
//	if (GLOBAL_DEBUG_VERBOSE >= 2) {
        printf("tmpbuf:");
        for (unsigned int i = 0; i < sizeof(tmpbuf); i++) {
            printf("%02X ", tmpbuf[i]);
        }
        printf("\n");
//	}
#endif

    if (BIT_RX_READ + 3 < sizeof(tmpbuf)) {
        *bootflag = (tmpbuf[BIT_RX_READ] << 24) |
                    (tmpbuf[BIT_RX_READ + 1] << 16) |
                    (tmpbuf[BIT_RX_READ + 2] << 8) |
                    (tmpbuf[BIT_RX_READ + 3]);
    }
    return ret;
}

//TODO: CHAOBAN TEST
//要讀0xA0004000+[14:15]的FW Version有用或沒用?
#if 0
bool sisGetFwId(QSerialPort* serial, quint16 *fw_version)
{
    bool ret = true;
    quint8 tmpbuf[MAX_BYTE] = {0};
    *fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
    return ret;
}
#endif

int sisReadDataFromChip(QSerialPort* serial, quint32 address, int length, unsigned char *rdata)
{
    int ret = EXIT_OK;
    int rlength = length;
    uint8_t R_SIZE_LSB = rlength & 0xff;
    uint8_t R_SIZE_MSB = (rlength >> 8) & 0xff;

    uint8_t sis_cmd_get_FW_INFO[CMD_SZ_READ] = {SIS_REPORTID, 0x00,/*CRC*/
                                                CMD_SISREAD,
                                               0x00, 0x00, 0x00, 0x00,
                                                R_SIZE_LSB, R_SIZE_MSB};
    sis_cmd_get_FW_INFO[3] = (address & 0xff);
    sis_cmd_get_FW_INFO[4] = ((address >> 8) & 0xff);
    sis_cmd_get_FW_INFO[5] = ((address >> 16) & 0xff);
    sis_cmd_get_FW_INFO[6] = ((address >> 24) & 0xff);
    //計算及填入CRC
    sis_cmd_get_FW_INFO[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_FW_INFO,
                                                            sizeof(sis_cmd_get_FW_INFO));

    ret = sisCmdTx(serial, sizeof(sis_cmd_get_FW_INFO), sis_cmd_get_FW_INFO);
    if (ret != EXIT_OK) {
        printf("SiS SEND Get FW ID CMD Failed - (86) %d\n", ret);
        return ret;
    }

    msleep(100);//ref benson's doc

    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ DATA Failed - Read FW INFO (86), Error code: %d\n", ret);
        return ret;
    }

    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);

    if (ret != EXIT_OK) {
        printf("SiS Read Data From Chip Error code: %d\n", ret);
        return ret;
    }

#if 0
    printf("tmpbuf:");
    for (unsigned int i = 0; i < sizeof(tmpbuf); i++) {
        printf("%02X ", tmpbuf[i]);
    }
    printf("\n");
#endif

	memcpy(rdata, tmpbuf, sizeof(tmpbuf));

    return ret;
}

/*
 * Base:    0x4000
 * 0-1:     Update Mark
 * 2  :     Chip ID
 * 3-5:     TP SIZE
 * 6-9:     Vender
 * 10-11:   Task ID
 * 12   :   Resverse
 * 13   :   Type
 * 14-15:   FW Version
 * Return 0 = OK, others failed
*/
int sisGetFwInfo(QSerialPort* serial, quint8 *chip_id, quint16 *task_id, quint8 *chip_type, quint16 *fw_version)
{
    int ret = EXIT_OK;
    int rlength = R_MAX_SIZE;
    uint8_t R_SIZE_LSB = rlength & 0xff;
    uint8_t R_SIZE_MSB = (rlength >> 8) & 0xff;

    uint8_t sis_cmd_get_FW_INFO[CMD_SZ_READ] = {SIS_REPORTID, 0x00,/*CRC*/
                                                CMD_SISREAD,
                                               (ADDR_FW_INFO & 0xff),
                                               ((ADDR_FW_INFO >> 8) & 0xff),
                                               ((ADDR_FW_INFO >> 16) & 0xff),
                                               ((ADDR_FW_INFO >> 24) & 0xff),
                                                R_SIZE_LSB, R_SIZE_MSB};
    //計算及填入CRC
    sis_cmd_get_FW_INFO[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_FW_INFO,
                                                            sizeof(sis_cmd_get_FW_INFO));

    ret = sisCmdTx(serial, sizeof(sis_cmd_get_FW_INFO), sis_cmd_get_FW_INFO);
    if (ret != EXIT_OK) {
        printf("SiS SEND Get FW ID CMD Failed - (86) %d\n", ret);
        return ret;
    }

    //msleep(100);//ref benson's doc
    msleep(1000);//2023.4.7 chaoban test

    uint8_t tmpbuf[MAX_BYTE] = {0};
#ifdef _RETRY_GETFWINFO
    int rcount = 0;
    ret = EXIT_FAIL;
    while (ret != EXIT_OK && rcount < 3) {
        ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
        if (ret == EXIT_OK)
            break;

        rcount++;
        msleep(1000);
    }

    if (ret != EXIT_OK) return ret;
#else
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ DATA Failed - Read FW INFO (86), Error code: %d\n", ret);
        return ret;
    }
#endif
    //printf("SiS Send Switch CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS Send Get Firmware Info Error code: %d\n", ret);
        return ret;
    }

#if 0
    printf("tmpbuf:");
    for (unsigned int i = 0; i < sizeof(tmpbuf); i++) {
        printf("%02X ", tmpbuf[i]);
    }
    printf("\n");
#endif

    //printf("Get some parameters from device\n");
    if (BIT_RX_READ + 2 < sizeof(tmpbuf)) *chip_id = tmpbuf[BIT_RX_READ + 2];
    //if (BIT_RX_READ + 4 < sizeof(tmpbuf)) *tp_size = (tmpbuf[BIT_RX_READ + 3] << 16) | (tmpbuf[BIT_RX_READ + 4] << 8) | (tmpbuf[BIT_RX_READ + 5]);
    //if (BIT_RX_READ + 9 < sizeof(tmpbuf)) *tp_vendor_id = (tmpbuf[BIT_RX_READ + 6] << 24) | (tmpbuf[BIT_RX_READ + 7] << 16) | (tmpbuf[BIT_RX_READ + 8] << 8) | (tmpbuf[BIT_RX_READ + 9]);
    if (BIT_RX_READ + 11 < sizeof(tmpbuf)) *task_id = (tmpbuf[BIT_RX_READ + 10] << 8) | (tmpbuf[BIT_RX_READ + 11]);
    if (BIT_RX_READ + 13 < sizeof(tmpbuf)) *chip_type = tmpbuf[BIT_RX_READ + 13];
    if (BIT_RX_READ + 15 < sizeof(tmpbuf)) *fw_version = (tmpbuf[BIT_RX_READ + 14] << 8) | (tmpbuf[BIT_RX_READ + 15]);
    //printf("Get some parameters success\n");

    return ret;
}

bool sisResetCmd(QSerialPort* serial)
{
    int ret = EXIT_OK;
    uint8_t sis_cmd_82[CMD_SZ_RESET] = {SIS_REPORTID,
                                        0x00, /*CRC*/
                                        CMD_SISRESET};

    sis_cmd_82[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_82, CMD_SZ_RESET);

    ret = sisCmdTx(serial, sizeof(sis_cmd_82), sis_cmd_82);
    if (ret != EXIT_OK) {
        printf("SiS SEND reset CMD Failed - 82(RESET) %d\n", ret);
        return false;
    }

    msleep(25);//ref benson's doc

    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ reset CMD Failed - 82(RESET) %d\n", ret);
        return false;
    }

    //printf("SiS READ reset CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS READ reset CMD Error code: %d\n", ret);
        return false;
    }
    return true;
}

/*
 * 指定燒錄的ROM Address和長度
 * 使用83 command
 * ROM Address為4K Alignment
 * 長度為要使用84 command傳送幾次 Packages number
 * 一個84 command的Payload用每筆52b去切割為多個Packages
 */
bool sisUpdateCmd(QSerialPort* serial, unsigned int addr, int pack_num)
{
    int ret = EXIT_OK;
    uint8_t sis_cmd_83[CMD_SZ_UPDATE] = {0};

    //printf("SiSUpdateCmd()\n");
    sis_Make_83_Buffer(sis_cmd_83, addr, pack_num);

    ret = sisCmdTx(serial, sizeof(sis_cmd_83), sis_cmd_83);
    if (ret != EXIT_OK) {
        printf("SiS Update CMD Failed - 83(WRI_FW_DATA_INFO) %d\n", ret);
        return false;
    }

    msleep(10);//ref benson's doc

    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS Update CMD Failed - 83(WRI_FW_DATA_INFO) %d\n", ret);
        return false;
    }

    //printf("SiS READ write CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS Update CMD Failed - Error code: %d\n", ret);
        return false;
    }
    return true;
}

/*
 * 84 COMMAND: 傳送Firmware Data
 * val: Firmware BIN File內要開始寫入的ADDRESS
 * count: 4Bytes ~ 52Bytes不等
 */
bool sisWriteDataCmd(QSerialPort* serial, const quint8 *val, unsigned int count)
{
    //printf("SiSWriteDataCmd()\n");
    int ret = EXIT_OK;
    quint8 len = BIT_PALD + count;
    quint8 *sis_cmd_84 = (quint8 *)malloc(len * sizeof(quint8));

    if (!sis_cmd_84) {
        printf("SiS alloc buffer error\n");
        return false;
    }

    sis_Make_84_Buffer(sis_cmd_84, val, count);

    ret = sisCmdTx(serial, len, sis_cmd_84);
    if (ret != EXIT_OK) {
        printf("SiS SEND write CMD Failed - 84(WRI_FW_DATA_PAYL) %d\n", ret);
        return false;
    }

    msleep(6);//ref benson's doc

    quint8 tmpbuf[MAX_BYTE] = {0}; /* MAX_BYTE = 64 */
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ write CMD Failed - 84(WRI_FW_DATA_PAYL) %d\n", ret);
        return false;
    }

    //printf("SiS READ write CMD: ");
    int result = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (result) {
        printf("SiS Write Data Failed - Error code: %d\n", result);
        return false;
    }

    free(sis_cmd_84);
    return true;
}

bool sisFlashRom(QSerialPort* serial)
{
    //printf("SiSFlashRom()\n");
    int ret = EXIT_OK;
    uint8_t sis_cmd_81[CMD_SZ_FLASH] = {SIS_REPORTID,
                                        0x00, /* CRC */
                                        CMD_SISFLASH};

    sis_cmd_81[BIT_CRC] = sis_Calculate_Output_Crc( sis_cmd_81, CMD_SZ_FLASH );

    ret = sisCmdTx(serial, sizeof(sis_cmd_81), sis_cmd_81);
    if (ret != EXIT_OK) {
        printf("SiS SEND flash CMD Failed - 81(FLASH_ROM) %d\n", ret);
        return false;
    }

    msleep(160);//ref benson's doc

    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ flash CMD Failed - 81(FLASH_ROM) %d\n", ret);
        return false;
    }

    //printf("SiS READ flash CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS flash CMD Failed - Error code: %d\n", ret);
        return false;
    }

    return true;
}

bool sisClearBootflag(QSerialPort* serial)
{
    /* sisUpdateCmd
     * sisWriteDataCmd
     * sisFlashRom
     */
    bool ret;
    int retry, i;
    int pack_num = 0;
    unsigned int count_84 = 0, size_84 = 0;
    unsigned char *tmpbuf;

    tmpbuf = (unsigned char*) malloc(64 * sizeof(unsigned char));
    if (tmpbuf == NULL) {
        printf("Failed to allocate memory\n");
        return false;
    }

    memset(tmpbuf, 0, 64);

    pack_num = ((BOOT_FLAG_SIZE + PACK_SIZE - 1) / PACK_SIZE);

#ifdef _PROCESSBAR
    /* 根據運算資料量設定進度欄的寬度
     * EX. 每4K設定5個字元寬
     */
    int progresWidth = (pack_num / _4K_ALIGN_52B) * 3;
#endif

    for (retry = 0; retry < 3; retry++)
    {
        //printf("Write to addr = 0001e000 pack_num=%d \n", pack_num);
        ret = sisUpdateCmd(serial, 0x0001e000, pack_num);

        if (ret == false) {
            //printf("SiS Write FW info (0x83) error\n");
            printf("Retry 0x83 commands ...\n");
            continue;
        }

        count_84 = 0x1e000;

        for (i = 0; i < pack_num; i++) {
#ifdef _PROCESSBAR
            progresBar(pack_num, i + 1, progresWidth, 1); // 列印進度條
#endif
            size_84 = (0x1f000 > (count_84 + PACK_SIZE))? PACK_SIZE : (0x1f000 - count_84);
            ret = sisWriteDataCmd(serial, tmpbuf, size_84);

            if (ret == false)
                break;
            count_84 += size_84;
        }
#ifdef _PROCESSBAR
        printf("\n"); // 進度條完成後換行用
#endif
        if (ret == false) {
            printf("SiS Write FW payload (0x84) error\n");
            continue;
        }

        //msleep(1000);
        //msleep(_TX_RX_MS_);

        ret = sisFlashRom(serial);

        if (ret == true) {
            //printf("SiS update block success\n");
            break;
        }
        else {
            //printf("SiS Flash ROM (0x81) error\n");
            printf("Retry 0x81 commands ...\n");
            continue;
        }
    }

    free(tmpbuf);
    if (ret == false) {
        printf("Retry timeout in sisClearBootflag\n");
        return ret;
    }
    return true;
}

bool sisUpdateBlock(QSerialPort* serial, quint8 *data, unsigned int addr, unsigned int count)
{
    int i, block_retry;
    bool ret;
    unsigned int end = addr + count;
    unsigned int count_83 = 0, size_83 = 0; // count_83: address, size_83: length
    unsigned int count_84 = 0, size_84 = 0; // count_84: address, size_84: length
    int pack_num = 0;
    /*
     * sisUpdateCmd: Use 83 COMMAND
     * sisWriteDataCmd: Use 84 COMMAND
     * sisFlashRom
    */
#ifdef _PROCESSBAR
    int total_pack = (count / _4K) * ((_4K + PACK_SIZE - 1) / PACK_SIZE);
    int pack_base = 0;
    /* 根據運算資料量設定進度欄的寬度
     * EX. 每4K設定2個字元寬
     */
    int progresWidth = (total_pack / _4K_ALIGN_52B) * 3;
#endif
    count_83 = addr;
    while (count_83 < end)
    {
        size_83 = end > (count_83 + RAM_SIZE)? RAM_SIZE : (end - count_83);
#if 0
        printf("SiSUpdateBlock size83 = %d, count_83 = %d\n", size_83, count_83);
#endif
        /* pack_num = 79, 158, or 237 */
        pack_num = ((size_83 + PACK_SIZE - 1) / PACK_SIZE);

        for (block_retry = 0; block_retry < 3; block_retry++)
        {
            //printf("Write to addr = %08x pack_num=%d \n", count_83, pack_num);

            ret = sisUpdateCmd(serial, count_83, pack_num);

            if (ret == false) {
                printf("SiS Write FW info (0x83) error\n");
                continue;
            }

            count_84 = count_83;

            for (i = 0; i < pack_num; i++) {
#ifdef _PROCESSBAR
                // 這邊每次做的都是一個RAM_SIZE的大小的寫入
                // 例如12K，每筆52Bytes，共需要237次(pack_num)
                progresBar(total_pack, pack_base + i + 1, progresWidth, 2); /* 列印進度條 */
#endif
                size_84 = (count_83 + size_83) > (count_84 + PACK_SIZE) ? PACK_SIZE : (count_83 + size_83 - count_84);
#if 0
                printf("SiSUpdateBlock size84 = %d, count_84 = %d\n", size_84, count_84);
#endif
                ret = sisWriteDataCmd(serial, data + count_84, size_84);
                if (ret == false)
                    break;
                count_84 += size_84;
            }

            if (ret == false) {
                printf("SiS Write FW payload (0x84) error\n");
                continue;
            }

            //msleep(1000);//TODO: Chaoban test: Need or Not

            ret = sisFlashRom(serial);
            if (ret == true) {
                //printf("SiS update block success\n");
                break;
            }
            else {
                printf("SiS Flash ROM (0x81) error\n");
                continue;
            }
        }

        if (ret == false) {
            printf("Retry timeout in sisUpdateBlock\n");
            return false;
        }

        count_83 += size_83;
#ifdef _PROCESSBAR
        pack_base += pack_num;
#endif
        if (count_83 == count_84) {
            //printf("SiS count_83 == count_84\n");
        }
    }
#ifdef _PROCESSBAR
    printf("\n"); // 進度條後完成後跳下一行
#endif
    return true;
}

bool burningCode(QSerialPort* serial, quint8 *fn, updateParams updateCodeParam)
{
    bool ret = true;

//FOR VERIFY FIRMWARE DATA
#if 0
    printf("Enter burningCode(), Header char: %x\n", fn[0]);
#endif

    /* Clear Boot Flag
     *     ADDRESS: 0x1e000, Length=0x1000
     *     Clear boot-flag to zero
     */
    printf("Clear Boot flag ...\n");
    ret = sisClearBootflag(serial);
    if (ret == false) {
        printf("SiS update firmware failed at Clear Boot Flag\n");
        return ret;
    }

    if (updateCodeParam.onlyparam != true) {

        /* Update Main Code Section 1
         *     ADDRESS: 0x4000, Length=0x1A000
         */
        printf("Update Main Code Section 1 ...\n");
        ret = sisUpdateBlock(serial, fn, 0x00007000, 0x00016000);
        if (ret == false) {
            printf("SiS update firmware fail at Main Code Section 1 ... \n");
            return ret;
        }

        /* Update Main Code Section 2
         *     ADDRESS: 0x6000, Length=0x1000
         */
        printf("Update Main Code Section 2 ...\n");
        ret = sisUpdateBlock(serial, fn, 0x00006000, 0x00001000);
        if (ret == false) {
            printf("SiS update firmware fail at Main Code Section 2 ... \n");
            return ret;
        }

        /* Update fwinfo, regmem, defmem, THQAmem, hidDevDesc, hidRptDesc
     *     ADDRESS: 0x4000, Length=0x2000
     */
        printf("Update firmware info ...\n");
        ret = sisUpdateBlock(serial, fn, 0x00004000, 0x00002000);
        if (ret == false) {
            printf("SiS update firmware fail at info, regmem ...\n");
            return ret;
        }

        /* Update bootloader (if need update_booloader)
     *     ADDRESS: 0x0, Length=0x4000
     *     (Notes: if need update bootloader)
     */
        if (updateCodeParam.bt == true) {
            printf("Update Boot loader ...\n");
            ret = sisUpdateBlock(serial, fn, 0x00000000, 0x00004000);
            if (ret == false) {
                printf("SiS update firmware fail at Boot loader ...\n");
                return ret;
            }
        }
        /* Update rodata
     *     ADDRESS: 0x1d000, Length=0x2000
     */
        printf("Update Rodata and Boot Flag ...\n");
        ret = sisUpdateBlock(serial, fn, 0x0001d000, 0x00002000);
        if (ret == false) {
            printf("SiS update firmware fail at Rodata and Boot Flag\n");
            return ret;
        }
    }

    if (updateCodeParam.onlyparam == true) {

        /* Update Parameters
         * Address: 0x00004000, Length=0x1000
         */
        printf("Burn Parameters ...\n");
        ret = sisUpdateBlock(serial, fn, 0x00004000, 0x00001000);
        if (ret == false) {
            printf("SiS update firmware parameters failed\n");
            return ret;
        }

        /* Burn Boot Flag
         *     ADDRESS: 0x1e000, Length=0x1000
         */
        printf("Burn Boot Flag ...\n");
        ret = sisUpdateBlock(serial, fn, 0x0001e000, 0x00001000);
        if (ret == false) {
            printf("SiS update firmware fail at Boot Flag\n");
            return ret;
        }
    }

    /* Reset */
#if 1
    printf("Reset SiS Device ... ");
    ret = sisResetCmd(serial);
    if (ret == false) {
        printf("\nSiS Reset device failed %d\n", ret);
        return ret;
    }
    else
        printf("Success\n");
#else
    printf("Temporarily canceled Reset SIS Device\n");
#endif
    return ret;
}

bool sisGetBootloaderId_Crc(QSerialPort* serial, quint32 *bootloader_version, quint32 *bootloader_crc)
{
    int ret = EXIT_OK;
    int rlength = R_MAX_SIZE;
    uint8_t R_SIZE_LSB = rlength & 0xff;
    uint8_t R_SIZE_MSB = (rlength >> 8) & 0xff;
    uint8_t sis_cmd_get_bootloader_id_crc[CMD_SZ_READ] = {SIS_REPORTID, 0x00/*CRC*/,
                                                          CMD_SISREAD,
                                                          (ADDR_BOOTLOADER_ID & 0xff), /* 0xA000230 */
                                                          ((ADDR_BOOTLOADER_ID >> 8) & 0xff),
                                                          ((ADDR_BOOTLOADER_ID >> 16) & 0xff),
                                                          ((ADDR_BOOTLOADER_ID >> 24) & 0xff),
                                                          R_SIZE_LSB,
                                                          R_SIZE_MSB};
    sis_cmd_get_bootloader_id_crc[BIT_CRC] = sis_Calculate_Output_Crc(sis_cmd_get_bootloader_id_crc, CMD_SZ_READ );

    ret = sisCmdTx(serial, sizeof(sis_cmd_get_bootloader_id_crc), sis_cmd_get_bootloader_id_crc);
    if (ret != EXIT_OK) {
        printf("SiS SEND Get Bootloader ID CMD Failed - 86 %d\n", ret);
        return false;
    }

    msleep(10);//ref benson's doc

    uint8_t tmpbuf[MAX_BYTE] = {0};
    ret = sisCmdRx(serial, sizeof(tmpbuf), tmpbuf);
    if (ret != EXIT_OK) {
        printf("SiS READ Get Bootloader ID CMD Failed - 86 %d\n", ret);
        return false;
    }

    //printf("SiS READ flash CMD: ");
    ret = verifyRxData(sizeof(tmpbuf), tmpbuf);
    //if (result == true) printf("Success\n");
    if (ret != EXIT_OK) {
        printf("SiS read Bootloader ID Failed - Error code: %d\n", ret);
        return false;
    }

    if (BIT_RX_READ + 7 < sizeof(tmpbuf)) {
        *bootloader_version = (tmpbuf[BIT_RX_READ] << 24) |
                    (tmpbuf[BIT_RX_READ + 1] << 16) |
                    (tmpbuf[BIT_RX_READ + 2] << 8) |
                    (tmpbuf[BIT_RX_READ + 3]);
        *bootloader_crc = (tmpbuf[BIT_RX_READ + 4] << 24) |
                    (tmpbuf[BIT_RX_READ + 5] << 16) |
                    (tmpbuf[BIT_RX_READ + 6] << 8) |
                    (tmpbuf[BIT_RX_READ + 7]);
    }
    return true;
}

int sisUpdateFlow(QSerialPort* serial,
                  quint8 *sis_fw_data,
                  updateParams updateCodeParam)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 獲取標準輸出設備的句柄
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);

    quint8 chip_id = 0x00;
    quint8 bin_chip_id = 0x00;
    //quint32 tp_size = 0x00000000;
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
    bool bRet = true;
    int count = 0;
    uint8_t tmpbuf[MAX_BYTE] = {0};
    /*
     * Switch FW Mode
     */
    printf("Switch Firmware Mode ... ");
#ifdef _CHAOBAN_RETRY
    do{
        count ++;
        bRet = sisSwitchCmdMode(serial);
        if (bRet == true) {
            printf("Success\n");
            count = 0;
            break;
        }
        msleep(10);
        if (count > 30) {
            printf("Retry %i times still failed\n", count);
            return CT_EXIT_FAIL;
        }
    }
    while(1);
#else
    bRet = sisSwitchCmdMode(serial);
    if (bRet == false) {
        qDebug() << "Error: sisSwitchCmdMode Fails";
        return CT_EXIT_FAIL;
    }
#endif
    //msleep(2000);//chaoban test 2023.4.7

    print_sep();

    /*
     * Get PKGID and compare it with Binary file
     */
    sisReadDataFromChip(serial, ADDR_PKGID, R_MAX_SIZE, tmpbuf);

    QByteArray pkgid_ic(reinterpret_cast<const char*>(&tmpbuf[BIT_RX_READ]), 8);
    QByteArray pkgid_fw(reinterpret_cast<const char*>(&sis_fw_data[BIN_PKGID]), 8);
    qDebug() << "PKGID Information:";
    qDebug() << " SiS IC:" << pkgid_ic.toHex();
    qDebug() << " Bin file:" << pkgid_fw.toHex();

    bool bPkgidIsMatch = false;

    if (pkgid_ic == pkgid_fw)
        bPkgidIsMatch = true;

    if ((bPkgidIsMatch == false) && (updateCodeParam.jcp == false))
    {
        printf("PKGID not match, stop update firmware process\n");
        return CT_EXIT_AP_FLOW_ERROR;
    }

    /*
     * Get FW Information
     */
    printf("Get Firmware Information ... ");
#ifdef _CHAOBAN_RETRY
    count = 0;
    do{
        count ++;
        if (count >= 20) {
            printf("Retry %i times still failed\n", count);
            return CT_EXIT_FAIL;
        }

        ret = sisGetFwInfo(serial, &chip_id, &task_id, &chip_type, &fw_version);
        if (ret == EXIT_OK) {
            printf("Success\n");
            break;
        }
        msleep(500);
    }
    while(1);
#else
    ret = sisGetFwInfo(serial, &chip_id, &tp_size, &tp_vendor_id, &task_id, &chip_type, &fw_version);
    if (ret) {
        printf("SiS get fw info failed. Error code: %d\n", ret);
        return ret;
    }
#endif

    //chip id
    bin_chip_id = sis_fw_data[0x4002];
    printf("  sis chip id: %02x;      bin file: %02x\n", chip_id, bin_chip_id);

    //task id
    bin_task_id = (sis_fw_data[0x400a] << 8) | (sis_fw_data[0x400b]);
    printf("  sis task id: %04x;    bin file: %04x\n", task_id, bin_task_id);

    //0x400c reserved

    //chip type
    bin_chip_type = sis_fw_data[0x400d];
    printf("  sis chip type: %02x;    bin file: %02x\n", chip_type, bin_chip_type);

    //fw version Major and small version
    bin_fw_version = (sis_fw_data[0x400e] << 8) | (sis_fw_data[0x400f]);
    printf("  sis fw version: %04x; bin file: %04x\n", fw_version, bin_fw_version);

    /*
     * Check FW Info
     */
    if ((chip_id != bin_chip_id) ||
        (task_id != bin_task_id) ||
        (chip_type != bin_chip_type))
    {
        if (updateCodeParam.jump == true) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_YELLOW);
            printf("Firmware info not match, but jump parameter validation. Update process go on\n");
            SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
        } else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_YELLOW);
            printf("Firmware info not match, stop update firmware process\n");
            SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
            return CT_EXIT_AP_FLOW_ERROR;
        }
    }else {
        printf("Firmware information checked successfully\n");
    }

      //chaoban test marked
      //msleep(2000);
    /*
     * Get BootFlag
     */
    printf("Get BootFlag ... ");
#ifdef _CHAOBAN_RETRY
    count = 0;
    do {
        count ++;
        if (count > 10) {
            printf("Retry %i times still failed\n", count);
            return CT_EXIT_FAIL;
        }
        ret = sisGetBootflag(serial, &bootflag);
        if (ret == EXIT_OK) {
            printf("Success\n");
            break;
        }
        msleep(1000);
    }
    while(1);
#else
    ret = sisGetBootflag(serial, &bootflag);
    if (ret) {
        printf("SiS get bootflag failed, Error code: %d\n", ret);
        return ret;
    }
#endif

    bin_bootflag = (sis_fw_data[0x1eff0] << 24) | (sis_fw_data[0x1eff1] << 16) | (sis_fw_data[0x1eff2] << 8) | (sis_fw_data[0x1eff3]);
    printf("  sis bootflag: %08x; bin file: %08x\n", bootflag, bin_bootflag);

    /*
     * Check BootFlag
     * Boot flag: 0x1eff0-0x1eff3
     * Value: 0x50 0x38 0x31 0x30
     */
    if (bin_bootflag == SIS_BOOTFLAG_P810) {
        printf("Checking BootFlag of binary file successfully\n");
    } else {
        printf("Firmware Binary file broken, stop update firmeare process\n");
        return CT_EXIT_FAIL;
    }

    if (bootflag != SIS_BOOTFLAG_P810) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_YELLOW);
        printf("Firmware of device is broken, force update\n");
        SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
        updateCodeParam.force = true;
    }

    //msleep(1000); //chaoban test
    /*
     * Get Bootloader ID and Bootloader CRC
     * sisGetBootloaderId_Crc
     */
    printf("Get Bootloader ID and Bootloader CRC ... ");
    bRet = sisGetBootloaderId_Crc(serial, &bootloader_version, &bootloader_crc_version);
    if (bRet == false) {
        printf("Failure\n");
        return CT_EXIT_CHIP_COMMUNICATION_ERROR;
    }
    printf("Success\n");

    /*
     * Check Bootloader ID and Bootloader CRC
     */
    printf("Check Bootloader ID and Bootloader CRC\n");

    //bootloader id
    bin_bootloader_version = (sis_fw_data[0x230] << 24) | (sis_fw_data[0x231] << 16) | (sis_fw_data[0x232] << 8) | (sis_fw_data[0x233]);
    printf("  sis bootloader id: %08x;  bin file: %08x\n", bootloader_version, bin_bootloader_version);

    //bootloader crc
    bin_bootloader_crc_version = (sis_fw_data[0x234] << 24) | (sis_fw_data[0x235] << 16) | (sis_fw_data[0x236] << 8) | (sis_fw_data[0x237]);
    printf("  sis bootloader crc: %08x; bin file: %08x\n", bootloader_crc_version, bin_bootloader_crc_version);

/******************************************************
 * 有-b參數時: 更新Main code+Bootloader
 * 無-b參數時
 *      比對BootloaderID和BootloaderCRC
 *          相同時:
 *              只更新Main code
 *          不同時:
 *              無-ba參數:停止更新
 *              有-ba參數:更新Main code + Bootloader
 *
 * 有-p參數時:
 *      比對Main Code CRC
 *          相同時:
 *              只更新0xa0004000-0xa0004FFF
 *          不同時:
 *              ?????
 *
*******************************************************/
    bool bootLoaderIsDifferent = false;

    if ((bootloader_version != bin_bootloader_version) &&
       (bootloader_crc_version != bin_bootloader_crc_version)) {
        bootLoaderIsDifferent = true;
        SetConsoleTextAttribute(hConsole, FOREGROUND_YELLOW);
        printf("Differences have been found in Bootloader\n");
        SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
    }
    else
        printf("Bootloader ID and CRC are consistent\n");

    if (updateCodeParam.bt == false){
        if (bootLoaderIsDifferent == true){
            if (updateCodeParam.bt_a == true) {
                updateCodeParam.bt = true;
                printf("Update Bootloader and MainCode\n");
            }
            else {
                printf("Please set -b or -ba\n");
                return EXIT_FAIL;
            }
        }
        else
            printf("Only Update MainCode\n");
    }
    else {
        printf("Set as mandatory update Bootloader and MainCode\n");
    }

    /*
     * Check Main Code CRC
     */
    if (updateCodeParam.onlyparam == true) {
        sisReadDataFromChip(serial, ADDR_MAIN_CRC, R_MAX_SIZE, tmpbuf);

        QByteArray mainCRC_ic(reinterpret_cast<const char*>(&tmpbuf[BIT_RX_READ]), 4);
        QByteArray mainCRC_fw(reinterpret_cast<const char*>(&sis_fw_data[0x4044]), 4);
        qDebug() << "MainCode CRC:";
        qDebug() << " SiS IC: " << mainCRC_ic.toHex();
        qDebug() << " Bin file: " << mainCRC_fw.toHex();

        if (mainCRC_ic == mainCRC_fw) {
            printf("Only Update Parameters\n");
            updateCodeParam.force = true;
        } else {
            updateCodeParam.onlyparam = false;
            printf("Due to Main Code differences, it can not only update the parameters. Please cancel the -p parameter\n");
            return CT_EXIT_AP_FLOW_ERROR;
        }
    }
	
	// Record the current update was going to update bootloader or not.
	// ASCII: "nb" means not.
	// ASCII: "ub" means update bootloader.
	// Update in 0x40a5, 0x1e004, Size=2
	if (updateCodeParam.bt == true){
		sis_fw_data[0x40a5] = IS_UPDATE_BOOT >> 8;
		sis_fw_data[0x40a6] = IS_UPDATE_BOOT & 0xff;
		sis_fw_data[0x1e004] = IS_UPDATE_BOOT >> 8;
		sis_fw_data[0x1e005] = IS_UPDATE_BOOT & 0xff;
	}
	else {
		sis_fw_data[0x40a5] = NO_UPDATE_BOOT  >> 8;
		sis_fw_data[0x40a6] = NO_UPDATE_BOOT & 0xff;
		sis_fw_data[0x1e004] = NO_UPDATE_BOOT  >> 8;
		sis_fw_data[0x1e005] = NO_UPDATE_BOOT & 0xff;
	}
	
    if ((bin_fw_version & 0xff00) == 0xab00) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_YELLOW);
        printf("bin_fw_version 0xff00 = 0xab00, force update it\n");
        SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
        updateCodeParam.force = true;
    }
	
    //  1. Read data from device of 0xa00040a0
    // 	2. Copy 0x40A0 from device to sis_fw_data[0x40C0], size=5
    //	3. Copy 0x40B0 from device to sis_fw_data[0x40D0], size=16
    //  4. Display FW UpdateInfo
	sisReadDataFromChip(serial, ADDR_FWUPDATE_INFO, R_MAX_SIZE, tmpbuf);
	
	for (int i = 0; i < 5; i++){
		if (BIT_RX_READ + i < sizeof(tmpbuf))
            sis_fw_data[0x40C0 + i] = tmpbuf[BIT_RX_READ + i];
	}
	for (int i = 0; i < 16; i++){
		if (BIT_RX_READ + 16 + i < sizeof(tmpbuf))
            sis_fw_data[0x40D0 + i] = tmpbuf[BIT_RX_READ + 16 + i];
	}
    print_sep();
    printf("Update History of 0xA00040A0\n");
    printAddrData(sis_fw_data, "Last Time", 0x40a0, 5, false);
    printAddrData(sis_fw_data, "Update Info", 0x40a5, 2, false);
    printAddrData(sis_fw_data, "Last Mark", 0x40b0, 2, false);
    printAddrData(sis_fw_data, "Last ID", 0x40b2, 14, false);
    printAddrData(sis_fw_data, "Prior Last Time", 0x40c0, 5, false);
    printAddrData(sis_fw_data, "Prior Last Mark", 0x40d0, 2, false);
    printAddrData(sis_fw_data, "Prior Last ID", 0x40d2, 14, false);

    // 保留Calibration參數
    if (updateCodeParam.rcal == true) {
        sisReadDataFromChip(serial, 0xa0005000, R_MAX_SIZE, tmpbuf);
        for (int i = 0; i < 4; i++){
            if (BIT_RX_READ + i < sizeof(tmpbuf))
                sis_fw_data[0x5000 + i] = tmpbuf[BIT_RX_READ + i];
        }
        printAddrData(sis_fw_data, "Reserve Calibration settings", 0x5000, 4, false);
    }

    print_sep();

    /*
     * Update Firmware
     * burningCode()
     */

    if (((bin_fw_version > fw_version) && (bin_fw_version < 0xab00))
            || (updateCodeParam.force == true))
    {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        printf("START FIRMWARE UPDATE, PLEASE DO NOT INTERRUPT IT\n");
        SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);

        bRet = burningCode(serial, sis_fw_data, updateCodeParam);

        if (bRet == false) {
            //SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            //printf("SiS Update Firmware failed\n");
            //SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
            return EXIT_ERR;
        }
        //firmware_id = bin_fw_version;
    }
    else if (bin_fw_version > 0xabff) {
        printf("Unavilable Firmware version\n");
        return CT_EXIT_AP_FLOW_ERROR;
    }
    else {
        printf("Current Firmware version is same or later than the bin file\n");
        return CT_EXIT_AP_FLOW_ERROR;
    }

    return CT_EXIT_PASS;
}


/*
 * Return 0 = OK, others faled
 */
int verifyRxData(int length, uint8_t *buffer)
{
    int ret = EXIT_OK;
    quint16 ackStatus = 0;

#ifdef _CHAOBAN_DVERRX
    if (GLOBAL_DEBUG_VERBOSE >= 3) {
        printf("verifyRxData buffer: ");
        for (int i = 0; i < length; i++) {
            printf("%x ", buffer[i]);
        }
        printf("\n");
    }
#endif

    ackStatus = (buffer[BUF_ACK_MSB] << 8) | buffer[BUF_ACK_LSB];

#ifdef _CHAOBAN_DVERRX
    if (GLOBAL_DEBUG_VERBOSE >= 3)
        printf("verifyRxData ackStatus:%x\n", ackStatus);
#endif

    if (ackStatus == 0xbeef)
        ret =  EXIT_OK;
    else if (ackStatus == 0xdead) {
        printf("Return NACK\n");
        ret = CT_EXIT_FAIL;
    } else {
        printf("Return Unknow, not 0xbeef and 0xdead\n");
        ret = CT_EXIT_FAIL;
    }

/*
    if (buffer[BUF_ACK_LSB] == BUF_NACK_L && buffer[BUF_ACK_MSB] == BUF_NACK_H) {
        printf("Return NACK\n");
        ret = CT_EXIT_FAIL;
    } else if ((buffer[BUF_ACK_LSB] != BUF_ACK_L) || (buffer[BUF_ACK_MSB] != BUF_ACK_H)) {
        printf("Return Unknow\n");
        ret = CT_EXIT_FAIL;
    }
*/
    return ret;
}
