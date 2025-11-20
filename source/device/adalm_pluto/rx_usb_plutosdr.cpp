#include "rx_usb_plutosdr.h"

#include <QDebug>

#define SDR_USB_GADGET_COMMAND_START (0x10)
#define SDR_USB_GADGET_COMMAND_STOP (0x11)
#define SDR_USB_GADGET_COMMAND_TARGET_RX (0x0)

typedef struct{
    uint32_t enabled_channels;
    uint32_t buffer_size;
} cmd_usb_start_request_t;

//------------------------------------------------------------------------------------------------
rx_usb_plutosdr::rx_usb_plutosdr()
{

}
//------------------------------------------------------------------------------------------------
void rx_usb_plutosdr:: start(libusb_device_handle* usb_sdr_dev_,
                             uint8_t usb_sdr_interface_num_, uint8_t usb_sdr_ep_in_,
                             iio_channel *rssi_channel_,
                             unsigned int num_channels, uint32_t buffer_size_samples_,
                             rx_usb_transfer *transfer_, usb_plutosdr_cb_fn callback_)
{
    if(!thread.joinable()) {

        enabled_channels = 0;
        for (unsigned int i = 0; i < num_channels * 2; i++) {
            // add bit to enabled channels
            enabled_channels |= (1 << i);
        }

        thread_stop = false;
        usb_sdr_dev = usb_sdr_dev_;
        usb_sdr_interface_num = usb_sdr_interface_num_;
        usb_sdr_ep_in =  usb_sdr_ep_in_;
        rssi_channel = rssi_channel_;
        uint32_t buffer_size_samples = buffer_size_samples_/* * 2*/;
        thread = std::thread(&rx_usb_plutosdr::thread_func, this, enabled_channels,
                             buffer_size_samples, transfer_, callback_);

         // attempt to increase thread priority
//         int max_prio = sched_get_priority_max(SCHED_RR);
//         if(max_prio >= 0) {
//             sched_param sch;
//             sch.sched_priority = max_prio;
//             if(int rc = pthread_setschedparam(thread.native_handle(), SCHED_RR, &sch)) {
//                 fprintf(stderr, "Failed to set RX thread priority (%d)\n", rc);
//             }
//         }
//         else {
//             fprintf(stderr, "Failed to query thread schedular priorities\n");
//         }
    }
}
//------------------------------------------------------------------------------------------------
void rx_usb_plutosdr::thread_func(uint32_t enabled_channels_, uint32_t buffer_size_samples_,
                                  rx_usb_transfer *transfer_, usb_plutosdr_cb_fn callback_)
{
    cmd_usb_start_request_t cmd;
    // start stream
    cmd.enabled_channels = enabled_channels_;
    cmd.buffer_size = buffer_size_samples_;

    int rc = libusb_control_transfer(usb_sdr_dev,
                                     LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                     SDR_USB_GADGET_COMMAND_START,
                                     SDR_USB_GADGET_COMMAND_TARGET_RX,
                                     usb_sdr_interface_num,
                                     (unsigned char*)&cmd,
                                     sizeof(cmd),
                                     1000);
    if(rc < 0) {
        fprintf(stderr, "Failed to start RX stream (%d)\n", rc);
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // calculate buffer size in bytes
    size_t buffer_size_bytes = buffer_size_samples_ * sizeof(int16_t) * 2;
    // allocate buffer
    uint8_t buffer[buffer_size_bytes * 2];
    bool swap = false;

    rx_usb_transfer *transfer = transfer_;
    usb_plutosdr_cb_fn callback = callback_;

    while(!thread_stop.load()) {
            // read data with 0.0s timeout
            int bytes_transferred = 0;
            uint8_t *ptr_buffer = swap ? buffer + buffer_size_bytes : buffer;
            swap = !swap;
            int rc = libusb_bulk_transfer(usb_sdr_dev, usb_sdr_ep_in, ptr_buffer, buffer_size_bytes,
                                          &bytes_transferred, 1);

            if(LIBUSB_SUCCESS == rc && ((size_t)bytes_transferred == buffer_size_bytes)) {
                transfer->buffer = ptr_buffer;
                transfer->len_buffer = buffer_size_bytes;
                callback(transfer);
            }
            else{
//                fprintf(stderr, "rx_usb_plutosdr::thread_func: ---------------------bytes_transferred (%d)\n", bytes_transferred);
            }
    }

    // stop stream
    rc = libusb_control_transfer(usb_sdr_dev,
                                 LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                 SDR_USB_GADGET_COMMAND_STOP,
                                 SDR_USB_GADGET_COMMAND_TARGET_RX,
                                 usb_sdr_interface_num,
                                 nullptr,
                                 0,
                                 1000);
    if(rc < 0) {
        fprintf(stderr, "Failed to stop RX stream (%d)", rc);
        return;
    }
}
//------------------------------------------------------------------------------------------------
void rx_usb_plutosdr::stop()
{
    if(thread.joinable()) {
        thread_stop = true;
        thread.join();
    }
}
//------------------------------------------------------------------------------------------------
