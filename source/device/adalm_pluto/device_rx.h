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
#ifndef DEVICE_RX_H
#define DEVICE_RX_H

#include <iio.h>

#include "device/device_type.h"

#define RX_PLUTO_LEN_BUFFER (16384 / 2)

class device_rx
{
public:
    explicit device_rx();
    ~device_rx();

    bool is_started = false;
    void start(iio_device *rx_, rx_thread_data_t *rx_thread_data_);


private:
    struct iio_channel *rx_channel_i = nullptr;
    struct iio_channel *rx_channel_q = nullptr;
    struct iio_buffer *rx_buffer = nullptr;
    void work(rx_thread_data_t *rx_thread_data_);
    void stop();
};

#endif // DEVICE_RX_H
