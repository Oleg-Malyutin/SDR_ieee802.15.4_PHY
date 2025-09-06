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

#include <QDateTime>

#include "utils/zep.h"

//-----------------------------------------------------------------------------------------
device::device(QObject *parent) : QObject(parent)
{
    type_dev = NONE;

    pluto = new pluto_sdr;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &device::search_devices);
    timer->start(1000);

    rx_thread_data = new rx_thread_data_t;

    phy = new phy_layer;
    connect_phy(phy);
    phy->connect_dev(this);
    phy->mac->connect_mpdu(this);
    phy->demodulator->connect_device(this);
    phy->modulator->connect_device(this);

    tx_socket = new QUdpSocket;

    data = new std::complex<float>[LEN_MAX_SAMPLES * 2];
}
//-----------------------------------------------------------------------------------------
device::~device()
{
    qDebug() << "device::~device() start";

    disconnect(timer, &QTimer::timeout, this, &device::search_devices);
    stop();
    delete timer;
    delete pluto;
    delete rx_thread_data;
    delete phy;

    delete tx_socket;

    delete[] data;

    qDebug() << "device::~device() stop";
}
//-----------------------------------------------------------------------------------------
void device::search_devices()
{
    if(!pluto_is_open) {
        if(pluto->open_device(pluto_sdr::USB, pluto_name)/* || pluto->get_device(pluto_sdr::IP)*/){     
            pluto->set_rx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
            pluto->set_rx_sampling_frequency(ieee802_15_4_info::samplerate);
            pluto->set_rx_frequency(ieee802_15_4_info::rf_frequency);
            pluto->set_rx_hardwaregain(69);
            pluto->set_tx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
            pluto->set_tx_sampling_frequency(ieee802_15_4_info::samplerate);
            pluto->set_tx_frequency(ieee802_15_4_info::rf_frequency);
            pluto->set_tx_hardwaregain(PLUTO_TX_GAIN_MAX);

            emit device_found(pluto_name);

            pluto_is_open = true;
        }
    }
    else if(pluto_is_open || !pluto_is_start){
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
    stop();
    if(name_ == pluto_name){
        timer->stop();
        type_dev = PLUTO;
        phy->modulator->connect_tx(pluto->dev_tx);
        pluto->start(rx_thread_data);
        min_rssi = -126.0;
        pluto_is_start = true;
    }
    else{

        return;

    }
    // TODO : protocol start;
    phy_thread = new std::thread(&phy_layer::start, phy, rx_thread_data, min_rssi);
    phy_thread->detach();
}
//-----------------------------------------------------------------------------------------
void device::stop()
{
    while (timer->isActive()){
        timer->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    switch (type_dev) {
    case PLUTO:
        if(pluto_is_start){
            pluto_is_start = false;
            pluto->stop();
        }
        if(pluto_is_open){
            pluto_is_open = false;
            if(!pluto->check_connect()){

                emit remove_device(pluto_name);

            }
            pluto->close_device();
        }
        break;
    case NONE:
        break;
    }
    type_dev = NONE;

    if(phy->is_started){
        callback_phy->callback_stop_phy_layer();
        while(phy->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete phy_thread;
    }

    timer->start(1000);
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
int device::set_rx_frequency(int channel_)
{
    channel = channel_;
    rx_thread_data->channel = channel_;
    uint64_t rx_frequency = ieee802_15_4_info::fq_channel_mhz[channel] * 1e6;
    int error = 0;
    switch (type_dev) {
    case PLUTO:
        error = pluto->set_rx_frequency(rx_frequency);
        error = pluto->set_tx_frequency(rx_frequency);
        break;
    case NONE:
        break;
    }

    return error;
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
int device::set_tx_frequency(int channel_)
{
    uint64_t tx_frequency = ieee802_15_4_info::fq_channel_mhz[channel_] * 1e6;
    int error = 0;
    switch (type_dev) {
    case PLUTO:
        error = pluto->set_tx_frequency(tx_frequency);
        break;
    case NONE:
        break;
    }

    return error;
}
//-----------------------------------------------------------------------------------------
void device::set_channel(int channel_, bool &status_)
{
    status_ = true;
    if(set_rx_frequency(channel_) != 0){
        status_ = false;
    }
    if(set_tx_frequency(channel_) != 0){
        status_ = false;
    }
}
//-----------------------------------------------------------------------------------------
void device::error_callback(enum_device_status status_)
{

    qDebug() << "error_callback" << status_;

    emit device_status();
}
//-----------------------------------------------------------------------------------------
void device::preamble_correlation_callback(int len_, std::complex<float> *b_)
{
    emit plot_preamble_correlaion(len_, b_);
}
//-----------------------------------------------------------------------------------------
void device::sfd_callback(int len1_, std::complex<float> *b1_,
                          int len2_, std::complex<float> *b2_)
{
    emit plot_sfd_correlation(len1_, b1_);
    emit plot_sfd_synchronize(len2_, b2_);
}
//-----------------------------------------------------------------------------------------
void device::constelation_callback(int len_, std::complex<float> *b_)
{
    emit plot_constelation(len_, b_);
}
//-----------------------------------------------------------------------------------------
void device::rx_mpdu(mpdu_analysis_t *mpdu_)
{
    emit mac_protocol_data_units(mpdu_);

    int64_t length;
    if(mpdu_->fcf.type == Frame_type_acknowledgment){
        zep_header_ack *z_header = (zep_header_ack *) rx_zep_buffer;
        strncpy(z_header->header.preamble, ZEP_PREAMBLE, 2);
        z_header->header.version = 2;
        z_header->type = ZEP_V2_TYPE_ACK;
        z_header->sequence++;
        size_t len_header= sizeof(zep_header_ack);
        // unsigned char *d = (unsigned char *)mpdu_->data;
        memcpy(&rx_zep_buffer[len_header], mpdu_->msdu, mpdu_->len_data);
        length = len_header + mpdu_->len_data;
    }
    else{
        zep_header_data *z_header = (zep_header_data *) rx_zep_buffer;
        strncpy(z_header->header.preamble, ZEP_PREAMBLE, 2);
        z_header->header.version = 2;
        z_header->type = ZEP_V2_TYPE_DATA;
        z_header->channel_id = channel;
        z_header->device_id = htobe32(0x00);
        z_header->lqi_mode = 1; // mode = 0 -> lqi, mode = 1 -> fcs
        z_header->lqi = 0;      // lqi;
        // struct timeval tv;
        // gettimeofday(&tv, NULL);
        // z_header->timestamp_s = htobe32(tv.tv_sec + EPOCH);
        // z_header->timestamp_ns = htobe32(tv.tv_usec * 1000);
        long unsigned tv = QDateTime::currentSecsSinceEpoch();
        z_header->timestamp_s = htobe32(tv + EPOCH);
        tv = QDateTime::currentMSecsSinceEpoch();
        z_header->timestamp_ns = htobe32(tv);
        z_header->sequence++;
        z_header->length = mpdu_->len_data;
        size_t len_header= sizeof(zep_header_data);

        // unsigned char *d = (unsigned char *)mpdu_->data;
        memcpy(&rx_zep_buffer[len_header], mpdu_->msdu, mpdu_->len_data);
        length = len_header + mpdu_->len_data;
    }

    tx_socket->writeDatagram((char*)rx_zep_buffer, length, QHostAddress::Any, ZEP_PORT);

}
//-----------------------------------------------------------------------------------------
void device::plot_test_callback(int len_, float *b_)
{
    int len = len_ / 2;
    for(int i = 0; i < len; ++i){
        data[i].real(b_[2 * i]);
        data[i].imag(b_[2 * i + 1]);
    }

    emit plot_test(len, data);

}
//-----------------------------------------------------------------------------------------
void device::test_test()
{
//    callback_phy->tx_test();
}
//-----------------------------------------------------------------------------------------

