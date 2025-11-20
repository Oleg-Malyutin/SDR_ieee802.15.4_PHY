#include "hackrf_one_tx.h"

#define TX_ENDPOINT_ADDRESS (LIBUSB_ENDPOINT_OUT | 2)
#define HACKRF_TRANSCEIVER_MODE_RECEIVE 1
#define HACKRF_TRANSCEIVER_MODE_TRANSMIT 2
#define HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE 1
#define HACKRF_TRANSCEIVER_MODE_OFF 0
#define TX_USB_BUFFER 1024

//-----------------------------------------------------------------------------------------
hackrf_one_tx::hackrf_one_tx()
{

}
//-----------------------------------------------------------------------------------------
hackrf_one_tx::~hackrf_one_tx()
{

}
//-----------------------------------------------------------------------------------------
void hackrf_one_tx::start(libusb_device_handle *usb_device_, std::mutex *rx_tx_mutex_)
{
    usb_device = usb_device_;
    usb_interface_num = 0;
    usb_ep_out =  TX_ENDPOINT_ADDRESS;
    rx_tx_mutex = rx_tx_mutex_;

    is_started = true;
}
//-----------------------------------------------------------------------------------------
bool hackrf_one_tx::tx_data(uint &len_buffer_, const float *ptr_buffer_)
{
    uint len_transfer = TX_USB_BUFFER;
    while(len_transfer < len_buffer_){
        len_transfer += TX_USB_BUFFER;
    }
    buffer.resize(len_transfer);
    for(uint i = 0; i < len_buffer_; ++i){
        buffer[i] = ptr_buffer_[i] * 126.999f;
    }
    for(uint i = len_buffer_; i < len_transfer; ++i){
        buffer[i] = 0;
    }

    tx_usb(len_transfer);

    return 0;
}
//-----------------------------------------------------------------------------------------
void hackrf_one_tx::tx_usb(uint32_t len_transfer_)
{
//    rx_tx_mutex->lock();
    // start stream
    uint16_t value;
    int rc = 0;
    value = HACKRF_TRANSCEIVER_MODE_TRANSMIT;
    rc |= libusb_control_transfer(usb_device,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                     LIBUSB_RECIPIENT_DEVICE,
                                     HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE,
                                     value,
                                     0,
                                     NULL,
                                     0,
                                     1000);
    if(rc != LIBUSB_SUCCESS) {
        fprintf(stderr, "Failed to start TX stream (%d)\n", rc);

        return;

    }
    uint8_t *cur_buffer = buffer.data();
    int tail = len_transfer_;
    do{
        int bytes_transferred = 0;
        rc = libusb_bulk_transfer(usb_device, usb_ep_out, cur_buffer, TX_USB_BUFFER, &bytes_transferred, 1000);
        if (LIBUSB_SUCCESS != rc || ((size_t)bytes_transferred != TX_USB_BUFFER)) {
            fprintf(stderr, "TX Transfer failed (%d)\n", rc);
        }
//        fprintf(stderr, "TX bytes_transferred (%d)  tail %d\n", bytes_transferred , tail);
        cur_buffer += TX_USB_BUFFER;
        tail -= TX_USB_BUFFER;
    }while(tail > 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // stop stream
    value = HACKRF_TRANSCEIVER_MODE_RECEIVE;
    rc |= libusb_control_transfer(usb_device,
                                 LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                 LIBUSB_RECIPIENT_DEVICE,
                                 HACKRF_VENDOR_REQUEST_SET_TRANSCEIVER_MODE,
                                 value,
                                 0,
                                 NULL,
                                 0,
                                 1000);
    if(rc != LIBUSB_SUCCESS) {
        fprintf(stderr, "Failed to start RX stream (%d)\n", rc);
    }
//rx_tx_mutex->unlock();
}
//------------------------------------------------------------------------------------------------
void hackrf_one_tx::stop()
{
    is_started = false;
}
//------------------------------------------------------------------------------------------------
