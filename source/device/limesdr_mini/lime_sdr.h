#ifndef LIME_SDR_H
#define LIME_SDR_H

#include <QDialog>
#include <QSettings>
#include <QMessageBox>

//#include <lime/LimeSuite.h>
#include "driver/LimeSuite.h"
#include "lime_sdr_rx.h"
#include "lime_sdr_tx.h"

namespace Ui {
class lime_sdr;
}

class lime_sdr : public QDialog, public sdr_device
{
    Q_OBJECT

public:
    explicit lime_sdr(QWidget *parent = nullptr);
    ~lime_sdr();

    lime_sdr_rx* dev_rx = nullptr;
    lime_sdr_tx* dev_tx = nullptr;
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
    void set_rx_hardwaregain(double hardwaregain_db_);
    void set_rx_frequency(long long int frequency_hz_);
    void get_rx_rssi(double &rssi_db_);
    void set_tx_rf_bandwidth(long long int bandwidht_hz_);
    void set_tx_sampling_frequency(long long int sampling_frequency_hz_);
    void set_tx_hardwaregain(double hardwaregain_db_);
    void set_tx_frequency(long long int frequency_hz_);

private slots:
    void on_comboBox_rx_rf_port_currentIndexChanged(const QString &arg1);
    void on_comboBox_rx_gain_control_mode_currentTextChanged(const QString &arg1);
    void on_comboBox_tx_rf_port_currentTextChanged(const QString &arg1);

private:
    Ui::lime_sdr *ui;

    static sdr_device_new<lime_sdr> add_sdr_device;

    lms_info_str_t* list_info = nullptr;
    lms_device_t *device = nullptr;
    int64_t agc;
    uint rf_gain;
    int32_t gain_db;
    int32_t lna_gain_db;
    int32_t tia_gain_db;
    int32_t pga_gain_db;
    float_type rf_frequency;
    int64_t rf_bandwidth_hz;
    double rx_sample_rate_hz;
    double tx_sample_rate_hz;
    lms_stream_t rx_stream;
    lms_stream_t tx_stream;

    enum key_map_config{
        rx_bb_dc_offset_tracking_en,
        rx_gain_control_mode,
        rx_gain_control_mode_available,
        rx_quadrature_tracking_en,
        rx_rf_dc_offset_tracking_en,
        rx_rf_port_select,
        tx_rf_port_select
    };
    QSettings* config;
    const QString faile_name = "../config/lime_settings.cfg";
    QMap<key_map_config, QVariant> map_config;
    int set_advanced_settings(QMap<key_map_config, QVariant> map_config_);
    void save_config();
    void read_config();

    rx_thread_data_t *rx_thread_data;
    std::thread *thread_rx = nullptr;
    std::thread *thread_tx = nullptr;
};

#endif // LIME_SDR_H
