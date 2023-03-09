#ifndef DELAY_H
#define DELAY_H

#include <QEventLoop>
#include <QTimer>

void msleep(unsigned int msec);

/*
 * Delay msec for QT
 *
 */

void msleep(unsigned int msec)
{
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    loop.exec();
}

#endif // DELAY_H
