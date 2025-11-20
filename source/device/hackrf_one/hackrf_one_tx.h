#ifndef HACKRF_ONE_TX_H
#define HACKRF_ONE_TX_H

#include <thread>
#include <atomic>

#include "device/hackrf_one/libhackrf/hackrf.h"
#include "device/device_type.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#include "../libusb-1.0/libusb.h"
#else
#include <libusb-1.0/libusb.h>
#endif

class hackrf_one_tx : public rf_tx_callback
{
public:
    explicit hackrf_one_tx();
    ~hackrf_one_tx();

    bool is_started = false;
    void start(libusb_device_handle *usb_device_, std::mutex *rx_tx_mutex_);
    void stop();

private:
    std::mutex *rx_tx_mutex;
    libusb_device_handle* usb_device;
    uint8_t usb_interface_num, usb_ep_out;
    std::vector<uint8_t> buffer{};

    bool tx_data(uint &len_buffer_, const float *ptr_buffer_);
    void tx_usb(uint32_t len_transfer_);

};

#endif // HACKRF_ONE_TX_H
