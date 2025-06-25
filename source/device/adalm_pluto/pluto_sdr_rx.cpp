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

}
//-----------------------------------------------------------------------------------------
pluto_sdr_rx::~pluto_sdr_rx()
{
    fprintf(stderr, "pluto_sdr_rx::~pluto_sdr_rx() stop\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::start(iio_device *rx_, rx_thread_data_t *rx_thread_data_)
{
    rx = rx_;
    rx_channel_i = iio_device_find_channel(rx_, "voltage0", false);
    rx_channel_q = iio_device_find_channel(rx_, "voltage1", false);
    iio_channel_enable(rx_channel_i);
    iio_channel_enable(rx_channel_q);
    int e = iio_device_set_kernel_buffers_count(rx_, 2);
    fprintf(stderr, "rx iio_device_set_kernel_buffers_count= %d success=%d\n", 2, e);
    rx_buffer = iio_device_create_buffer(rx_, rx_thread_data_->len_buffer, false);
    is_started = true;
    work(rx_thread_data_);
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::work(rx_thread_data_t *rx_thread_data_)
{
//#define RETING
    static int c = 0;
    rx_thread_data_t *rx_thread_data = rx_thread_data_;

    while(!rx_thread_data->stop){

#ifdef RETING
        auto start_time = std::chrono::steady_clock::now();
#endif
        int bytes = iio_buffer_refill(rx_buffer);

#ifdef RETING
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        auto elapsed_time = elapsed_ns.count();
        fprintf(stderr, "iio_buffer_refill(rx_buffer) reting=  %f\n", elapsed_time / 1000.f);
#endif
        if(rx_thread_data->stop || bytes < 0) {
            rx_thread_data->stop = true;
            fprintf(stderr, "device_rx::work() bytes %d\n", bytes);
            if(bytes == -5){
                rx_thread_data->status = Disconnect;
            }
            else{
                rx_thread_data->status = Unknow;
            }
            break;
        }

        int16_t *ptr_dat = (int16_t*)iio_buffer_first(rx_buffer, rx_channel_i);

        if (rx_thread_data->mutex.try_lock()){
            rx_thread_data->len_buffer = bytes / sizeof(int16_t) / 2;
            rx_thread_data->ptr_buffer = ptr_dat;
            rx_thread_data->ready = true;
            rx_thread_data->mutex.unlock();
            rx_thread_data->cond_value.notify_one();
            c = 0;
        }
        else{
            ++c;
            fprintf(stderr, "pluto_sdr_rx::work: skip buffer %d\n", c);
        }

    }
    rx_thread_data->ready = false;
    rx_thread_data->cond_value.notify_all();
    stop();

    fprintf(stderr, "pluto_sdr_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::stop()
{
    iio_channel_disable(rx_channel_i);
    iio_channel_disable(rx_channel_q);
    iio_buffer_destroy(rx_buffer);
    is_started = false;
}
//-----------------------------------------------------------------------------------------



