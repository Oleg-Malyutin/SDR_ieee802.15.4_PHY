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



//-----------------------------------------------------------------------------------------
pluto_sdr_rx::pluto_sdr_rx()
{
    buffer_a = new std::complex<float>[RX_PLUTO_LEN_BUFFER * 2];
    buffer_b = new std::complex<float>[RX_PLUTO_LEN_BUFFER * 2];
    transfer = new rx_usb_transfer;
    usb_rx = new rx_usb_plutosdr;
}
//-----------------------------------------------------------------------------------------
pluto_sdr_rx::~pluto_sdr_rx()
{
    fprintf(stderr, "pluto_sdr_rx::~pluto_sdr_rx() start\n");
    delete usb_rx;
    delete transfer;
    delete [] buffer_a;
    delete [] buffer_b;
    fprintf(stderr, "pluto_sdr_rx::~pluto_sdr_rx() stop\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::start(libusb_device_handle* usb_sdr_dev_,
                         uint8_t usb_sdr_interface_num_, uint8_t usb_sdr_ep_,
                         iio_channel *rssi_channel_,
                         rx_thread_data_t *rx_thread_data_)
{
    rssi_channel = rssi_channel_;
    rx_thread_data = rx_thread_data_;
    stop();

    transfer->usb_event_ready = false;
    rx_thread_data->status = Connect;

    transfer->ctx = this;
    usb_rx->start(usb_sdr_dev_, usb_sdr_interface_num_, usb_sdr_ep_,
                  rssi_channel_, 1, RX_PLUTO_LEN_BUFFER, transfer, rx_callback);

    is_started = true;

    work();
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::stop()
{
    while(is_started){
        is_started = false;
        usb_rx->stop();
        rx_thread_data->stop = true;
        transfer->usb_event_ready = false;
        transfer->usb_event_mutex.unlock();
        transfer->usb_event.notify_all();
        rx_thread_data->ready = false;
        rx_thread_data->mutex.unlock();
        rx_thread_data->condition_value.notify_all();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
//-----------------------------------------------------------------------------------------
int pluto_sdr_rx::rx_callback(rx_usb_transfer* transfer_)
{
//    auto start_time = std::chrono::steady_clock::now().time_since_epoch();
//    static std::chrono::nanoseconds end_time;

    pluto_sdr_rx *ctx = reinterpret_cast<pluto_sdr_rx*>(transfer_->ctx);
    int len_buffer = transfer_->len_buffer / 4;
    int16_t *buffer = reinterpret_cast<int16_t*>(transfer_->buffer);
    std::complex<float> *ptr_buffer = ctx->swap ? ctx->buffer_b : ctx->buffer_a;
    ctx->swap = !ctx->swap;

    int j = 0;
    for(int i = 0; i < len_buffer; ++i){
        ptr_buffer[i].real(buffer[j] * 2e-3f);
        ptr_buffer[i].imag(buffer[j + 1] * 2e-3f);
        j += 2;
    }
//    auto end_time = std::chrono::steady_clock::now().time_since_epoch();
//    long int elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

//    long int elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(start_time -  end_time).count();
//    std::cout << "elapsed_time: " << "   " << elapsed_time << " ns" << std::endl;
//    end_time = start_time;

    transfer_->usb_event_mutex.lock();

    if(transfer_->usb_event_ready == false){
        ctx->len_buffer = len_buffer;
        ctx->ptr_buffer = ptr_buffer;
        transfer_->usb_event_ready = true;

        transfer_->usb_event_mutex.unlock();

        transfer_->usb_event.notify_all();
    }
    else{

        transfer_->usb_event_mutex.unlock();

//        fprintf(stderr, "pluto_sdr_rx::rx_callback: skip------\n");
    }


    return 0;

}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::work()
{
    rx_thread_data->stop = false;
    rx_thread_data->set_mode = rx_mode_data;
    std::unique_lock<std::mutex> lock(transfer->usb_event_mutex);

    while(!rx_thread_data->stop){

        if(rx_thread_data->set_mode == rx_mode_data){
            rx_data(lock);
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

    stop();

    fprintf(stderr, "pluto_sdr_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::rx_data(std::unique_lock<std::mutex> &lock_)
{
    transfer->usb_event.wait(lock_);
    if(transfer->usb_event_ready){
        transfer->usb_event_ready = false;
        if (rx_thread_data->mutex.try_lock()){
            rx_thread_data->len_buffer = len_buffer;
            rx_thread_data->ptr_buffer = ptr_buffer;
            rx_thread_data->mode = rx_mode_data;
            rx_thread_data->ready = true;
            rx_thread_data->mutex.unlock();
            rx_thread_data->condition_value.notify_one();
            c = 0;
        }
        else{
            ++c;
//            fprintf(stderr, "pluto_sdr_rx::rx_data: skip buffer %d\n", c);
        }
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_rx::rx_rssi()
{
    double rssi;
    int err = iio_channel_attr_read_double(rssi_channel, "rssi", &rssi);

    rx_thread_data->mutex.lock();
    rx_thread_data->rssi = -rssi;

//    fprintf(stderr, "pluto_sdr_rx::rx_rssi rssi %f\n", -rssi);

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




