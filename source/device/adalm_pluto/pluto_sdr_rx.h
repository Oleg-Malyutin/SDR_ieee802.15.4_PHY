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
#ifndef PLUTO_SDR_RX_H
#define PLUTO_SDR_RX_H

#include <iio.h>

#include "device/device_type.h"

#define RX_PLUTO_LEN_BUFFER (16384 / 2)
#define PLUTO_TX_GAIN_MIN -80
#define PLUTO_TX_GAIN_MAX 0
#define PLUTO_SAMPLES_SCALE  32767.0f

class pluto_sdr_rx
{
public:
    explicit pluto_sdr_rx();
    ~pluto_sdr_rx();

    bool is_started = false;
    void start(iio_channel *ch_rx_, iio_device *dev_rx_, rx_thread_data_t *rx_thread_data_);

private:
    iio_device *dev_rx;
    struct iio_channel *rssi_channel = nullptr;
    struct iio_channel *rx_channel_i = nullptr;
    struct iio_channel *rx_channel_q = nullptr;
    struct iio_buffer *rx_buffer = nullptr;
    std::complex<float> *buffer;
//    moving_average<float, 32> avg_energy_detect;
    int c = 0;
    rx_thread_data_t *rx_thread_data;
    void work();
    void rx_data();
    void rx_rssi();
    void rx_off();
    void stop();
    void shutdown();
};

#endif // PLUTO_SDR_RX_H
