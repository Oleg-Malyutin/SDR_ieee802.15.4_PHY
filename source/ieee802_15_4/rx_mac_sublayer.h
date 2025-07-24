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
#ifndef RX_MAC_SUBLAYER_H
#define RX_MAC_SUBLAYER_H

#include "ieee802_15_4.h"
#include "mac_sublayer.h"
#include "utils/udp.h"

class rx_mpdu_sublayer_callback
{
public:
    virtual void rx_mpdu_callback(mpdu mpdu_)=0;
};

class rx_mac_sublayer
{
public:
    rx_mac_sublayer();
    ~rx_mac_sublayer();

    void start(mac_sublayer_data_t *data_);
    void connect_callback(rx_mpdu_sublayer_callback *cb_){callback = cb_;};

private:
    rx_mpdu_sublayer_callback *callback;
    void work(mac_sublayer_data_t *data_);
    void parser_data(std::vector<uint8_t> *mpdu_, int &channel_);
    frame_control_field parser_frame_control(uint16_t &frame_control_, uint8_t &sequence_number_);
    addressing_fields parse_addressing(frame_control_field &fcf_, uint8_t *data_);
    UdpSocket *udp_socket;
    unsigned char rx_zep_buffer[ZEP_BUFFER_SIZE];

    void send(mpdu &mpdu_, int &channel_);

};

#endif // RX_MAC_SUBLAYER_H
