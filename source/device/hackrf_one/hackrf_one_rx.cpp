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
    swap = false;
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

    for(int i = 0; i < len_buffer; ++i){
        ptr_buffer[i].real(buffer[0] * scale);
        ptr_buffer[i].imag(buffer[1] * scale);
        buffer += 2;
    }

    transfer_->usb_event_mutex.lock();

    ctx->len_buffer = len_buffer;
    ctx->ptr_buffer = ptr_buffer;
    transfer_->usb_event_ready = true;

    transfer_->usb_event_mutex.unlock();
    transfer_->usb_event.notify_all();
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::work()
{
    rx_thread_data->stop = false;
    rx_thread_data->set_mode = rx_mode_data;
    std::unique_lock<std::mutex> lock(transfer->usb_event_mutex);

    while(!rx_thread_data->stop){

        if(rx_thread_data->status != Connect){

            break;

        }

        rx_data(lock);
    }

    stop();

    fprintf(stderr, "hackrf_one_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void hackrf_one_rx::rx_data(std::unique_lock<std::mutex> &lock_)
{
    transfer->usb_event.wait(lock_);
    if(transfer->usb_event_ready){
        transfer->usb_event_ready = false;
        float sum = 0.0f;
        for(int i = 0; i < len_buffer; ++i){
            sum += norm(ptr_buffer[i]);
        }
        float len_avg = len_buffer;
        float rssi = 10.0f * log10(sum / len_avg * 0.707f) - 72.0f;
        rx_thread_data->mutex.lock();
        rx_thread_data->len_buffer = len_buffer;
        rx_thread_data->ptr_buffer = ptr_buffer;
        rx_thread_data->rssi = rssi;
        rx_thread_data->ready = true;
        rx_thread_data->mutex.unlock();
        rx_thread_data->condition.notify_one();
    }
}
//-----------------------------------------------------------------------------------------


