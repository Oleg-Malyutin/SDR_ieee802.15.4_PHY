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
#include <ctime>
#include <iostream>

#include "device/device_type.h"
#include "rx_usb_plutosdr.h"

#define RX_PLUTO_LEN_BUFFER (16384 / 16)

class pluto_sdr_rx
{
public:
    explicit pluto_sdr_rx();
    ~pluto_sdr_rx();

    bool is_started = false;
    void start(libusb_device_handle *usb_sdr_dev_,
               uint8_t usb_sdr_interface_num_, uint8_t usb_sdr_ep_,
               iio_channel *rssi_channel_,
               rx_thread_data_t *rx_thread_data_);
    void stop();

private:
    rx_usb_plutosdr *usb_rx;

    double rssi = 0.0;
    struct iio_channel *rssi_channel = nullptr;

    int len_buffer = 0;
    std::complex<float> *ptr_buffer;
    std::complex<float> *buffer_a;
    std::complex<float> *buffer_b;
    bool swap = false;

    rx_usb_transfer* transfer;
    static int rx_callback(rx_usb_transfer *transfer_);

    int c = 0;
    rx_thread_data_t *rx_thread_data;

    void work();
    void rx_data(std::unique_lock<std::mutex> &lock_);
    void rx_rssi();
    void rx_off();

};

#endif // PLUTO_SDR_RX_H
