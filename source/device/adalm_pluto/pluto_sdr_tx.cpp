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
#include "pluto_sdr_tx.h"
#include "pluto_sdr.h"

//-----------------------------------------------------------------------------------------
pluto_sdr_tx::pluto_sdr_tx()
{
}
//-----------------------------------------------------------------------------------------
pluto_sdr_tx::~pluto_sdr_tx()
{
    fprintf(stderr, "device_tx::~device_tx() start");
    fprintf(stderr, "device_tx::~device_tx() stop");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::start(iio_device *tx_, tx_thread_data_t *tx_thread_data_)
{
    stop();
    tx = tx_;
    tx_channel_i = iio_device_find_channel(tx, "voltage0", true);
    tx_channel_q = iio_device_find_channel(tx, "voltage1", true);
    iio_channel_enable(tx_channel_i);
    iio_channel_enable(tx_channel_q);
    int e = iio_device_set_kernel_buffers_count(tx, 2);
    fprintf(stderr, "tx iio_device_set_kernel_buffers_count=%d success=%d\n", 2, e);
    tx_buffer = iio_device_create_buffer(tx, TX_PLUTO_LEN_BUFFER, true);
    is_started = true;
    work(tx_thread_data_);
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::stop()
{
    if(is_started){
        is_started = false;
        shutdown();
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::work(tx_thread_data_t *tx_thread_data_)
{
    tx_thread_data_t *tx_thread_data = tx_thread_data_;
    std::unique_lock<std::mutex> read_lock(tx_thread_data->mutex);
    while (!tx_thread_data->stop){
        tx_thread_data->cond_value.wait(read_lock);
        if(tx_thread_data->ready){
            tx_thread_data->ready = false;
            tx_data(tx_thread_data->len_buffer, tx_thread_data->ptr_buffer);
        }
    }
    stop();
//    callback->error_callback(rx_thread_data->status);
    fprintf(stderr, "device_tx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::tx_data(int &len_buffer_, const float *ptr_buffer)
{
    int16_t *p_dat = static_cast<int16_t*>(iio_buffer_first(tx_buffer, tx_channel_i));

    for (int i = 0; i < len_buffer_; ++i){
        p_dat[i] = ptr_buffer[i] * PLUTO_SAMPLES_SCALE;
    }

    uint64_t bytes = iio_buffer_push_partial(tx_buffer, len_buffer_);
    if (bytes < len_buffer_ / sizeof(int)){
        fprintf(stderr, "device_tx::tx_data() lost: need=%d transmitted bytes=%ld\n", len_buffer_, bytes);
    }
    fprintf(stderr, "device_tx::tx_data() len=%d transmitted bytes=%ld\n", len_buffer_, bytes);
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::shutdown()
{
    iio_buffer_destroy(tx_buffer);
    iio_channel_disable(tx_channel_i);
    iio_channel_disable(tx_channel_q);
}
//-----------------------------------------------------------------------------------------


