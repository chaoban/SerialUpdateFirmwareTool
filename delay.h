#ifndef DELAY_H
#define DELAY_H
/*
 * Delay msec for QT
 */
#include <QEventLoop>
#include <QTimer>

#define _TX_RX_MS_	2    // Delay ms between TX/RX

void msleep(unsigned int msec);

void msleep(unsigned int msec)
{
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    loop.exec();
}
#endif // DELAY_H
