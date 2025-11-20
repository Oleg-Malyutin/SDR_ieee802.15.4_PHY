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

#include "adalm_pluto/pluto_sdr.h"
#include "limesdr_mini/lime_sdr.h"
#include "hackrf_one/hackrf_one.h"

std::string pluto_name = "PlutoSDR";
#ifdef USE_PLUTOSDR
sdr_device_new<pluto_sdr> pluto_sdr::add_sdr_device(pluto_name);
#endif

std::string lime_name = "LimeSDR";
#ifdef USE_LIMESDR
sdr_device_new<lime_sdr> lime_sdr::add_sdr_device(lime_name);
#endif

std::string hackrf_name = "HackRF";
#ifdef USE_HACKRF
sdr_device_new<hackrf_one> hackrf_one::add_sdr_device(hackrf_name);
#endif

//-----------------------------------------------------------------------------------------
device::device(QObject *parent) : QObject(parent)
{

}
//-----------------------------------------------------------------------------------------
device::~device()
{
    qDebug() << "device::~device() start";
    stop();
    qDebug() << "device::~device() stop";
}
//-----------------------------------------------------------------------------------------
void device::start()
{
    timer = new QTimer();
    connect(timer, &QTimer::timeout, this, &device::scan_usb_device);
    timer->start(1000);

    rx_thread_data = new rx_thread_data_t;

    phy = new phy_layer;
    connect_phy(phy);
    phy->connect_dev(this);
    phy->mac->connect_mpdu(this);
    phy->demodulator->connect_device(this);
    phy->modulator->connect_device(this);

    tx_socket = new QUdpSocket();

    data = new std::complex<float>[LEN_MAX_SAMPLES * 2];
}
//-----------------------------------------------------------------------------------------
void device::stop()
{
    if(is_started){
        is_started = false;
        disconnect(timer, &QTimer::timeout, this, &device::scan_usb_device);
        close_device();
        delete timer;
        delete sdr;
        delete rx_thread_data;
        delete phy;
        delete tx_socket;
        delete[] data;
    }
}
//-----------------------------------------------------------------------------------------
void device::scan_usb_device()
{
    libusb_device **devs;
    int r = libusb_init(NULL);
    if(r < 0) {
        fprintf(stderr, "scan_usb_device: failed libusb_init");

        return;

    }
    ssize_t cnt = libusb_get_device_list(NULL, &devs);
    if(cnt < 0) {
        libusb_exit(NULL);
        fprintf(stderr, "scan_usb_device: failed libusb_get_device_list");

        return;

    }
    libusb_device *dev;
    int i = 0;
    while((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if(r < 0) {
            fprintf(stderr, "scan_usb_device: failed libusb_get_device_descriptor");

            continue;

        }
        if(desc.idVendor == 0x0456 &&  desc.idProduct == 0xb673){
#ifdef USE_PLUTOSDR
             emit device_found(QString::fromStdString(pluto_name));
#endif
        }
        if(desc.idVendor == 0x0403 &&  desc.idProduct == 0x601f){
#ifdef USE_LIMESDR
            emit device_found(QString::fromStdString(lime_name));
#endif
        }
        if(desc.idVendor == 0x1d50 &&  desc.idProduct == 0x6089){
#ifdef USE_HACKRF
            emit device_found(QString::fromStdString(hackrf_name));
#endif
        }

    }
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
}
//-----------------------------------------------------------------------------------------
void device::open_device(QString name_)
{
    close_device();
    while (timer->isActive()){
        timer->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::string name = name_.toStdString();
    std::string err;
    if(name == pluto_name){
        sdr_name = pluto_name;
    }
    else if(name == lime_name){
        sdr_name = lime_name;
    }
    else if(name == hackrf_name){
        sdr_name = hackrf_name;
    }

    sdr = sdr_factory::instantiate(sdr_name);

    emit device_status(QString::fromStdString(sdr_name) + ": Wait for to load...");


    if(sdr->open_device(sdr_name, err)/* || pluto->get_device(pluto_sdr::IP)*/){
        sdr->set_rx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
        sdr->set_rx_sampling_frequency(ieee802_15_4_info::samplerate);
        sdr->set_rx_frequency(ieee802_15_4_info::rf_frequency);
        sdr->set_rx_hardwaregain(69);
        sdr->set_tx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
        sdr->set_tx_sampling_frequency(ieee802_15_4_info::samplerate);
        sdr->set_tx_frequency(ieee802_15_4_info::rf_frequency);
        sdr->set_tx_hardwaregain(PLUTO_TX_GAIN_MAX);
        sdr_is_open = true;

        emit device_open();

    }
    else{
        sdr->close_device();
        timer->start(1000);

        emit device_status(QString::fromStdString(name + err));

    }
}
//-----------------------------------------------------------------------------------------
void device::close_device()
{
    device_stop();
    if(sdr_is_open){
        sdr_is_open = false;
        if(!sdr->check_connect()){

            emit remove_device(QString::fromStdString(sdr_name));

        }
        sdr->close_device();qDebug() << "close_device";
    }
}
//-----------------------------------------------------------------------------------------
void device::device_start()
{
    device_stop();

    while (timer->isActive()){
        timer->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    phy->modulator->connect_tx((rf_tx_callback*)sdr->get_dev_tx());
    sdr->start(rx_thread_data);
    double min_rssi;
    sdr->get_min_rssi(min_rssi);
    sdr_is_start = true;
    // TODO : protocol start;
    phy_thread = new std::thread(&phy_layer::start, phy, rx_thread_data, min_rssi);
    phy_thread->detach();
}
//-----------------------------------------------------------------------------------------
void device::device_stop()
{
    if(sdr_is_start){
        sdr_is_start = false;
        sdr->stop();
    }
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
    sdr->advanced_settings_dialog();
}
//-----------------------------------------------------------------------------------------
int device::set_rx_frequency(int channel_)
{
    if(sdr == nullptr){

        return 0;

    }
    channel = channel_;
    rx_thread_data->channel = channel_;
    uint64_t rx_frequency = ieee802_15_4_info::fq_channel_mhz[channel] * 1e6;
    int error = 0;
    sdr->set_rx_frequency(rx_frequency);
    sdr->set_tx_frequency(rx_frequency);

    return error;
}
//-----------------------------------------------------------------------------------------
void device::set_rx_hardwaregain(double rx_hardwaregain_)
{
    if(sdr == nullptr){

        return;

    }
    sdr->set_rx_hardwaregain(rx_hardwaregain_);
}
//-----------------------------------------------------------------------------------------
int device::set_tx_frequency(int channel_)
{
    if(sdr == nullptr){

        return 0;

    }
    uint64_t tx_frequency = ieee802_15_4_info::fq_channel_mhz[channel_] * 1e6;
    int error = 0;
    sdr->set_tx_frequency(tx_frequency);

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

    emit device_error_status();
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

