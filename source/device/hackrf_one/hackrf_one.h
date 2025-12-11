#ifndef HACKRF_ONE_H
#define HACKRF_ONE_H

#include <QDialog>

#include "hackrf_one_rx.h"
#include "hackrf_one_tx.h"

namespace Ui {
class hackrf_one;
}

class hackrf_one : public QDialog, public sdr_device
{
    Q_OBJECT

public:
    explicit hackrf_one(QWidget *parent = nullptr);
    ~hackrf_one();

    hackrf_one_rx* dev_rx = nullptr;
    hackrf_one_tx* dev_tx = nullptr;
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

private:
    Ui::hackrf_one *ui;

    static sdr_device_new<hackrf_one> add_sdr_device;

    std::string error (int err);
    hackrf_device *device;
    libusb_device_handle *usb_device;
    rx_thread_data_t *rx_thread_data;
    std::thread *thread_rx = nullptr;
    std::thread *thread_tx = nullptr;

    std::mutex *rx_tx_mutex;
};

#endif // HACKRF_ONE_H
