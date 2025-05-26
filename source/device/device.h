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

#include <stdint.h>

//#include "device/device_thread.h"
#include "adalm_pluto/pluto_sdr.h"
#include "ieee802_15_4/ieee802_15_4.h"

class device : public QObject, public ieee802_15_4_callback, public mpdu_sublayer_callback
{
    Q_OBJECT
public:
    explicit device(QObject *parent = nullptr);
    ~device();

public slots:
    void start(QString name_);
    void stop();
    void advanced_settings_dialog();
    void set_rx_frequency(long long int rx_frequency_);
    void set_rx_hardwaregain(double rx_hardwaregain_);

signals:
    void device_found(QString name_);
    void remove_device(QString name_);
    void error_open_device(QString name_);
    void get_frame_capture(QStringList list_);

    void plot_preamble_correlaion(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_correlation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_sfd_synchronize(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_constelation(int len_plot_, std::complex<float>* plot_buffer_);
    void plot_fft_correlation(int len_plot_, std::complex<float>* plot_buffer_);

private:
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
    void plot_callback(int len1_, std::complex<float> *b1_, int len2_, std::complex<float> *b2_,
                       int len3_, std::complex<float> *b3_, int len4_, std::complex<float> *b4_);
    void mpdu_callback(mpdu mpdu_);

    bool *ready;
    std::condition_variable *cond_value;
    std::mutex *mutex;
    int *len_buffer;
    int16_t *ptr_buffer;

    ieee802_15_4 *dev_ieee802_15_4;
    std::thread *ieee802_15_4_thread;


};

#endif // DEVICE_H
