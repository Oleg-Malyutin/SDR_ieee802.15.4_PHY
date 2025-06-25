#ifndef USB_POWER_CYCLE_H
#define USB_POWER_CYCLE_H

#include <stdio.h>
#include <libusb-1.0/libusb.h>

unsigned		realtek_hub_vid = 0x0bda;
unsigned		realtek_hub_pid = 0x5411;

int usb_power_cycle()
{
        int ret = -1;
        libusb_device_handle	*hub_devh;
        libusb_context		*context;

        ret = libusb_init(&context);
        if (ret != 0){
            fprintf(stderr,"[error]:libusb init fail.\n");
            return ret;
        }

        hub_devh = libusb_open_device_with_vid_pid(context, realtek_hub_vid, realtek_hub_pid);
        if (!hub_devh) {
            fprintf(stderr, "[error]:open device %04x:%04x fail.\n", realtek_hub_vid, realtek_hub_pid);
            return -1;
        }

        /*ep0 vendor command enable*/
        ret = libusb_control_transfer(hub_devh, 0x40, 0x02, 0x01, ((0x0B<<8)|(0xDA)), 0, 0, 100000);
        if (ret < 0) {
                fprintf(stderr, "[error]:ep0 vendor command enable fail.\n");
                return ret;
        }

        /*ep0 vendor command disable*/
        libusb_control_transfer(hub_devh, 0x40, 0x1, 0x08, 0, NULL, 0, 100);
        libusb_control_transfer(hub_devh, 0x40, 0x3, 0x08, 0, NULL, 0, 100);
        libusb_control_transfer(hub_devh, 0x40, 0x02, 0x00, ((0x0B<<8)|(0xDA)), 0, 0, 100000);

        return ret;
}

#endif // USB_POWER_CYCLE_H
