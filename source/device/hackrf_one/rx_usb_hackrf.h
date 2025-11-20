#ifndef RX_USB_HACKRF_H
#define RX_USB_HACKRF_H

#include <thread>
#include <atomic>
#if defined(_WIN32) || defined(__CYGWIN__)
#include "../libusb-1.0/libusb.h"
#else
#include <libusb-1.0/libusb.h>
#endif

#include "device/hackrf_one/libhackrf/hackrf.h"
#include "device/device_type.h"

#define RX_USB_BUFFER 512

typedef void (*usb_cb_fn)(rx_usb_transfer* transfer_);

class rx_usb_hackrf
{
public:
    rx_usb_hackrf();

    void start(libusb_device_handle *usb_device_,
               rx_usb_transfer *transfer_,
               usb_cb_fn callback_, std::mutex *rx_tx_mutex_);
    void stop();

private:
    std::thread thread;
    std::atomic<bool> thread_stop;

    void thread_func(libusb_device_handle *usb_device_,
                     uint8_t usb_interface_num_, uint8_t usb_ep_in_,
                     uint32_t buffer_size_samples_,
                     rx_usb_transfer *transfer_,
                     usb_cb_fn callback_, std::mutex *rx_tx_mutex_);
};

#endif // RX_USB_HACKRF_H
