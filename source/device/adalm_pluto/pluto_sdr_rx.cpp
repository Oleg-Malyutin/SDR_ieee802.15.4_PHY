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
#include "pluto_sdr_rx.h"

#include <iostream>
#include <ctime>

//-----------------------------------------------------------------------------------------
pluto_sdr_rx::pluto_sdr_rx()
{
    dev_rx = nullptr;
}
//-----------------------------------------------------------------------------------------
pluto_sdr_rx::~pluto_sdr_rx()
{
    fprintf(stderr, "pluto_sdr_rx::~pluto_sdr_rx() start\n");
    fprintf(stderr, "pluto_sdr_rx::~pluto_sdr_rx() stop\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::start(iio_channel *ch_rx_, iio_device *dev_rx_, rx_thread_data_t *rx_thread_data_)
{
    rssi_channel = ch_rx_;
    dev_rx = dev_rx_;
    rx_channel_i = iio_device_find_channel(dev_rx_, "voltage0", false);
    rx_channel_q = iio_device_find_channel(dev_rx_, "voltage1", false);
    iio_channel_enable(rx_channel_i);
    iio_channel_enable(rx_channel_q);
    uint b = 2;
    int e = iio_device_set_kernel_buffers_count(dev_rx_, b);
    fprintf(stderr, "rx iio_device_set_kernel_buffers_count= %d success=%d\n", b, e);
    rx_thread_data = rx_thread_data_;
    rx_buffer = iio_device_create_buffer(dev_rx_, rx_thread_data->len_buffer, false);
    buffer = new std::complex<float>[rx_thread_data->len_buffer];
    rx_thread_data->status = Connect;
    is_started = true;
    work();
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::stop()
{
    if(is_started){
        is_started = false;
        shutdown();
        delete [] buffer;
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::work()
{
    rx_thread_data->stop_rx = false;
    rx_thread_data->set_mode = rx_mode_data;

    while(!rx_thread_data->stop_rx){

        if(rx_thread_data->set_mode == rx_mode_data){
            rx_data();
        }
        else if(rx_thread_data->set_mode == rx_mode_rssi){
            rx_rssi();
        }
        else if(rx_thread_data->set_mode == rx_mode_off){
            rx_off();
        }

        if(rx_thread_data->status != Connect){

            break;

        }

    }

    rx_thread_data->ready = false;
    stop();

    fprintf(stderr, "pluto_sdr_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::rx_data()
{
    int bytes = iio_buffer_refill(rx_buffer);

    if(rx_thread_data->stop_rx || bytes < 0) {
        rx_thread_data->stop_rx = true;
        fprintf(stderr, "pluto_sdr_rx::work: bytes %d\n", bytes);
        if(bytes == -5){
            rx_thread_data->status = Disconnect;
        }
        else{
            rx_thread_data->status = Unknow;
        }

        return;
    }

    int16_t *ptr_dat = (int16_t*)iio_buffer_first(rx_buffer, rx_channel_i);
    int len = bytes / sizeof(int16_t) / 2;
    int j = 0;
    for(int i = 0; i < len; ++i){
        std::complex<float> data;
        data.real(ptr_dat[j] * 2e-3f);
        data.imag(ptr_dat[j + 1] * 2e-3f);
        buffer[i] = data;
        j += 2;
    }

    if (rx_thread_data->mutex.try_lock()){
        rx_thread_data->len_buffer = len;
        rx_thread_data->ptr_buffer = buffer;
        rx_thread_data->ready = true;
        rx_thread_data->mode = rx_mode_data;
        rx_thread_data->mutex.unlock();
        rx_thread_data->condition_value.notify_one();
        c = 0;
    }
    else{
        ++c;
        fprintf(stderr, "pluto_sdr_rx::work: skip buffer %d\n", c);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::rx_rssi()
{
    double rssi;
    int err = iio_channel_attr_read_double(rssi_channel, "rssi", &rssi);

    rx_thread_data->mutex.lock();
    rx_thread_data->rssi = -rssi;
    if(err != 0){
        rx_thread_data->status = Unknow;
    }
    rx_thread_data->ready = true;
    rx_thread_data->mode = rx_mode_rssi;
    rx_thread_data->mutex.unlock();
    rx_thread_data->condition_value.notify_one();
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::rx_off()
{
    rx_thread_data->mutex.lock();
    rx_thread_data->ready = true;
    rx_thread_data->mode = rx_mode_off;
    rx_thread_data->mutex.unlock();
    rx_thread_data->condition_value.notify_one();

    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // TODO 8 symbols
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::shutdown()
{
    iio_channel_disable(rx_channel_i);
    iio_channel_disable(rx_channel_q);
    iio_buffer_destroy(rx_buffer);
}
//-----------------------------------------------------------------------------------------



