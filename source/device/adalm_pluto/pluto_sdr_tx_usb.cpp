/*
 *  Copyright 2025 Oleg Malyutin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "pluto_sdr_tx_usb.h"
#include "pluto_sdr.h"

//-----------------------------------------------------------------------------------------
pluto_sdr_tx::pluto_sdr_tx()
{
    usb_tx = new tx_usb_plutosdr;
}
//-----------------------------------------------------------------------------------------
pluto_sdr_tx::~pluto_sdr_tx()
{
    fprintf(stderr, "pluto_sdr_tx::~pluto_sdr_tx() start\n");
    delete usb_tx;
    fprintf(stderr, "pluto_sdr_tx::~pluto_sdr_tx() stop\n");
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::start(libusb_device_handle* usb_sdr_dev_,
                         uint8_t usb_sdr_interface_num_, uint8_t usb_sdr_ep_,
                         int len_buffer_)
{
    stop();

    usb_tx->start(usb_sdr_dev_, usb_sdr_interface_num_, usb_sdr_ep_, 1, len_buffer_);

    is_started = true;

}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::stop()
{
    if(is_started){
        is_started = false;
        usb_tx->stop();
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr_tx::shutdown()
{
}
//-----------------------------------------------------------------------------------------
bool pluto_sdr_tx::tx_data(uint &len_buffer_, const float *ptr_buffer_)
{
    usb_tx->tx_data(len_buffer_, ptr_buffer_);

    return true;
}
//-----------------------------------------------------------------------------------------




