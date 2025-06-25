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
#ifndef PLUTO_SDR_TX_H
#define PLUTO_SDR_TX_H

#include <iio.h>

#include "device/device_type.h"

class pluto_sdr_tx
{
public:
    explicit pluto_sdr_tx();
    ~pluto_sdr_tx();

    bool is_started = false;
    void start(iio_device *tx_, tx_thread_data_t *tx_thread_data_);

private:
    iio_device *tx;
    struct iio_channel* tx_channel_i = nullptr;
    struct iio_channel* tx_channel_q = nullptr;
    struct iio_buffer* tx_buffer = nullptr;
    void work(tx_thread_data_t *tx_thread_data_);
    void tx_data(int &len_buffer_, const float *ptr_buffer);
    void stop();
    void shutdown();

};

#endif // PLUTO_SDR_TX_H
