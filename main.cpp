#include <QCoreApplication>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#include <iostream>
#include <stdio.h>
#include <QSettings>
#include <QStringList>
#include <QSerialPort>
#include <QDebug>
#include <QFile>
#include "DoFirmwareUpdate.h"
#include "version.h"
#include "ExitStatus.h"
//#pragma comment(lib, "Advapi32.lib")

#define TIMEOUT_TIME 3000

#if 0
#define SIS_VERIFY {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 11
#else
#define SIS_VERIFY {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 12
#endif

const QStringList getComportRegKey();
DWORD WINAPI RcvWaitProc(LPVOID lpParamter);
int testSerialPort(QString *ComPortName);
int readBinary(QString path);
void print_sep();
extern int ScanPort();
extern int getTimestamp();

unsigned char * fn; /* 讀取韌體檔案用 */
QByteArray FirmwareString;
QSerialPort serial; /* 開啟Serial Port用 */
int occupiedPortCount = 0;
int timeOutPortCount = 0;
bool mismatchKey = FALSE;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    const int argumentCount = QCoreApplication::arguments().size();
    const QStringList argumentList = QCoreApplication::arguments();
    int exitCode = CT_EXIT_AP_FLOW_ERROR;
    QString ComPortName;
    QString FwFileName = "FW.BIN";
    //FILE* input_file = 0;

    QTextStream standardOutput(stdout);
    bool userassign = false;

    printVersion();

    int mmddHHMM = getTimestamp();
    printf("Time Stamp=%08x\n", mmddHHMM);

    /* CHECK COMMAND ARGUMENTS */
    switch (argumentCount) {
        case 1:
            ScanPort();
            exitCode = testSerialPort(&ComPortName);
            userassign = true;
            return exitCode;  //CHAOBAN TEST FOR DEBUG
        break;
        case 2:
            ComPortName = argumentList.at(1);
            if(ComPortName.size()>4)
            {
                QString tmp = "\\\\.\\";
                tmp.append(ComPortName);
                ComPortName = tmp;
            }
            userassign = true;
        break;
        default:
        break;
    }

    //TODO: PARSE COMMAND FORMAT

    /*
     * Get the Comm Port of SiS Device
     */

    //printf("\nSerial Port test:");
    if (!userassign) {
        exitCode = testSerialPort(&ComPortName);
        if (exitCode) {
            return exitCode;
        }
     }

    /*
     * OPEN LOCAL FIRMWARE BIN FILE
     */
    //printf("Open the Firmware file: ");
    exitCode = readBinary(FwFileName);
    if (exitCode) {
        printf("Load Firmware Bin File Fails.\n");
        exitCode = EXIT_ERR;
        return exitCode;
    }

    /*
     * OPEN SIS UART COMM PORT
     */
    qDebug() << "Open SiS" << ComPortName << "port";
    serial.setPortName(ComPortName);

    /*
     * UART預設值
     */
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial.open(QIODevice::ReadWrite)) {
        standardOutput << QObject::tr("Failed to open port %1, error: %2")
                          .arg(ComPortName, serial.errorString())
                       << Qt::endl;
        return CT_EXIT_NO_COMPORT;
    }

    printf("Open %s successfully.\n", ComPortName.toStdString().c_str());

    /* UPDATE FW */
    exitCode = SISUpdateFlow();

    /* GET FW ID */

    printf("\nExit code : %d\n", exitCode);

    if (serial.isOpen()) serial.close();

    return exitCode;
    //  return a.exec();
}

const QStringList getComportRegKey()
{
    QStringList comports;
    QString keyPath = "HARDWARE\\DEVICEMAP\\SERIALCOMM";
    HKEY comsKey;
    LPCWSTR winKeyPath = (LPCWSTR) keyPath.constData();
    if( RegOpenKey( HKEY_LOCAL_MACHINE, winKeyPath, &comsKey ) != ERROR_SUCCESS ) {
        RegCloseKey( comsKey );
        return comports;
    }

    QSettings settings( QString("HKEY_LOCAL_MACHINE\\")+keyPath, QSettings::NativeFormat );
    QStringList keys = settings.allKeys();
    foreach( const QString &key, keys ) {
        QString newKey(key);
        newKey.replace( QString("/"), QString("\\") );
        LPCWSTR winKey = (LPCWSTR) newKey.constData();
        char *szData = new char[101];
        DWORD dwType, dwLen=100;
        if( RegQueryValueEx( comsKey, winKey,
                             NULL, &dwType, (unsigned char *)szData, &dwLen) == ERROR_SUCCESS ) {
            comports.append( QString::fromUtf16( (ushort*)szData ) );
            printf("%s, ",QString::fromUtf16( (ushort*)szData ).toStdString().c_str() );
        }
        delete[] szData;
    }

    RegCloseKey( comsKey );
    return comports;
}

DWORD WINAPI RcvWaitProc(LPVOID lpParamter)
{
    HANDLE hComm = (HANDLE) lpParamter;
    SetCommMask( hComm, EV_RXCHAR );
    DWORD dwEventMask;
    WaitCommEvent( hComm, &dwEventMask, NULL );

    return(0L);
}

int testSerialPort(QString *ComPortName)
{
    printf("\nList all com port : ");
    QStringList comports = getComportRegKey();

    int portAmount = comports.size();
    if ( portAmount == 0 ) {
        printf("Error : cannot find any existing com port, please attach UART/USB adapter.\n");
        return CT_EXIT_NO_COMPORT;
    }

    printf("\nStart polling com port ...\n");
    for (int pollingCount=portAmount-1; pollingCount>=0; pollingCount--) {
        QString curPortName = comports.at(pollingCount);
        *ComPortName = curPortName;
        if (curPortName.size()>4) {
            QString tmp = "\\\\.\\";
            tmp.append(curPortName);
            curPortName = tmp;
        }

        HANDLE hComm;
        hComm = CreateFile( (const wchar_t*) curPortName.utf16(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
        DCB tmpDCB;
        bool commState;
        commState = GetCommState(hComm, &tmpDCB);
        if ( hComm == INVALID_HANDLE_VALUE || commState == FALSE) {
            printf("Open %s failed\t--> Warning : this com port was occupied or no longer available\n", curPortName.toStdString().c_str());
            occupiedPortCount ++;
            CloseHandle( hComm );
            continue;
        }
        else if ( commState == TRUE ) {
            tmpDCB.BaudRate = CBR_115200;
            tmpDCB.ByteSize = 8;
            tmpDCB.StopBits = ONESTOPBIT;
            tmpDCB.Parity = NOPARITY;
            SetCommState( hComm, &tmpDCB );
            printf("Open %s successfully\t--> ", curPortName.toStdString().c_str());
        }

        //BOOL status;
        char lpBuffer[] = SIS_VERIFY;
        DWORD dNoOfBytestoWrite;
        DWORD dNoOfBytesWritten = 0;
        dNoOfBytestoWrite = sizeof( lpBuffer );

        //status = WriteFile( hComm,
        WriteFile( hComm,
                   lpBuffer,
                   dNoOfBytestoWrite,
                   &dNoOfBytesWritten,
                   NULL );

        HANDLE hThread;
        hThread = CreateThread(NULL, 0, RcvWaitProc, hComm, 0, NULL);
        if ( WaitForSingleObject(hThread, TIMEOUT_TIME) == WAIT_TIMEOUT ) {
            printf("Warning : receiver timeout\n");
            timeOutPortCount++;
//            CloseHandle( hComm );
            continue;
        }

        char TempChar;
        char SerialBuffer[256];
        DWORD NoByteRead;
        int inByteCount = 0;
        do {
            ReadFile( hComm,
                      &TempChar,
                      sizeof(TempChar),
                      &NoByteRead,
                      NULL );
            SerialBuffer[inByteCount] = TempChar;
            inByteCount++;
//            printf("%x , %d, %d\n", TempChar, NoByteRead, inByteCount);
        }
        while(inByteCount < SIS_VERIFY_LENGTH);
//        while( NoByteRead > 0 );

        char verifyKey[] = SIS_ACK;
        mismatchKey = FALSE;
        for ( int i=0; i<inByteCount; i++ ) {
            if ( verifyKey[i]!= SerialBuffer[i] ) {
                mismatchKey = TRUE;
            }
        }

        if (mismatchKey == FALSE) {
            printf("verifying key matches for SiS Device.\n");
            CloseHandle( hComm );
            return CT_EXIT_PASS;
            break;
        }
        else {
            printf("verifying key mismatched\n");
#if 0
            for( int i=0; i<inByteCount; i++ )
            {
                printf("%x,", SerialBuffer[i]);
            }
            printf("\n\n");
#endif
            CloseHandle( hComm );
            continue;
        }

        CloseHandle( hComm );
    }

    printf("FAIL\n");
    if ( occupiedPortCount>0 ) {
        printf("(Open com port fail, some com ports were occupied)\n");
    }

    if (mismatchKey == TRUE) {
        printf("(Verifying key mismatch)\n");
    }

    if (timeOutPortCount>0) {
        printf("(UART RX no response)\n");
    }

    return CT_EXIT_FAIL;
}

int readBinary(QString path)
{
    char file_data;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open bin file for reading";
        return -1;
    }

    while(!file.atEnd())
    {
      // return value from 'file.read' should always be sizeof(char).
      file.read(&file_data,sizeof(char));
      //TODO: 要轉換格式
      FirmwareString.append(file_data);
    }
    file.close();

    printf("Firmware data being read are %i bytes\n", FirmwareString.length());

    fn = (unsigned char *)FirmwareString.data();
    
    return EXIT_OK;
}

void print_sep()
{
    printf( "-----\n" );
}
