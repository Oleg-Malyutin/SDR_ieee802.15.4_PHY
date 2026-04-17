#ifndef USRP_H
#define USRP_H

#include <QDialog>

#include <uhd.h>
#include <uhd/config.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/convert.hpp>

#include "usrp_rx.h"
#include "usrp_tx.h"

namespace Ui {
class usrp;
}

class usrp : public QDialog, public sdr_device
{
    Q_OBJECT
public:
    explicit usrp(QWidget *parent = nullptr);
    ~usrp();

    usrp_rx* dev_rx = nullptr;
    usrp_tx* dev_tx = nullptr;
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
    Ui::usrp *ui;

    static sdr_device_new<usrp> add_sdr_device;

    uhd::usrp::multi_usrp::sptr device;
    const int channel = 0;
    std::string error(int _err);
    rx_thread_data_t *rx_thread_data;
    std::thread *thread_rx = nullptr;
    std::thread *thread_tx = nullptr;

};

#endif // USRP_H
