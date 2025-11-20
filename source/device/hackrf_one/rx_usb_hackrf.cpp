#include "rx_usb_hackrf.h"

#define RX_ENDPOINT_ADDRESS (LIBUSB_ENDPOINT_IN | 1)
#define HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE 1
#define HACKRF_TRANSCEIVER_MODE_RECEIVE 1
#define HACKRF_TRANSCEIVER_MODE_OFF 0

//------------------------------------------------------------------------------------------------
rx_usb_hackrf::rx_usb_hackrf()
{

}
//------------------------------------------------------------------------------------------------
void rx_usb_hackrf:: start(libusb_device_handle *usb_device_, rx_usb_transfer *transfer_,
                           usb_cb_fn callback_, std::mutex *rx_tx_mutex_)
{
    thread_stop = false;
    uint8_t usb_interface_num = 0;
    uint8_t usb_ep_in =  RX_ENDPOINT_ADDRESS;
    uint32_t buffer_size_samples = RX_USB_BUFFER;
    thread = std::thread(&rx_usb_hackrf::thread_func, this,
                         usb_device_, usb_interface_num, usb_ep_in,
                         buffer_size_samples, transfer_, callback_, rx_tx_mutex_);
}
//------------------------------------------------------------------------------------------------
void rx_usb_hackrf::thread_func(libusb_device_handle *usb_device_,
                                uint8_t usb_interface_num_, uint8_t usb_ep_in_,
                                uint32_t buffer_size_samples_,
                                rx_usb_transfer *transfer_,
                                usb_cb_fn callback_, std::mutex *rx_tx_mutex_)
{
    // prepare stream
    static libusb_device_handle *usb_device = usb_device_;
    rx_usb_transfer *transfer = transfer_;
    usb_cb_fn callback = callback_;
    uint16_t value = HACKRF_TRANSCEIVER_MODE_RECEIVE;
    int rc = libusb_control_transfer(usb_device,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                     LIBUSB_RECIPIENT_DEVICE,
                                     HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE,
                                     value,
                                     usb_interface_num_,
                                     NULL,
                                     0,
                                     1);

    if(rc < 0) {
        fprintf(stderr, "Failed to start RX stream (%d)\n", rc);

        return;

    }
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // calculate buffer size in bytes
    size_t buffer_size_bytes = buffer_size_samples_ * sizeof(int8_t) * 2;
    // allocate buffer
    uint8_t buffer[buffer_size_bytes * 2];
    bool swap = false;
    // start stream
    while(!thread_stop.load()) {
        int bytes_transferred = 0;
        uint8_t *ptr_buffer = swap ? buffer + buffer_size_bytes : buffer;
        swap = !swap;
//        rx_tx_mutex_->lock();
        int rc = libusb_bulk_transfer(usb_device, usb_ep_in_, ptr_buffer, buffer_size_bytes,
                                      &bytes_transferred, 2);
//        rx_tx_mutex_->unlock();

        if(LIBUSB_SUCCESS == rc && ((size_t)bytes_transferred == buffer_size_bytes)) {
            transfer->buffer = ptr_buffer;
            transfer->len_buffer = buffer_size_bytes;
            callback(transfer);
//            fprintf(stderr, "hackrf_one_rx::thread_func:bytes_rx (%d)\n", bytes_transferred);
        }
        else{
            fprintf(stderr, "hackrf_one_rx::thread_func: ---------------------bytes_rx (%d) err (%d)\n", bytes_transferred, rc);
        }
    }
    // stop stream
    value = HACKRF_TRANSCEIVER_MODE_OFF;
    rc = libusb_control_transfer(usb_device,
                                 LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                 LIBUSB_RECIPIENT_DEVICE,
                                 HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE,
                                 value,
                                 usb_interface_num_,
                                 NULL,
                                 0,
                                 100);

    if(rc < 0) {
        fprintf(stderr, "Failed to stop RX stream (%d)", rc);
    }
    fprintf(stderr, "Stop RX stream");
}
//------------------------------------------------------------------------------------------------
void rx_usb_hackrf::stop()
{
    if(thread.joinable()) {
        thread_stop = true;
        thread.join();
    }
}
//------------------------------------------------------------------------------------------------
