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

#include "adalm_pluto/pluto_sdr.h"
#include "limesdr_mini/lime_sdr.h"
#include "ieee802_15_4/rx_ieee802_15_4.h"
#include "ieee802_15_4/tx_ieee802_15_4.h"

class device : public QObject,
               public ieee802_15_4_callback,
               public rx_mpdu_sublayer_callback,
               public tx_mpdu_sublayer_callback
{
    Q_OBJECT
public:
    explicit device(QObject *parent = nullptr);
    ~device();

public slots:
    void start(QString name_);
    void stop();
    void advanced_settings_dialog();
    void set_rx_frequency(int channel_);
    void set_rx_hardwaregain(double rx_hardwaregain_);
    void set_tx_frequency(int channel_);

    void test_shr();
    void test_test();

signals:
    void device_found(QString name_);
    void remove_device(QString name_);
    void error_open_device(QString name_);
    void mac_protocol_data_units(mpdu mpdu_);

    void plot_preamble_correlaion(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_correlation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_synchronize(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_constelation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_fft_correlation(int len_plot_, std::complex<float>* plot_buffer_);

    void plot_test(int len_, std::complex<float>* data_);

    void device_status();

private:
    bool is_started = false;
    int channel;
    rx_thread_data_t *rx_thread_data;
    tx_thread_data_t *tx_thread_data;
    tx_thread_send_data_t *tx_thread_send_data;
    enum {
        PLUTO,
        LIME,
        NONE
    }type_dev;
    QTimer *timer;
    void search_devices();

    pluto_sdr *pluto = nullptr;
    QString pluto_name;
    bool pluto_is_open = false;
    bool pluto_is_start = false;

    lime_sdr *lime = nullptr;
    QString lime_name;
    bool lime_is_open = false;
    bool lime_is_start = false;

    rx_ieee802_15_4 *rx_dev_ieee802_15_4;
    std::thread *rx_ieee802_15_4_thread;
    tx_ieee802_15_4 *tx_dev_ieee802_15_4;
    std::thread *tx_ieee802_15_4_thread;

    std::complex<float> data[2000];

    void error_callback(enum_device_status status_);
    void preamble_correlation_callback(int len_, std::complex<float> *b_);
    void sfd_callback(int len1_, std::complex<float> *b1_,
                      int len2_, std::complex<float> *b2_);
    void constelation_callback(int len_, std::complex<float> *b_);
    void plot_test_callback(int len_, float *b_);
    void rx_mpdu_callback(mpdu mpdu_);
    void tx_mpdu_callback(mpdu mpdu_);

};

#endif // DEVICE_H
