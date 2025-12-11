#ifndef DEVICE_H
#define DEVICE_H
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
#include <QObject>
#include <QTimer>
#include <QUdpSocket>

#include <stdint.h>

#include "device_type.h"
#include "ieee802_15_4/phy_layer/phy_layer.h"
#include "callback_device.h"

class device : public QObject,
               public mpdu_callback,
               public device_callback

{
    Q_OBJECT
public:
    explicit device(QObject *parent = nullptr);
    ~device();

    phy_layer *phy;

public slots:
    void start();
    void stop();
    void open_device(QString name_);
    void close_device();
    void device_start();
    void device_stop();
    void advanced_settings_dialog();
    int set_rx_frequency(int channel_);
    void set_rx_hardwaregain(double rx_hardwaregain_);
    int set_tx_frequency(int channel_);

    void test_test();

signals:
    void device_status(QString status_);
    void device_found(QString name_);
    void remove_device(QString name_);
    void device_open();
    void set_max_rx_gain(int value);

    void plot_preamble_correlaion(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_correlation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_synchronize(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_constelation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_fft_correlation(int len_plot_, std::complex<float>* plot_buffer_);

    void mac_protocol_data_units(mpdu_analysis_t *mpdu_);

    void plot_test(int len_, std::complex<float>* data_);

    void device_error_status();

private:
    bool is_started = false;
    int channel;
    rx_thread_data_t *rx_thread_data;

    QTimer *timer;
    void scan_usb_device();

    std::string sdr_name;
    sdr_device *sdr = nullptr;
    bool sdr_is_open = false;
    bool sdr_is_start = false;

    std::thread *phy_thread;
    uint32_t channel_mask;

    phy_layer_callback *callback_phy;
    void connect_phy(phy_layer_callback *cb_){callback_phy = cb_;};
    void set_channel(int channel_, bool &status_);

    void error_callback(enum_device_status status_);
    void preamble_correlation_callback(int len_, std::complex<float> *b_);
    void sfd_callback(int len1_, std::complex<float> *b1_,
                      int len2_, std::complex<float> *b2_);
    void constelation_callback(int len_, std::complex<float> *b_);

    QUdpSocket *tx_socket;
    void rx_mpdu(mpdu_analysis_t *mpdu_);

    std::complex<float> *data;
    void plot_test_callback(int len_, float *b_);

};

#endif // DEVICE_H
