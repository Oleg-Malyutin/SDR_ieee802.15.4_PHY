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
#include "pluto_sdr.h"
#include "ui_pluto_sdr.h"

#include <QDebug>

//--------------------------------------------------------------------------------------------------
pluto_sdr::pluto_sdr(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::pluto_sdr)
{
    ui->setupUi(this);

    dev_rx = new pluto_sdr_rx;
    dev_tx = new pluto_sdr_tx;

    config = new QSettings(faile_name , QSettings::NativeFormat);
    ui->comboBox_rx_rf_port->addItem("A_BALANCED");
    ui->comboBox_rx_rf_port->addItem("A_N");
    ui->comboBox_rx_rf_port->addItem("A_P");
    ui->comboBox_rx_rf_port->addItem("B_BALANCED");
    ui->comboBox_rx_rf_port->addItem("B_N");
    ui->comboBox_rx_rf_port->addItem("B_P");
    ui->comboBox_rx_rf_port->addItem("C_BALANCED");
    ui->comboBox_rx_rf_port->addItem("C_N");
    ui->comboBox_rx_rf_port->addItem("C_P");
    ui->comboBox_rx_gain_control_mode->addItem("manual");
    ui->comboBox_rx_gain_control_mode->addItem("fast_attack");
    ui->comboBox_rx_gain_control_mode->addItem("slow_attack");
    ui->comboBox_rx_gain_control_mode->addItem("hybrid");
    ui->comboBox_rx_rf_port->addItem("TX_MONITOR1");
    ui->comboBox_rx_rf_port->addItem("TX_MONITOR2");
    ui->comboBox_rx_rf_port->addItem("TX_MONITOR3");
    ui->comboBox_tx_rf_port->addItem("A");
    ui->comboBox_tx_rf_port->addItem("B");
    read_config();
}
//--------------------------------------------------------------------------------------------------
pluto_sdr::~pluto_sdr()
{
    qDebug() << "pluto_sdr::~pluto_sdr() start";
    stop();
    delete dev_rx;
    delete dev_tx;
    delete config;
    delete ui;
    qDebug() << "pluto_sdr::~pluto_sdr() stop";
}
//--------------------------------------------------------------------------------------------------
//bool pluto_sdr::get_device(enum_uri idx)
//{
//    std::string uri;
//    switch(idx){
//    case USB:
//        uri = "usb:";
//        break;
//    case IP:
//        uri = "ip:192.168.3.1";
//    }
//    context = iio_create_context_from_uri(uri.c_str());

//    if(context == nullptr) return false;

//    return true;
//}
//--------------------------------------------------------------------------------------------------
bool pluto_sdr::open_device(enum_uri idx, QString &name_)
{
    std::string uri;
    switch(idx){
    case USB:
        uri = "usb:";
        break;
    case IP:
        uri = "ip:192.168.3.1";
    }
    context = iio_create_context_from_uri(uri.c_str());

    if(context == nullptr) return false;

    std::string info;
    for(uint i = 0; i < iio_context_get_attrs_count(context); ++i){
        const char* name;
        const char* value;
        err(iio_context_get_attr(context, i, &name, &value));
        info.append(name);
        info.append(" ");
        info.append(value);
        info.append("\n");
        if(i == 0) name_ = value;
    }

    ad9361_phy = iio_context_find_device(context, "ad9361-phy");
    rx_lo = iio_device_find_channel(ad9361_phy, "altvoltage0", true);
    tx_lo = iio_device_find_channel(ad9361_phy, "altvoltage1", true);
    rx_channel = iio_device_find_channel(ad9361_phy, "voltage0", false);
    tx_channel = iio_device_find_channel(ad9361_phy, "voltage0", true);
    cf_ad9361_lpc = iio_context_find_device(context, "cf-ad9361-lpc");
    cf_ad9361_dds_core_lpc = iio_context_find_device(context, "cf-ad9361-dds-core-lpc");

    iio_context_set_timeout(context, 0);

    ui->label_info->setText(QString::fromStdString(info));
    read_config();
    set_advanced_settings(map_config);

    return true;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::close_device()
{
    if(context != nullptr){
        iio_context_destroy(context);
        context = nullptr;
    }
}
//--------------------------------------------------------------------------------------------------
bool pluto_sdr::check_connect()
{
    if(iio_context_set_timeout(context, 0) < 0 ) return false;
    return true;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::start(rx_thread_data_t *rx_thread_data_, tx_thread_data_t *tx_thread_data_)
{
    rx_thread_data = rx_thread_data_;
    rx_thread_data->reset();
    rx_thread_data->len_buffer = RX_PLUTO_LEN_BUFFER;
    thread_rx = new std::thread(&pluto_sdr_rx::start, dev_rx, cf_ad9361_lpc, rx_thread_data);
    thread_rx->detach();

    tx_thread_data = tx_thread_data_;
    tx_thread_data->reset();
    tx_thread_data->len_buffer = TX_PLUTO_LEN_BUFFER;
    thread_tx = new std::thread(&pluto_sdr_tx::start, dev_tx, cf_ad9361_dds_core_lpc, tx_thread_data);
    thread_tx->detach();
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::stop()
{
    if(dev_rx->is_started){
        rx_thread_data->stop = true;
        while(dev_rx->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
        delete thread_rx;
    }
    if(dev_tx->is_started){
        tx_thread_data->stop = true;
        tx_thread_data->cond_value.notify_one();
        while(dev_tx->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
        delete thread_tx;
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_rx_rf_bandwidth(long long int bandwidht_hz_)
{
    long long int bw = bandwidht_hz_;
    int ret = iio_channel_attr_write_longlong(rx_channel, "rf_bandwidth", bw);
    if(ret < 0){
        char attr[] = "rf_bandwidth_available";
        check_range(rx_channel, attr, bandwidht_hz_);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_rx_sampling_frequency(long long int sampling_frequency_hz_)
{
    long long int sf = sampling_frequency_hz_;
    int ret = iio_channel_attr_write_longlong(rx_channel, "sampling_frequency", sf);
    if(ret < 0){
        char attr[] = "sampling_frequency_available";
        check_range(rx_channel, attr, sampling_frequency_hz_);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_rx_hardwaregain(double hardwaregain_db_)
{
    int ret = iio_channel_attr_write_double(rx_channel, "hardwaregain", hardwaregain_db_);
//    if(ret < 0){
//        char attr[] = "hardwaregain_available";
//        int hg = hardwaregain_db_;
//        check_range(rx_channel, attr, hg);
//    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_rx_frequency(long long int frequency_hz_)
{
    long long int fq = frequency_hz_;
    int ret = iio_channel_attr_write_longlong(rx_lo, "frequency", fq);
    if(ret < 0){
        char attr[] = "frequency_available";
        check_range(rx_lo, attr, frequency_hz_);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::get_rx_rssi(double &rssi_db_)
{
    iio_channel_attr_read_double(rx_channel, "rssi", &rssi_db_);
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_tx_rf_bandwidth(long long int bandwidht_hz_)
{
    long long int bw = bandwidht_hz_;
    int ret = iio_channel_attr_write_longlong(tx_channel, "rf_bandwidth", bw);
    if(ret < 0){
        char attr[] = "rf_bandwidth_available";
        check_range(tx_channel, attr, bandwidht_hz_);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_tx_sampling_frequency(long long int sampling_frequency_hz_)
{
    long long int sf = sampling_frequency_hz_;
    int ret = iio_channel_attr_write_longlong(tx_channel, "sampling_frequency", sf);
    if(ret < 0){
        char attr[] = "sampling_frequency_available";
        check_range(tx_channel, attr, sampling_frequency_hz_);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_tx_hardwaregain(double hardwaregain_db_)
{
    int ret = iio_channel_attr_write_double(tx_channel, "hardwaregain", hardwaregain_db_);
    if(ret < 0){
        char attr[] = "hardwaregain_available";
        int hg = hardwaregain_db_;
        check_range(tx_channel, attr, hg);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::set_tx_frequency(long long int frequency_hz_)
{
    long long int fq = frequency_hz_;
    int ret = iio_channel_attr_write_longlong(tx_lo, "frequency", fq);
    if(ret < 0){
        char attr[] = "frequency_available";
        check_range(tx_lo, attr, frequency_hz_);
    }
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::check_range(iio_channel *ch_, char *attr_, int value_)
{
    char buf[256];
    int size_out = iio_channel_attr_read(ch_, attr_, buf, 256);
    QString str_value(buf);
    str_value.remove(0, 1);
    str_value.resize(size_out - 2);
    QRegularExpression rx("[ ]");
    QStringList list = str_value.split(rx, Qt::SkipEmptyParts);
    int min = list[0].toInt();
    int max = list[2].toInt();
    QMessageBox msg;
    if(min > value_){
        msg.setText("The value of " + QString::fromUtf8(attr_) + " is less than the minimum: " +
                    QString::number(min));
    }
    else if(max < value_){
        msg.setText("The value of " + QString::fromUtf8(attr_) + " is greater than the minimum: " +
                    QString::number(max));
    }
    else{
        msg.setText("Unknown error");
    }
    msg.exec();
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::advanced_settings_dialog()
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    switch (exec()) {
    case QDialog::Accepted:
        if(set_advanced_settings(map_config) < 0){
            QMessageBox msg_box;
            msg_box.setText("Don't set advanced parameters");
            msg_box.exec();
            read_config();
        }
        else{
            save_config();
        }
        break;
    case QDialog::Rejected:
        read_config();
        break;
    default:
        break;
    }
}
//-----------------------------------------------------------------------------------------
int pluto_sdr::set_advanced_settings(QMap<key_map_config, QVariant> map_config_)
{
    int ret;
    ret = iio_channel_attr_write(rx_channel, "rf_port_select",
                               map_config_[rx_rf_port_select].toString().toStdString().c_str());
    if(ret < 0) return ret;
    ret = iio_channel_attr_write(rx_channel, "gain_control_mode",
                               map_config_[rx_gain_control_mode].toString().toStdString().c_str());
    if(ret < 0) return ret;
    ret = iio_channel_attr_write_bool(rx_channel, "bb_dc_offset_tracking_en",
                                    map_config_[rx_bb_dc_offset_tracking_en].toBool());
    if(ret < 0) return ret;
    ret = iio_channel_attr_write_bool(rx_channel, "quadrature_tracking_en",
                                    map_config_[rx_quadrature_tracking_en].toBool());
    if(ret < 0) return ret;
    ret = iio_channel_attr_write_bool(rx_channel, "rf_dc_offset_tracking_en",
                                    map_config_[rx_rf_dc_offset_tracking_en].toBool());
    if(ret < 0) return ret;
    ret = iio_channel_attr_write(tx_channel, "rf_port_select",
                               map_config_[tx_rf_port_select].toString().toStdString().c_str());
    if(ret < 0) return ret;

    return 0;
}
//-----------------------------------------------------------------------------------------
void pluto_sdr::save_config()
{
    config->beginGroup("PlutoSDR");
    config->setValue("rx_rf_port_select", map_config[rx_rf_port_select]);
    config->setValue("rx_gain_control_mode", map_config[rx_gain_control_mode]);
    config->setValue("rx_bb_dc_offset_tracking_en", map_config[rx_bb_dc_offset_tracking_en]);
    config->setValue("rx_quadrature_tracking_en", map_config[rx_quadrature_tracking_en]);
    config->setValue("rx_rf_dc_offset_tracking_en", map_config[rx_rf_dc_offset_tracking_en]);
    config->setValue("tx_rf_port_select", map_config[tx_rf_port_select]);
    config->endGroup();
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::read_config()
{
    config->beginGroup("PlutoSDR");
    map_config[rx_rf_port_select] = config->value("rx_rf_port_select", "A_BALANCED");
    map_config[rx_gain_control_mode] = config->value("rx_gain_control_mode", "fast_attack");
    map_config[rx_bb_dc_offset_tracking_en] = config->value("rx_bb_dc_offset_tracking_en", true);
    map_config[rx_quadrature_tracking_en] = config->value("rx_quadrature_tracking_en", true);
    map_config[rx_rf_dc_offset_tracking_en] = config->value("rx_rf_dc_offset_tracking_en", true);

    map_config[tx_rf_port_select] = config->value("tx_rf_port_select", "A");
    config->endGroup();

    ui->comboBox_rx_rf_port->setCurrentText(map_config[rx_rf_port_select].toString());
    ui->comboBox_rx_gain_control_mode->setCurrentText(map_config[rx_gain_control_mode].toString());
    ui->checkBox_rx_bb_dc_offset_tracking_en->setChecked(map_config[rx_bb_dc_offset_tracking_en].toBool());
    ui->checkBox_rx_quadrature_tracking_en->setChecked(map_config[rx_quadrature_tracking_en].toBool());
    ui->checkBox_rx_rf_dc_offset_tracking_en->setChecked(map_config[rx_rf_dc_offset_tracking_en].toBool());

    ui->comboBox_tx_rf_port->setCurrentText(map_config[tx_rf_port_select].toString());
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::on_comboBox_rx_rf_port_currentIndexChanged(const QString &arg1)
{
    map_config[rx_rf_port_select] = arg1;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::on_comboBox_rx_gain_control_mode_currentTextChanged(const QString &arg1)
{
    map_config[rx_gain_control_mode] = arg1;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::on_checkBox_rx_bb_dc_offset_tracking_en_clicked(bool checked)
{
    map_config[rx_bb_dc_offset_tracking_en] = checked;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::on_checkBox_rx_quadrature_tracking_en_clicked(bool checked)
{
    map_config[rx_quadrature_tracking_en] = checked;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::on_checkBox_rx_rf_dc_offset_tracking_en_clicked(bool checked)
{
    map_config[rx_rf_dc_offset_tracking_en] = checked;
}
//--------------------------------------------------------------------------------------------------
void pluto_sdr::on_comboBox_tx_rf_port_currentTextChanged(const QString &arg1)
{
    map_config[tx_rf_port_select] = arg1;
}
//--------------------------------------------------------------------------------------------------
