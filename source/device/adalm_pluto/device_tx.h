/*
 *  Copyright 2025 Oleg Malyutin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef DEVICE_TX_H
#define DEVICE_TX_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>

#include <iio.h>

#include <QDebug>

#define TX_BUFFER 16384

class device_tx : public QObject
{
    Q_OBJECT
public:
    explicit device_tx(QObject *parent = nullptr);
    ~device_tx();

    bool init(iio_device *tx_);

signals:
    void tx_ptr_data(int16_t* ptr_, int len_);

public slots:
    void start();
    void tx_data();
    void stop();

private:
    struct iio_channel* tx_channel_i = nullptr;
    struct iio_channel* tx_channel_q = nullptr;
    struct iio_buffer* tx_buffer = nullptr;
    bool init_ready = false;
    void shutdown();

};

#endif // DEVICE_TX_H
