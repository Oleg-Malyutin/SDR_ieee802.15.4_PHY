#include "sdrusbgadget.h"

#include <iostream>
#include <cstring>

//------------------------------------------------------------------------------------------------
sdrusbgadget::sdrusbgadget()
{

}
//------------------------------------------------------------------------------------------------
sdrusbgadget::~sdrusbgadget()
{
    close();
}
//------------------------------------------------------------------------------------------------
void sdrusbgadget::open(iio_context* context)
{
    close();
    // retrieve url and separate bus / device
    const char *uri = iio_context_get_attr_value(context, "uri");
    if(!uri) {
        fprintf(stderr, "failed to retrieve uri from iio");
        throw std::runtime_error("failed to retrieve uri from iio");
    }
    // retrieve bus and device number from uri
    unsigned short int bus_num, dev_addr;
    if(2 != std::sscanf(uri, "usb:%hu.%hu", &bus_num, &dev_addr)) {
        fprintf(stderr, "failed to extract usb bus and device address from uri");
        throw std::runtime_error("failed to extract usb bus and device address from uri");
    }

    int rc;
    // init libusb
    rc = libusb_init(&usb_ctx);
    if (rc < 0) {
        fprintf(stderr, "libusb init error (%d)", rc);
        throw std::runtime_error("libusb init error");
    }
    // retrieve device list
    struct libusb_device **devs;
    int dev_count = libusb_get_device_list(usb_ctx, &devs);
    if(dev_count < 0) {
        fprintf(stderr, "libusb get device list error (%d)", dev_count);
        throw std::runtime_error("libusb get device list error");
    }
    // iterate over devices
    for(int i = 0; i < dev_count; i++) {
        struct libusb_device *dev = devs[i];
        // check device bus and address
        if((libusb_get_bus_number(dev) == bus_num) && (libusb_get_device_address(dev) == dev_addr)) {
                // found device, open it
                int rc = libusb_open(dev, &this->usb_sdr_dev);
                if(rc < 0) {
                    // Failed to open device
                    fprintf(stderr, "libusb failed to open device (%d)\n", rc);
                    this->usb_sdr_dev = nullptr;
                }

                break;

        }
    }
    // free list, reducing device reference counts
    libusb_free_device_list(devs, 1);
    // check handle
    if(!this->usb_sdr_dev) {
        fprintf(stderr, "failed to open sdr_usb_gadget\n");
        throw std::runtime_error("failed to open sdr_usb_gadget");
    }
    // retrieve active config descriptor
    struct libusb_config_descriptor *config;
    rc = libusb_get_active_config_descriptor(libusb_get_device(this->usb_sdr_dev), &config);
    if(rc < 0) {
        fprintf(stderr, "failed to get usb device descriptor (%d)", rc);
        throw std::runtime_error("failed to get usb device descriptor");
    }

    // loop through interfaces and find one with the desired name
    int interface_num = -1;
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface *iface = &config->interface[i];
        for (int j = 0; j < iface->num_altsetting; j++) {
            const struct libusb_interface_descriptor *desc = &iface->altsetting[j];
            // get the interface name
            char name[128];
            rc = libusb_get_string_descriptor_ascii(this->usb_sdr_dev, desc->iInterface, (unsigned char*)name, sizeof(name));
            if (rc < 0) {
                fprintf(stderr, "failed to get usb device interface name (%d)", rc);
                throw std::runtime_error("failed to get usb device interface name");
            }

            if (0 == strcmp(name, "sdrgadget")) {
                // capture interface number
                interface_num = desc->bInterfaceNumber;
                // Capture endpoint addresses
                for (int k = 0; k < desc->bNumEndpoints; k++) {
                    const struct libusb_endpoint_descriptor *ep_desc = &desc->endpoint[k];
                    if(ep_desc->bEndpointAddress & 0x80) {
                        this->usb_sdr_ep_in = ep_desc->bEndpointAddress;
                    }
                    else {
                        this->usb_sdr_ep_out = ep_desc->bEndpointAddress;
                    }
                }
                // all done
                break;

            }
        }
    }

    // free the configuration descriptor
    libusb_free_config_descriptor(config);
    config = nullptr;
    if(interface_num < 0) {
        fprintf(stderr, "failed to find usb device interface");
        throw std::runtime_error("failed to find usb device interface");
    }
    // store interface number
    this->usb_sdr_interface_num = (uint8_t)interface_num;
    // claim the interface
    rc = libusb_claim_interface(this->usb_sdr_dev, this->usb_sdr_interface_num);
    if(rc < 0) {
        fprintf(stderr, "failed to claim usb device interface (%d)", rc);
        throw std::runtime_error("failed to claim usb device interface");
    }
}
//------------------------------------------------------------------------------------------------
void sdrusbgadget::close()
{
    if(usb_ctx != nullptr){
        libusb_release_interface(this->usb_sdr_dev, this->usb_sdr_interface_num);
        libusb_exit(usb_ctx);
        usb_ctx = nullptr;
    }
}
//------------------------------------------------------------------------------------------------
