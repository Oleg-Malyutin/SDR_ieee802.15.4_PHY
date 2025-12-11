#ifndef HACKRF_ONE_RX_H
#define HACKRF_ONE_RX_H

#include <thread>
#include <atomic>

//#include "device/hackrf_one/libhackrf/hackrf.h"
#include "device/hackrf_one/rx_usb_hackrf.h"
//#include "device/device_type.h"

class hackrf_one_rx
{
public:
    explicit hackrf_one_rx();
    ~hackrf_one_rx();

    bool is_started = false;
    void start(rx_thread_data_t *rx_thread_data_,
               libusb_device_handle *usb_device_, std::mutex *rx_tx_mutex_);

private:
    rx_thread_data_t *rx_thread_data;
    rx_usb_hackrf *usb_rx;
    libusb_device_handle *usb_device;
    rx_usb_transfer *transfer;
    constexpr static float scale = 1.0f / 127.0f;
    int len_buffer = 0;
    std::complex<float> *ptr_buffer;
    std::complex<float> *buffer_a;
    std::complex<float> *buffer_b;
    bool swap = false;

    static void rx_callback(rx_usb_transfer *transfer_);
    void work();
    void rx_data(std::unique_lock<std::mutex> &lock_);
    void stop();

};

#endif // HACKRF_ONE_RX_H
