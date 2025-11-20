#include <cstring>

#include <sched.h>
#include <pthread.h>

#include "tx_usb_plutosdr.h"

#define SDR_USB_GADGET_COMMAND_START (0x10)
#define SDR_USB_GADGET_COMMAND_STOP (0x11)
#define SDR_USB_GADGET_COMMAND_TARGET_TX (0x1)

typedef struct{
    uint32_t enabled_channels;
    uint32_t buffer_size;
} cmd_usb_start_request_t;

//------------------------------------------------------------------------------------------------
tx_usb_plutosdr::tx_usb_plutosdr()
{

}
//------------------------------------------------------------------------------------------------
tx_usb_plutosdr::~tx_usb_plutosdr()
{
	if (thread.joinable()) {
        stop();
	}
}
//------------------------------------------------------------------------------------------------
int tx_usb_plutosdr::start(libusb_device_handle *usb_sdr_dev_,
                           uint8_t usb_sdr_interface_num_, uint8_t usb_sdr_ep_out_,
                           uint32_t num_channels_, uint32_t buffer_size_samples_)
{
    if (!thread.joinable()) {

        enabled_channels = 0;
        for(unsigned int i = 0; i < num_channels_ * 2; i++) {
            // add bit to enabled channels
            enabled_channels |= (1 << i);
        }

        thread_stop = false;
        usb_sdr_dev = usb_sdr_dev_;
        usb_sdr_interface_num = usb_sdr_interface_num_;
        usb_sdr_ep_out =  usb_sdr_ep_out_;

        thread = std::thread(&tx_usb_plutosdr::thread_func, this, enabled_channels, buffer_size_samples_);
        // attempt to increase thread priority
//		int max_prio = sched_get_priority_max(SCHED_RR);
//		if (max_prio >= 0) {
//			sched_param sch;
//			sch.sched_priority = max_prio;
//			if (int rc = pthread_setschedparam(thread.native_handle(), SCHED_RR, &sch)) {
//				fprintf(stderr, "Failed to set TX thread priority (%d)\n", rc);
//			}
//		}
//        else {
//			fprintf(stderr, "Failed to query thread schedular priorities\n");
//		}
    }

    return 0;
}
//------------------------------------------------------------------------------------------------
int tx_usb_plutosdr::tx_data(uint &len_buffer_, const float *ptr_buffer_)
{
//    fprintf(stderr, "tx_usb_plutosdr::tx_data (%d)\n", len_buffer_);

    event_mutex.lock();

    if(buffer.size() != len_buffer_ * 2){
        buffer.resize(len_buffer_ * 2);
    }
    for(uint i = 0; i < len_buffer_; ++i){
        uint16_t data = ptr_buffer_[i] * 32767.999f;
        buffer[i * 2] = data & 0x00ff;
        buffer[i * 2 + 1] = data >> 8;
    }

    event_ready = true;
    event_mutex.unlock();
    event.notify_all();

    return 0;
}
//------------------------------------------------------------------------------------------------
void tx_usb_plutosdr::thread_func(uint32_t enabled_channels_, uint32_t buffer_size_samples_)
{
	cmd_usb_start_request_t cmd;
    // start stream
    cmd.enabled_channels = enabled_channels_;
    cmd.buffer_size = buffer_size_samples_;
    int rc = libusb_control_transfer(usb_sdr_dev,
									 LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
									 SDR_USB_GADGET_COMMAND_START,
									 SDR_USB_GADGET_COMMAND_TARGET_TX,
                                     usb_sdr_interface_num,
									 (unsigned char*)&cmd,
									 sizeof(cmd),
									 1000);
	if (rc < 0) {
        fprintf(stderr, "Failed to start TX stream (%d)\n", rc);

		return;

	}

    std::unique_lock<std::mutex> read_lock(event_mutex);
    while (!thread_stop.load()) {
        event.wait(read_lock);
        if(event_ready){
            event_ready = false;
            int bytes_transferred = 0;
            int rc = libusb_bulk_transfer(usb_sdr_dev, usb_sdr_ep_out, buffer.data(), buffer.size(), &bytes_transferred, 1000);
            if (LIBUSB_SUCCESS != rc || ((size_t)bytes_transferred != buffer.size())) {
                fprintf(stderr, "TX Transfer failed (%d)\n", rc);
            }
            fprintf(stderr, "TX bytes_transferred (%d)\n", bytes_transferred);
        }
    }

    // stop stream
    rc = libusb_control_transfer(usb_sdr_dev,
								 LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
								 SDR_USB_GADGET_COMMAND_STOP,
								 SDR_USB_GADGET_COMMAND_TARGET_TX,
                                 usb_sdr_interface_num,
								 nullptr,
								 0,
								 1000);
	if (rc < 0) {
        fprintf(stderr, "Failed to stop TX stream (%d)", rc);
		return;
	}
}
//------------------------------------------------------------------------------------------------
void tx_usb_plutosdr::stop()
{
    if (thread.joinable()){
        thread_stop = true;
        event_ready = false;
        event_mutex.unlock();
        event.notify_all();
        thread.join();
    }
}
//------------------------------------------------------------------------------------------------
