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
#include "device.h"

//-----------------------------------------------------------------------------------------
device::device(QObject *parent) : QObject(parent)
{
    pluto = new pluto_sdr;
    dev_ieee802_15_4 = new ieee802_15_4;
    dev_ieee802_15_4->connect_callback(this);
    dev_ieee802_15_4->mac_layer->connect_callback(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &device::search_devices);
    timer->start(1000);
    rx_thread_data = new rx_thread_data_t;
}
//-----------------------------------------------------------------------------------------
device::~device()
{
    qDebug() << "device::~device() start";
    disconnect(timer, &QTimer::timeout, this, &device::search_devices);
    if(timer->isActive()){
        timer->stop();
    }
    stop();
    delete timer;
    delete dev_ieee802_15_4;
    delete pluto;
    delete rx_thread_data;
    qDebug() << "device::~device() stop";
}
//-----------------------------------------------------------------------------------------
void device::search_devices()
{
    if(!pluto_is_open) {
        if(pluto->open_device(pluto_sdr::USB, pluto_name)/* || pluto->get_device(pluto_sdr::IP)*/){
            pluto_is_open = true;
            pluto->set_rx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
            pluto->set_rx_sampling_frequency(ieee802_15_4_info::samplerate);
            pluto->set_rx_frequency(ieee802_15_4_info::rf_frequency);
            pluto->set_rx_hardwaregain(26);

            emit device_found(pluto_name);

        }
    }
    else if(!pluto_is_start){
        if(!pluto->check_connect()){
            pluto->close_device();
            pluto_is_open = false;

            emit remove_device(pluto_name);

        }
    }
}
//-----------------------------------------------------------------------------------------
void device::start(QString name_)
{
    if(name_ == pluto_name){
        // TODO__
        timer->stop();
        //__
        pluto_is_start = true;
        type_dev = PLUTO;
        pluto->start(rx_thread_data);
        // TODO : protocol start;
        ieee802_15_4_thread = new std::thread(&ieee802_15_4::start, dev_ieee802_15_4, rx_thread_data);
        ieee802_15_4_thread->detach();
    }
}
//-----------------------------------------------------------------------------------------
void device::stop()
{
    if(dev_ieee802_15_4->is_started){
        rx_thread_data->stop = true;
        rx_thread_data->cond_value.notify_one();
        while(dev_ieee802_15_4->is_started){
            QThread::msleep(1);
        }
        delete ieee802_15_4_thread;
    }
}
//-----------------------------------------------------------------------------------------
void device::plot_callback(int len1_, std::complex<float> *b1_, int len2_, std::complex<float> *b2_,
                         int len3_, std::complex<float> *b3_, int len4_, std::complex<float> *b4_)
{
    emit plot_preamble_correlaion(len1_, b1_);
    emit plot_sfd_correlation(len2_, b2_);
    emit plot_sfd_synchronize(len3_, b3_);
    emit plot_constelation(len4_, b4_);
}
//-----------------------------------------------------------------------------------------
void device::mpdu_callback(mpdu mpdu_)
{
    QString text_capture = "Frame version: ";
    QStringList list;
    list.append("");
    switch (mpdu_.fcf.frame_version) {
    case 0:
        text_capture += "IEEE Std 802.15.4-2003.";
        break;
    case 1:
        text_capture += "IEEE Std 802.15.4-2006.";
        break;
    case 2:
        text_capture += "IEEE Std 802.15.4.";
        break;
    case 3:
        text_capture += "Reserved.";
        break;
    }
    QString frame_type;
    switch (mpdu_.fcf.type){
    case Frame_type_beacon:
        frame_type = "Frame type: beacon.";
        break;
    case Frame_type_data:
        frame_type = "Frame type: data";
        break;
    case Frame_type_acknowledgment:
        frame_type = "Frame type: acknowledgment.";
        break;
    case Frame_type_MAC_command:
        frame_type = "Frame type: MAC_command.";
        break;
    case Frame_type_reserved:
        frame_type = "Frame type: reserved.";
        break;
    }
    text_capture += " " + frame_type;
    list.append(text_capture);

    QString address;
    if(mpdu_.af.dest_address == 0){
        address = "not present";
    }
    else{
        address = "0x" + QString::number(mpdu_.af.dest_address, 16);
    }
    text_capture = "Destination PAN 0x" + QString::number(mpdu_.af.dest_pan_id, 16) +
                   ",  Destination address " + address + ". ";

    if(mpdu_.af.source_pan_id == 0){
        address = "not present";
    }
    else{
        address = "0x" + QString::number(mpdu_.af.source_pan_id, 16);
    }
    text_capture += "Source PAN " + address;
    if(mpdu_.af.source_address == 0){
        address = "not present";
    }
    else{
        address = "0x" + QString::number(mpdu_.af.source_address, 16);
    }
    text_capture += ",  Source address " + address;
    list.append(text_capture);
    QVector<QString> data;
    data.push_back(QString());

    int idx_char = 0;
    int slice = 0;
    QString text("  ");
    while(idx_char < mpdu_.len_data){
        uint16_t d = mpdu_.data[idx_char];
        if(++slice > 32){
            slice = 0;
            data.last() += text;
            text.clear();
            data.push_back(QString());
        }
        if(idx_char % 8 == 0){
            data.last().append("  ");
        }
        if(mpdu_.data[idx_char] < 0x10){
            data.last().append("0");
        }
        data.last().append(QString::number(d, 16) + " ");
        if(d > 0x1f && d < 0x7f){
            text.append(d);
        }
        else{
            text.append(".");
        }

        ++idx_char;
    }
    if(slice > 0 && slice < 32){
        data.last().append("  ");
        int add_space = (31 - slice) / 8;
        while(add_space){
            data.last().append("  ");
            --add_space;
        }
        while(++slice < 32){
            data.last().append("   ");
        }
        data.last() += text;
    }
    for(auto &it : data){
        list.append(it);
    }


    emit get_frame_capture(list);
}
//-----------------------------------------------------------------------------------------
void device::advanced_settings_dialog()
{
    switch (type_dev) {
    case PLUTO:
        pluto->advanced_settings_dialog();
        break;
    case NONE:
        break;
    }
}
//-----------------------------------------------------------------------------------------
void device::set_rx_frequency(long long int rx_frequency_)
{
    switch (type_dev) {
    case PLUTO:
        pluto->set_rx_frequency(rx_frequency_);
        break;
    case NONE:
        break;
    }
}
//-----------------------------------------------------------------------------------------
void device::set_rx_hardwaregain(double rx_hardwaregain_)
{
    switch (type_dev) {
    case PLUTO:
        pluto->set_rx_hardwaregain(rx_hardwaregain_);
        break;
    case NONE:
        break;
    }
}
//-----------------------------------------------------------------------------------------


