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
    void start(QString name_);
    void stop();
    void advanced_settings_dialog();
    int set_rx_frequency(int channel_);
    void set_rx_hardwaregain(double rx_hardwaregain_);
    int set_tx_frequency(int channel_);

    void test_test();

signals:
    void device_found(QString name_);
    void remove_device(QString name_);
    void error_open_device(QString name_);

    void plot_preamble_correlaion(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_correlation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_synchronize(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_constelation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_fft_correlation(int len_plot_, std::complex<float>* plot_buffer_);

    void mac_protocol_data_units(mpdu_analysis_t *mpdu_);

    void plot_test(int len_, std::complex<float>* data_);

    void device_status();

private:
    bool is_started = false;
    int channel;
    rx_thread_data_t *rx_thread_data;
    enum {
        PLUTO,
        NONE
    }type_dev;
    QTimer *timer;
    void search_devices();

    pluto_sdr *pluto = nullptr;
    QString pluto_name;
    bool pluto_is_open = false;
    bool pluto_is_start = false;

    double min_rssi;

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
