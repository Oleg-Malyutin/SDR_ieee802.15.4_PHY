#ifndef SDRUSBGADGET_H
#define SDRUSBGADGET_H

#if defined(_WIN32) || defined(__CYGWIN__)
#include "../libusb-1.0/libusb.h"
#else
#include <libusb-1.0/libusb.h>
#endif

#include "iio.h"

class sdrusbgadget
{
public:
    sdrusbgadget();
    ~sdrusbgadget();

    libusb_device_handle *usb_sdr_dev;
    uint8_t usb_sdr_interface_num, usb_sdr_ep_in, usb_sdr_ep_out;
    void open(iio_context* context);
    void close();

private:
    libusb_context *usb_ctx = nullptr;
};

#endif // SDRUSBGADGET_H
