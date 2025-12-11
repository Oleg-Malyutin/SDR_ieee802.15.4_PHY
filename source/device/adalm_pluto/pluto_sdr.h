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
#ifndef PLUTO_SDR_H
#define PLUTO_SDR_H

#include <QDialog>
#include <QSettings>
#include <QMessageBox>

#include <iio.h>

#include "upload_sdrusbgadget.h"
#include "sdrusbgadget.h"
#include "pluto_sdr_rx.h"
#include "pluto_sdr_tx.h"

#define err(ans) { pluto_sdr_assert((ans), __FILE__, __LINE__); }
inline void pluto_sdr_assert(int error, const char *file, int line, bool abort=true)
{
    if (error < 0){
        char err_char[1024];
        iio_strerror(error, err_char, sizeof(err_char));
        std::string  err_str(err_char);
        fprintf(stderr,"device assert: %s %s %d\n", err_str.c_str(), file, line);
//        if (abort) exit(error);
    }
}

namespace Ui {
class pluto_sdr;
}

class pluto_sdr : public QDialog , public upload_gadget_callback, public sdr_device
{
    Q_OBJECT

public:
    explicit pluto_sdr(QWidget *parent = nullptr);
    ~pluto_sdr();

    enum enum_uri{
        IP,
        USB
    };
//    usb_plutosdr *usb_direct;
    pluto_sdr_rx* dev_rx = nullptr;
    pluto_sdr_tx* dev_tx = nullptr;
    void* get_dev_tx(){return dev_tx;};
    bool open_device(std::string &name_, std::string &err_);
    void close_device();
    bool check_connect();

signals:

public slots:
    void advanced_settings_dialog();
    void start(rx_thread_data_t *rx_thread_data_);
    void stop();
    void set_rx_rf_bandwidth(long long int bandwidht_hz_);
    void set_rx_sampling_frequency(long long int sampling_frequency_hz_);
    void get_rx_max_hardwaregain(double &hardwaregain_db_);
    void set_rx_hardwaregain(double hardwaregain_db_);
    void set_rx_frequency(long long int frequency_hz_);
    void set_tx_rf_bandwidth(long long int bandwidht_hz_);
    void set_tx_sampling_frequency(long long int sampling_frequency_hz_);
    void get_tx_max_hardwaregain(double &hardwaregain_db_);
    void set_tx_hardwaregain(double hardwaregain_db_);
    void set_tx_frequency(long long int frequency_hz_);

private slots:
    void on_comboBox_rx_rf_port_currentIndexChanged(const QString &arg1);
    void on_comboBox_rx_gain_control_mode_currentTextChanged(const QString &arg1);
    void on_checkBox_rx_bb_dc_offset_tracking_en_clicked(bool checked);
    void on_checkBox_rx_quadrature_tracking_en_clicked(bool checked);
    void on_checkBox_rx_rf_dc_offset_tracking_en_clicked(bool checked);
    void on_comboBox_tx_rf_port_currentTextChanged(const QString &arg1);

private:
    Ui::pluto_sdr *ui;

    static sdr_device_new<pluto_sdr> add_sdr_device;

    void show_upload(bool set_show_=true);

    enum io_dev{
        RX,
        TX
    };
    struct iio_context* context = nullptr;
    struct iio_device* ad9361_phy = nullptr;
    struct iio_device* cf_ad9361_lpc = nullptr;
    struct iio_device* cf_ad9361_dds_core_lpc = nullptr;
    struct iio_channel* rx_lo = nullptr;
    struct iio_channel* tx_lo = nullptr;
    struct iio_channel* rx_channel = nullptr;
    struct iio_channel* tx_channel = nullptr;

    void check_range(iio_channel *ch_, char *attr_, int value_);

    enum key_map_config{
        rx_bb_dc_offset_tracking_en,
        rx_gain_control_mode,
        rx_gain_control_mode_available,
        rx_quadrature_tracking_en,
        rx_rf_dc_offset_tracking_en,
        rx_rf_port_select,
        tx_rf_port_select
    };
    QSettings *config;
    const QString faile_name = "../config/pluto_settings.cfg";
    QMap<key_map_config, QVariant> map_config;
    int set_advanced_settings(QMap<key_map_config, QVariant> map_config_);
    void save_config();
    void read_config();

    sdrusbgadget *usb_direct;
    rx_thread_data_t *rx_thread_data;
    std::thread *thread_rx = nullptr;
    std::thread *thread_tx = nullptr;

};

#endif // PLUTO_SDR_H
