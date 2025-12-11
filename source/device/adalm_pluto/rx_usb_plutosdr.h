#ifndef RX_USB_PLUTOSDR_H
#define RX_USB_PLUTOSDR_H

#include <stdexcept>
#include <vector>
#include <thread>
#include <atomic>

#if defined(_WIN32) || defined(__CYGWIN__)
#include "../libusb-1.0/libusb.h"
#else
#include <libusb-1.0/libusb.h>
#endif

#include "iio.h"
#include "device/device_type.h"
//#include "utils/thread_safe_queue.hh"


typedef int (*usb_plutosdr_cb_fn)(rx_usb_transfer* transfer_);

class rx_usb_plutosdr
{
public:
    rx_usb_plutosdr();
    void start(libusb_device_handle* usb_sdr_dev_,
               uint8_t usb_sdr_interface_num_, uint8_t usb_sdr_ep_in_,
               unsigned int num_channels, uint32_t buffer_size_samples_,
               rx_usb_transfer *transfer_, usb_plutosdr_cb_fn callback_);
    void stop();

private:
    libusb_device_handle *usb_sdr_dev;
    uint8_t usb_sdr_interface_num, usb_sdr_ep_in, usb_sdr_ep_out;
    // channel bitmask
    uint32_t enabled_channels;
    std::thread thread;
    std::atomic<bool> thread_stop;

    void thread_func(uint32_t enabled_channels_,
                     uint32_t buffer_size_samples_, rx_usb_transfer *transfer_,
                     usb_plutosdr_cb_fn callback_);
};

#endif // RX_USB_PLUTOSDR_H
