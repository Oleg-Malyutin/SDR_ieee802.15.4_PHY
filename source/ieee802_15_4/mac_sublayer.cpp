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
#include "mac_sublayer.h"

#include <QDebug>

//--------------------------------------------------------------------------------------------------
mac_sublayer::mac_sublayer()
{

}
//--------------------------------------------------------------------------------------------------
mac_sublayer::~mac_sublayer()
{

}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::start(mac_sublayer_data_t *data_)
{
    work(data_);
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::work(mac_sublayer_data_t *data_)
{
    mac_sublayer_data_t *data = data_;
    std::unique_lock<std::mutex> read_lock(data->mutex);
    data->reset();
    while (!data->stop){
        data->cond_value.wait(read_lock);
        if(data->ready){
            data->ready = false;
            parser_data(data->mpdu);
        }
    }
    data->is_started = false;
    //    stop();
    qDebug() << "mac_sublayer::work() finish";
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::parser_data(std::vector<uint8_t> *mpdu_)
{
    int idx_symbol = 0;
    const int len = mpdu_->size();
    int len_data = 0;
    if(len >= 10 && len <= MAX_PHY_PAYLOAD){
//        qDebug() << "mac_sublayer::parser_data                       " << len;
        uint16_t data[len];
        uint16_t byte;
        uint8_t symbol;//4 bits
        uint16_t crc = 0x0;
        for(int i = 0; i < len - 4; ++i){
            symbol = (*mpdu_)[i];
            // need two symbol for byte
            if(++idx_symbol < 2){
                byte = symbol;
            }
            else{
                idx_symbol = 0;
                byte += symbol << 0x4;
                data[len_data++] = byte;
                // revert bit to LSB
                uint8_t mask;
                uint8_t b = 0;
                for (int bit = 0; bit < 8; ++bit) {
                    mask = 1 << bit;
                    b |= ((byte & mask) >> bit) << (7 - bit);
                }
                // crc16
                uint16_t idx = ((crc >> 8) ^ b) & 0xff;
                crc = ((crc << 8) ^ CRC_CCITT_TABLE[idx]) & 0xffff;
            }
        }
        idx_symbol = 0;
        int idx_byte = 0;
        uint16_t fcs = 0x0;//frame check sequence (FCS)
        for(int i = len - 4; i < len; ++i){
            symbol = (*mpdu_)[i];
            // need two symbol for byte
            if(++idx_symbol < 2){
                byte = symbol;
            }
            else{
                idx_symbol = 0;
                byte += symbol << 0x4;
                data[len_data++] = byte;
                // revert bit to LSB
                uint8_t mask;
                uint8_t b = 0;
                for (int bit = 0; bit < 8; ++bit) {
                    mask = 1 << bit;
                    b |= ((byte & mask) >> bit) << (7 - bit);
                }
                // frame check sequence
                if(++idx_byte < 2){
                    fcs = b << 0x8;
                }
                else{
                    fcs += b;
                }

            }
        }

//        bool check_crc = crc == fcs;
//        qDebug() << "fcs" << QString::number(fcs, 16)
//                 << "crc" << QString::number(crc, 16)
//                 << check_crc;

        if(crc == fcs){
            uint16_t frame_control = data[0];
            frame_control += data[1] << 0x8;

            frame_control_field fcf = parser_frame_control(frame_control);
            fcf.sequence_number = data[2];
            addressing_fields af = parse_addressing(fcf, data);

            mpdu m;
//            for(int i = 0; i < len_data; ++i){
//                m.data[i] = data[i];
//            }
            m.fcf = fcf;
            m.af = af;
            m.data = data;
            m.len_data = len_data;
            callback->mpdu_callback(m);

//            qDebug() << "frame control" << QString::number(frame_control, 16);
//            qDebug() << "frame version" << fcf.frame_version;
//            qDebug() << "data sequence number" << QString::number(fcf.sequence_number);
//            qDebug() << "dest_pan_id" << QString::number(af.dest_pan_id, 16);
//            qDebug() << "source_pan_id" << QString::number(af.source_pan_id, 16);
//            qDebug() << "dest_address" << QString::number(af.dest_address, 16);
//            qDebug() << "source_address" << QString::number(af.source_address, 16);

        }

    }
    else{

//        qDebug() << "error:mac too short";

    }
}
//--------------------------------------------------------------------------------------------------
frame_control_field mac_sublayer::parser_frame_control(uint16_t &frame_control_)
{
    frame_control_field fcf;
    uint8_t type = frame_control_ & 0x4;
    if(type == 0){
        type = frame_control_ & 0x3;
    }
    fcf.type = static_cast<frame_type>(type);
    fcf.security_enabled = (frame_control_ & 0x8) >> 0x3;
    fcf.frame_pending = (frame_control_ & 0x10) >> 0x4 ;
    fcf.ask_request = (frame_control_ & 0x20) >> 0x5;
    fcf.intra_pan = (frame_control_ & 0x40) >> 0x6;
    fcf.dest_mode = static_cast<adressing_mode>((frame_control_ & 0xc00) >> 0xa);
    fcf.frame_version = (frame_control_ & 0x3000) >> 0xc;
    fcf.source_mode = static_cast<adressing_mode>((frame_control_ & 0xc000) >> 0xe);

    return fcf;

}
//--------------------------------------------------------------------------------------------------
addressing_fields mac_sublayer::parse_addressing(frame_control_field &fcf_, uint16_t *data_)
{
    addressing_fields af;
    uint8_t offset = 3; //frame_control_field 2(byte) + sequence_number 1(byte)
    uint16_t *p_d = data_ + offset;
    af.dest_pan_id = p_d[0] + (p_d[1] << 0x8);
    offset += 2;
    p_d = data_ + offset;
    bool  da = true;
    switch(fcf_.dest_mode){
    case Adressing_mode_not_present:
        da = false;
        af.dest_address = 0x0;
        break;
    case Adressing_mode_reserved:
        da = false;
        af.dest_address = 0x0;
        break;
    case Adressing_mode_16bit_short:
        af.dest_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data_ + offset;
        break;
    case Adressing_mode_64bit_extended:
        af.dest_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data_ + offset;
        uint64_t address = p_d[0] + (p_d[1] << 0x8);
        af.dest_address += (address << 0x10) + af.dest_address;
        offset += 2;
        p_d = data_ + offset;
        break;
    }

    bool sa = (fcf_.source_mode > Adressing_mode_reserved) && da;
    if(fcf_.intra_pan == 0 && sa){
        af.source_pan_id = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data_ + offset;
    }
    else{
        af.source_pan_id = 0x0;
    }
    switch(fcf_.source_mode){
    case Adressing_mode_not_present:
        af.source_address = 0x0;
        break;
    case Adressing_mode_reserved:
        af.source_address = 0x0;
        break;
    case Adressing_mode_16bit_short:
        af.source_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
//        p_d = data_ + offset;
        break;
    case Adressing_mode_64bit_extended:
        af.source_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data_ + offset;
        uint64_t address = p_d[0] + (p_d[1] << 0x8);
        af.source_address += (address << 0x10) + af.source_address;
        offset += 2;
//        p_d = data_ + offset;
        break;
    }

    af.offset = offset;

    return af;

}
//--------------------------------------------------------------------------------------------------
