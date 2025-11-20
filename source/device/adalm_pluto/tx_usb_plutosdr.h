#pragma once

#include <vector>
#include <thread>
#include <atomic>

#include "iio.h"
#include "device/device_type.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#include "../libusb-1.0/libusb.h"
#else
#include <libusb-1.0/libusb.h>
#endif


class tx_usb_plutosdr
{
public:
    tx_usb_plutosdr();
    ~tx_usb_plutosdr();

    int start(libusb_device_handle* usb_sdr_dev_,
              uint8_t usb_sdr_interface_num_, uint8_t  usb_sdr_ep_out_,
              uint32_t num_channels_, uint32_t buffer_size_samples_);
    int tx_data(uint &len_buffer_, const float *ptr_buffer_);
    void stop();

private:
    libusb_device_handle* usb_sdr_dev;
    uint8_t usb_sdr_interface_num, usb_sdr_ep_out;
    std::thread thread;
    std::atomic<bool> thread_stop;
    // channel bitmask
    uint32_t enabled_channels;
    bool event_ready = false;
    std::condition_variable event;
    std::mutex event_mutex;
    std::vector<uint8_t> buffer{};
    void thread_func(uint32_t enabled_channels_, uint32_t buffer_size_samples_);

};
