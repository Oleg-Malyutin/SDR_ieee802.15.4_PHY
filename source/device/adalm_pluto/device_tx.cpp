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
#include "device_tx.h"
#include "pluto_sdr.h"

//-----------------------------------------------------------------------------------------
device_tx::device_tx(QObject *parent) : QObject(parent)
{
}
//-----------------------------------------------------------------------------------------
device_tx::~device_tx()
{
    qDebug() << "device_tx::~device_tx() start";
    if(init_ready) shutdown();
    qDebug() << "device_tx::~device_tx() stop";
}
//-----------------------------------------------------------------------------------------
bool device_tx::init(iio_device *tx_)
{
    tx_channel_i = iio_device_find_channel(tx_, "voltage0", true);
    tx_channel_q = iio_device_find_channel(tx_, "voltage1", true);
//    iio_channel_enable(tx_channel_i);
//    iio_channel_enable(tx_channel_q);

    iio_channel_disable(tx_channel_i);
    iio_channel_disable(tx_channel_q);

//    tx_buffer = iio_device_create_buffer(tx_, TX_BUFFER, true);

    init_ready = true;

    return init_ready;
}
//-----------------------------------------------------------------------------------------
void device_tx::start()
{
//    emit tx_ptr_data((int16_t*)iio_buffer_first(tx_buffer, tx_channel_i), TX_BUFFER);
}
//-----------------------------------------------------------------------------------------
void device_tx::tx_data()
{
    iio_buffer_push(tx_buffer);
    emit tx_ptr_data((int16_t*)iio_buffer_first(tx_buffer, tx_channel_i), TX_BUFFER);
}
//-----------------------------------------------------------------------------------------
void device_tx::stop()
{

}
//-----------------------------------------------------------------------------------------
void device_tx::shutdown()
{
//    iio_buffer_destroy(tx_buffer);
//    iio_channel_disable(tx_channel_i);
//    iio_channel_disable(tx_channel_q);
}
//-----------------------------------------------------------------------------------------


