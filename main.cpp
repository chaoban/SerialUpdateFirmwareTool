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
#include "version.h"
#include "ExitStatus.h"

//#pragma comment(lib, "Advapi32.lib")

#if 0
#define SIS_VERIFY {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 11
#else
#define SIS_VERIFY {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 12
#endif

#define TIMEOUT_TIME 3000//3000

int occupiedPortCount = 0;
int timeOutPortCount = 0;
bool mismatchKey = FALSE;

QSerialPort serial;
FILE* open_firmware_bin( const char* filename);
int uartTest(QString *);
void printVersion();
void print_sep();
extern int ScanPort();
extern int Do_Update();

const QStringList getComportRegKey()
{
    QStringList comports;
    QString keyPath = "HARDWARE\\DEVICEMAP\\SERIALCOMM";
    HKEY comsKey;
    LPCWSTR winKeyPath = (LPCWSTR) keyPath.constData();
    if( RegOpenKey( HKEY_LOCAL_MACHINE, winKeyPath, &comsKey ) != ERROR_SUCCESS )
    {
        RegCloseKey( comsKey );
        return comports;
    }

    QSettings settings( QString("HKEY_LOCAL_MACHINE\\")+keyPath, QSettings::NativeFormat );
    QStringList keys = settings.allKeys();
    foreach( const QString &key, keys )
    {
        QString newKey(key);
        newKey.replace( QString("/"), QString("\\") );
        LPCWSTR winKey = (LPCWSTR) newKey.constData();
        char *szData = new char[101];
        DWORD dwType, dwLen=100;
        if( RegQueryValueEx( comsKey, winKey,
                             NULL, &dwType, (unsigned char *)szData, &dwLen) == ERROR_SUCCESS )
        {
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

int testserialport(QString *ComPortName)
{
    printf("\nList all com port : ");
    QStringList comports = getComportRegKey();

    int portAmount = comports.size();
    if( portAmount == 0 )
    {
        printf("Error : cannot find any existing com port, please attach UART/USB adapter.\n");
        return CT_EXIT_NO_COMPORT;
    }

    printf("\nStart polling com port ...\n");
    for(int pollingCount=portAmount-1; pollingCount>=0; pollingCount--)
    {
        QString curPortName = comports.at(pollingCount);
        *ComPortName = curPortName;
        if(curPortName.size()>4)
        {
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
        if( hComm == INVALID_HANDLE_VALUE || commState == FALSE)
        {
            printf("Open %s failed\t--> Warning : this com port was occupied or no longer available\n", curPortName.toStdString().c_str());
            occupiedPortCount ++;
            CloseHandle( hComm );
            continue;
        }
        else if( commState == TRUE )
        {
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
        if( WaitForSingleObject(hThread, TIMEOUT_TIME) == WAIT_TIMEOUT )
        {
            printf("Warning : receiver timeout\n");
            timeOutPortCount++;
//            CloseHandle( hComm );
            continue;
        }

        char TempChar;
        char SerialBuffer[256];
        DWORD NoByteRead;
        int inByteCount = 0;
        do
        {
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
        for( int i=0; i<inByteCount; i++ )
        {
            if( verifyKey[i]!= SerialBuffer[i] )
            {
                mismatchKey = TRUE;
            }
        }

        if(mismatchKey == FALSE)
        {
            printf("verifying key matches for SiS Device.\n");
            CloseHandle( hComm );
            return CT_EXIT_PASS;
            break;
        }
        else
        {
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
    if( occupiedPortCount>0 )
    {
        printf("(Open com port fail, some com ports were occupied)\n");
    }
    if(mismatchKey == TRUE)
    {
        printf("(Verifying key mismatch)\n");
    }
    if(timeOutPortCount>0)
    {
        printf("(UART RX no response)\n");
    }

    return CT_EXIT_FAIL;
}


FILE* open_firmware_bin( const char* filename)
{
    FILE* input_file = 0;
    errno_t err;

    err = fopen_s( &input_file, filename, "r" );

    if ( err == 0 )
    {
        printf( "Open firmware %s Success.\n", filename );
    }
    else
    {
        printf( "ERROR, Can not open firmware %s.\n", filename );
        return 0;
    }

    fseek( input_file, 0, SEEK_END );

    int file_size = ftell( input_file );

    printf( "Pattern file contains %i bytes\n", file_size );

    fseek( input_file, 0, SEEK_SET );
    return input_file;
}

void print_sep()
{
    printf( "-----\n" );
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    const int argumentCount = QCoreApplication::arguments().size();
    const QStringList argumentList = QCoreApplication::arguments();
    int exitCode = CT_EXIT_AP_FLOW_ERROR;
    QString ComPortName;
    FILE* input_file = 0;
    QTextStream standardOutput(stdout);
    bool userassign = false;

    printVersion();

    //CHECK COMMAND FORMAT
    switch (argumentCount)
    {
        case 1:
            ScanPort();
            exitCode = testserialport(&ComPortName);
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


    // Get the Comm Port of SiS Device
    //printf("\nSerial Port test:");
    if (!userassign) {
        exitCode = testserialport(&ComPortName);
        if (exitCode) {
            return exitCode;
        }
     }

    // OPEN LOCAL FIRMWARE BIN FILE
#if 0
    input_file = open_firmware_bin(argv[1]);
    if ( !input_file )
    {
        printf("Load Firmware Bin File Fails.\n");
        exitCode = EXIT_ERR;
        return exitCode;
    }
#endif

    // OPEN SIS UART COMM PORT
    qDebug() << "Open SiS" << ComPortName << "port";
    serial.setPortName(ComPortName);
    /*
     * 下面這些UART設定預設寫死的
     */
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial.open(QIODevice::ReadWrite)) {
        standardOutput << QObject::tr("Failed to open port %1, error: %2")
              //            .arg(curPortName).arg(serial.errorString())
                          .arg(ComPortName, serial.errorString())
                       << Qt::endl;
        return CT_EXIT_NO_COMPORT;
    }

    printf("Open %s successfully.\n", ComPortName.toStdString().c_str());

    // 1.UPDATE FW
    //TODO: 傳遞Comm Port & Local Bin file給 Do_Update
    exitCode = Do_Update();
    if (exitCode) {
        return exitCode;
    }



    // 2. GET FW ID





    // 3. REVERSE




/*
    try
    {
       //TODO:
    }
    catch(const std::exception& e)
    {
        std::cout << e.what();
    }
    catch(...)
    {
        std::cout << "\nError : unknown expection" << std::endl;
    }
*/
    printf("\nExit code : %d\n", exitCode);

    if(input_file) {
        fclose(input_file);
    }

    if(serial.isOpen()) {
        serial.close();
    }

    return exitCode;

    //  return 0;
    //  return a.exec();
}
