#include "hackrf_one_rx.h"

#include <iostream>

//-----------------------------------------------------------------------------------------
hackrf_one_rx::hackrf_one_rx()
{
    transfer = new rx_usb_transfer;
    buffer_a = new std::complex<float>[RX_USB_BUFFER];
    buffer_b = new std::complex<float>[RX_USB_BUFFER];
    usb_rx = new rx_usb_hackrf;
}
//-----------------------------------------------------------------------------------------
hackrf_one_rx::~hackrf_one_rx()
{
    delete usb_rx;
    delete[] buffer_a;
    delete[] buffer_b;
    delete transfer;
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::start(rx_thread_data_t *rx_thread_data_,
                          libusb_device_handle *usb_device_, std::mutex *rx_tx_mutex_)
{
    rx_thread_data = rx_thread_data_;
    rx_thread_data->stop = false;
    rx_thread_data->set_mode = rx_mode_data;
    rx_thread_data->status = Connect;

    transfer->ctx = this;
    transfer->usb_event_ready = false;

    usb_rx->start(usb_device_, transfer, rx_callback, rx_tx_mutex_);

    is_started = true;

    work();
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::stop()
{

}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::rx_callback(rx_usb_transfer* transfer_)
{
    hackrf_one_rx *ctx = static_cast<hackrf_one_rx*>(transfer_->ctx);

    int len_buffer = transfer_->len_buffer / 2;
    int8_t* buffer = (int8_t*)transfer_->buffer;
    std::complex<float> *ptr_buffer = ctx->swap ? ctx->buffer_a : ctx->buffer_b;
    ctx->swap = !ctx->swap;

    int j = 0;
    for(int i = 0; i < len_buffer; ++i){
        ptr_buffer[i].real(buffer[j] * 1e-2f);
        ptr_buffer[i].imag(buffer[j + 1] * 1e-2f);
        j += 2;
    }

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

//        fprintf(stderr, "hackrf_one_rx::callback: skip------\n");
    }

}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::work()
{
    rx_thread_data->stop = false;
    rx_thread_data->set_mode = rx_mode_data;
    std::unique_lock<std::mutex> lock(transfer->usb_event_mutex);

    while(!rx_thread_data->stop){
        if(rx_thread_data->set_mode == rx_mode_data){
            rx_data(lock);
        }
        else if(rx_thread_data->set_mode == rx_mode_rssi){
            rx_rssi(lock);
        }
        else if(rx_thread_data->set_mode == rx_mode_off){
            rx_off();
        }
    }

    stop();

    fprintf(stderr, "hackrf_one_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::rx_data(std::unique_lock<std::mutex> &lock_)
{
    transfer->usb_event.wait(lock_);
    if(transfer->usb_event_ready){
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
//            fprintf(stderr, "hackrf_one_rx::rx_data: skip buffer %d\n", c);
        }
        transfer->usb_event_ready = false;
    }
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::rx_rssi(std::unique_lock<std::mutex> &lock_)
{
    transfer->usb_event.wait(lock_);
    if(transfer->usb_event_ready){
        float sum = 0.0f;
        for(int i = 0; i < len_buffer; ++i){
            sum += norm(ptr_buffer[i] / 1.271f);
        }
        sum /= len_buffer;
        float rssi = 10.0f * log10f(sum) - 48.0f;// (-48.0f) this is: -31 - noise with antenna, -76 treshold for CCA and -3 margin

        rx_thread_data->mutex.lock();
        rx_thread_data->rssi = rssi;
        rx_thread_data->ready = true;
        rx_thread_data->mode = rx_mode_rssi;
        rx_thread_data->mutex.unlock();
        rx_thread_data->condition_value.notify_one();
        transfer->usb_event_ready = false;
    }
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::rx_off()
{
    rx_thread_data->mutex.lock();
    rx_thread_data->ready = true;
    rx_thread_data->mode = rx_mode_off;
    rx_thread_data->mutex.unlock();
    rx_thread_data->condition_value.notify_one();

    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // TODO 8 symbols
}
//-----------------------------------------------------------------------------------------

