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



//-----------------------------------------------------------------------------------------
device::device(QObject *parent) : QObject(parent)
{
    pluto = new pluto_sdr;
    lime = new lime_sdr;
    rx_dev_ieee802_15_4 = new rx_ieee802_15_4;
    rx_dev_ieee802_15_4->connect_callback(this);
    rx_dev_ieee802_15_4->mac_sublayer->connect_callback(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &device::search_devices);
    timer->start(1000);
    rx_thread_data = new rx_thread_data_t;
    tx_dev_ieee802_15_4 = new tx_ieee802_15_4;
    tx_dev_ieee802_15_4->connect_callback(this);// Only test;
    tx_dev_ieee802_15_4->mac_sublayer->connect_callback(this);
    tx_thread_data = new tx_thread_data_t;
    tx_thread_send_data = new tx_thread_send_data_t;
}
//-----------------------------------------------------------------------------------------
device::~device()
{
    disconnect(timer, &QTimer::timeout, this, &device::search_devices);
    stop();
    delete timer;
    delete rx_dev_ieee802_15_4;
    delete tx_dev_ieee802_15_4;
    delete pluto;
    delete rx_thread_data;
    delete tx_thread_data;
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
            pluto->set_tx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
            pluto->set_tx_sampling_frequency(ieee802_15_4_info::samplerate);
            pluto->set_tx_frequency(ieee802_15_4_info::rf_frequency);
            pluto->set_tx_hardwaregain(PLUTO_TX_GAIN_MAX);

            emit device_found(pluto_name);

        }
    }
    else if(pluto_is_open || !pluto_is_start){
        if(!pluto->check_connect()){
            pluto->close_device();
            pluto_is_open = false;

            emit remove_device(pluto_name);

        }
    }
    if(!lime_is_open) {
        if(lime->open_device(lime_name)){
            lime_is_open = true;
            lime->set_rx_sampling_frequency(ieee802_15_4_info::samplerate);
            lime->set_rx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
            lime->set_rx_frequency(ieee802_15_4_info::rf_frequency);
            lime->set_rx_hardwaregain(69);
            lime->set_tx_rf_bandwidth(ieee802_15_4_info::rf_bandwidth);
            lime->set_tx_sampling_frequency(ieee802_15_4_info::samplerate);
            lime->set_tx_frequency(ieee802_15_4_info::rf_frequency);
            lime->set_tx_hardwaregain(PLUTO_TX_GAIN_MAX);

            emit device_found(lime_name);

        }
    }
    else if(lime_is_open || !lime_is_start){
        if(!lime->check_connect()){
            lime->close_device();
            lime_is_open = false;

            emit remove_device(lime_name);

        }
    }
}
//-----------------------------------------------------------------------------------------
void device::start(QString name_)
{
    stop();
    if(name_ == pluto_name){
        // TODO__
        timer->stop();
        //__
        pluto_is_start = true;
        type_dev = PLUTO;
        pluto->start(rx_thread_data, tx_thread_data);  
    }
    else if(name_ == lime_name){
        // TODO__
        timer->stop();
        //__
        lime_is_start = true;
        type_dev = LIME;
        lime->start(rx_thread_data, tx_thread_data);
    }
    else{

        return;

    }
    // TODO : protocol start;
    rx_ieee802_15_4_thread = new std::thread(&rx_ieee802_15_4::start,
                                             rx_dev_ieee802_15_4, rx_thread_data);
    rx_ieee802_15_4_thread->detach();
    tx_ieee802_15_4_thread = new std::thread(&tx_ieee802_15_4::start,
                                             tx_dev_ieee802_15_4, tx_thread_data);
    tx_ieee802_15_4_thread->detach();
}
//-----------------------------------------------------------------------------------------
void device::stop()
{
    while (timer->isActive()){
        timer->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

//        qDebug() << "timer->isActive()" << timer->isActive();
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
    case LIME:
        if(lime_is_start){
            lime_is_start = false;
            lime->stop();
        }
        if(lime_is_open){
            lime_is_open = false;
            if(!lime->check_connect()){

                emit remove_device(lime_name);

            }
            lime->close_device();
        }
        break;
    default:
        break;
    }
    type_dev = NONE;
    if(rx_dev_ieee802_15_4->is_started){
        rx_thread_data->stop = true;
        rx_thread_data->cond_value.notify_one();
        while(rx_dev_ieee802_15_4->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete rx_ieee802_15_4_thread;
    }
    if(tx_dev_ieee802_15_4->is_started){
        tx_thread_send_data->stop = true;
        tx_thread_send_data->cond_value.notify_one();
        while(tx_dev_ieee802_15_4->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete tx_ieee802_15_4_thread;
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
    case LIME:
        lime->advanced_settings_dialog();
        break;
    case NONE:
        break;
    }
}
//-----------------------------------------------------------------------------------------
void device::set_rx_frequency(int channel_)
{
    channel = channel_;
    rx_thread_data->channel = channel_;
    uint64_t rx_frequency = ieee802_15_4_info::fq_channel_mhz[channel] * 1e6;

    switch (type_dev) {
    case PLUTO:
        pluto->set_rx_frequency(rx_frequency);
        break;
    case LIME:
        lime->set_rx_frequency(rx_frequency);
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
    case LIME:
        lime->set_rx_hardwaregain(rx_hardwaregain_);
        break;
    case NONE:
        break;
    }
}
//-----------------------------------------------------------------------------------------
void device::set_tx_frequency(int channel_)
{
    uint64_t tx_frequency = ieee802_15_4_info::fq_channel_mhz[channel_] * 1e6;

    switch (type_dev) {
    case PLUTO:
        pluto->set_tx_frequency(tx_frequency);
        break;
    case LIME:
        lime->set_tx_frequency(tx_frequency);
        break;
    case NONE:
        break;
    }
}
//-----------------------------------------------------------------------------------------
void device::error_callback(enum_device_status status_)
{

    qDebug() << "error_callback" << status_;

    emit device_status();
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
void device::rx_mpdu_callback(mpdu mpdu_)
{
    emit mac_protocol_data_units(mpdu_);
}
//-----------------------------------------------------------------------------------------
void device::tx_mpdu_callback(mpdu mpdu_)
{

}
//-----------------------------------------------------------------------------------------
void device::test_shr()
{
    tx_thread_send_data->mutex.lock();
    tx_thread_send_data->mdpu.clear();
    tx_thread_send_data->ready = true;
    tx_thread_send_data->mutex.unlock();
    tx_thread_send_data->cond_value.notify_one();
}
//-----------------------------------------------------------------------------------------
void device::test_test()
{
    tx_thread_send_data->mutex.lock();
    tx_thread_send_data->mdpu.clear();
    for(int i = 0; i < 32; ++i){
        tx_thread_send_data->mdpu.push_back(0xa);
    }
    tx_thread_send_data->ready = true;
    tx_thread_send_data->mutex.unlock();
    tx_thread_send_data->cond_value.notify_one();
}
//-----------------------------------------------------------------------------------------

