#include "lime_sdr.h"
#include "ui_lime_sdr.h"

#include <QDebug>

//-----------------------------------------------------------------------------------------
lime_sdr::lime_sdr(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lime_sdr)
{
    ui->setupUi(this);

    dev_rx = new lime_sdr_rx;
    dev_tx = new lime_sdr_tx;

    config = new QSettings(faile_name , QSettings::NativeFormat);
    ui->comboBox_rx_rf_port->addItem("No active");
    ui->comboBox_rx_rf_port->addItem("LNA_H");
    ui->comboBox_rx_rf_port->addItem("LNA_L");
    ui->comboBox_rx_rf_port->addItem("LNA_W");
    ui->comboBox_rx_rf_port->addItem("Automatic");
    ui->comboBox_rx_gain_control_mode->addItem("manual");
    ui->comboBox_tx_rf_port->addItem("TX1");
    ui->comboBox_tx_rf_port->addItem("TX2");
    read_config();
}
//-----------------------------------------------------------------------------------------
lime_sdr::~lime_sdr()
{
    qDebug() << "lime_sdr::~pluto_sdr() start";
    close_device();
    stop();
    delete dev_rx;
    delete dev_tx;
    delete config;
    delete ui;
    qDebug() << "lime_sdr::~pluto_sdr() stop";
}
//-----------------------------------------------------------------------------------------
bool lime_sdr::open_device(std::string &name_, std::string &err_)
{
    name_.clear();
    int n = LMS_GetDeviceList(nullptr);
    if(n <= 0){
        err_ = "LimeSDR open filed:  Device not found!";

        return false;

    }
    list_info = new lms_info_str_t[n];
    if(LMS_GetDeviceList(list_info) <= 0){
        err_ = "LimeSDR open failed:  Not get device list!";

        return false;

    }
    if (LMS_Open(&device, list_info[0], nullptr)){
        err_ = "LimeSDR open failed:  Do not open device!";
        close_device();

        return false;

    }

    LMS_Reset(device);

    if (LMS_EnableCache(device, true) != 0){
        fprintf(stderr, "LimeSDR open filed:  Do not enable cache! \n");
    }

    if (LMS_Init(device) != 0){
        fprintf(stderr, "LimeSDR open filed:  Do not init device!\n");
        close_device();

        return false;

    }
//    if (LMS_LoadConfig(device, "./lime_oqpsk.ini") != 0){
//        err_ = "LimeSDR open failed:  Do not load file!";
//        close_device();

//        return false;

//    }
    // enable rx channels
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0){
        fprintf(stderr, "LimeSDR open filed:  Do not enable channel RX!\n");
        stop();

        return false;

    }
    // enable tx channels
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0){
        fprintf(stderr, "LimeSDR open filed:  Do not enable channel TX!\n");
        stop();

        return false;

    }
//    if(LMS_Synchronize(device, true) != 0){
//        fprintf(stderr, "LimeSDR open filed:  Do not synchronize device! \n");
//    }
    //configure LPF rx
    if (LMS_SetLPFBW(device, LMS_CH_RX, 0, 2000000.0) != 0){
        fprintf(stderr, "lime_sdr_tx::start:LMS_SetLPFBW: %s\n", LMS_GetLastErrorMessage());

//        return false;

    }
    //configure LPF tx
    if (LMS_SetLPFBW(device, LMS_CH_TX, 0, 5000000.0) != 0){
        fprintf(stderr, "lime_sdr_tx::start:LMS_SetLPFBW: %s\n", LMS_GetLastErrorMessage());

//        return false;

    }
    // perform automatic calibration rx
    if (LMS_Calibrate(device, LMS_CH_RX, 0, 2000000.0, 0) < 0){
        fprintf(stderr, "lime_sdr_rx::start:LMS_Calibrate:  %s\n", LMS_GetLastErrorMessage());
//        stop();

//        return false;

    }
    //perform automatic calibration tx
    if (LMS_Calibrate(device, LMS_CH_TX, 0, 5000000.0, 0) < 0){
        fprintf(stderr, "lime_sdr_tx::start:LMS_SetLPFBW: %s\n", LMS_GetLastErrorMessage());
//        stop();

//        return false;

    }
    // avoid having a spike at start
//    LMS_SetNormalizedGain(device, LMS_CH_TX, 0, 0.0f);
//    LMS_SetNormalizedGain(device, LMS_CH_RX, 0, 0.0f);

    for(int i = 0; i < 42; ++i){
        name_.append(list_info[0][i], 1);
    }
    ui->label_info->setText(list_info[0]);
    delete [] list_info;

    read_config();
    set_advanced_settings(map_config);

    err_ = "";

    return true;
}
//-----------------------------------------------------------------------------------------
void lime_sdr::close_device()
{
    if (device != nullptr) {
        LMS_Close(device);
    }
}
//-----------------------------------------------------------------------------------------
bool lime_sdr::check_connect()
{
    return LMS_GetDeviceList(nullptr) <= 0 ? false : true;
}
//-----------------------------------------------------------------------------------------
void lime_sdr::start(rx_thread_data_t *rx_thread_data_)
{
    rx_thread_data = rx_thread_data_;
    rx_thread_data->reset();
    rx_thread_data->len_buffer = LMS_FIFO_SIZE;
    thread_rx = new std::thread(&lime_sdr_rx::start, dev_rx, device, rx_thread_data);
    thread_rx->detach();

    thread_tx = new std::thread(&lime_sdr_tx::start, dev_tx, device, tx_sample_rate_hz);
    thread_tx->detach();
}
//-----------------------------------------------------------------------------------------
void lime_sdr::stop()
{
    if(dev_rx->is_started){
        rx_thread_data->stop = true;
        while(dev_rx->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
        delete thread_rx;
    }
    if(dev_tx->is_started){
        dev_tx->stop();
        while(dev_rx->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
        delete thread_tx;
    }
}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_rx_rf_bandwidth(long long int bandwidht_hz_)
{
    float bw = bandwidht_hz_;
    //configure LPF
//    LMS_SetGFIRLPF(device, LMS_CH_RX, 0, true, bw);
}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_rx_sampling_frequency(long long int sampling_frequency_hz_)
{
    rx_sample_rate_hz = sampling_frequency_hz_;
    //set sample rate, preferred oversampling in RF 0(use device default oversampling value)
    LMS_SetSampleRate(device, rx_sample_rate_hz, 0);

}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_rx_hardwaregain(double hardwaregain_db_)
{
    //set rx gain range [0, 61](dB), where 73 represents the maximum gain
    float_type gain = hardwaregain_db_ / 73.0;
    LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain);
//    LMS_SetGaindB(device, LMS_CH_RX, 0, (float_type)hardwaregain_db_);

//    uint16_t val = 12;// 1-31
//    LMS_WriteParam(device, LMS7_G_PGA_RBB, val);
//    val = 1;//1-3
//    LMS_WriteParam(device, LMS7_G_TIA_RFE, val);
}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_rx_frequency(long long int frequency_hz_)
{
    float_type fq = frequency_hz_;
    //set center frequency
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, fq) != 0){
        fprintf(stderr, "LimeSDR set_rx_frequency: %s\n", LMS_GetLastErrorMessage());
        LMS_SetLOFrequency(device, LMS_CH_RX, 0, fq);
        fprintf(stderr, "LimeSDR set_rx_frequency: %s\n", LMS_GetLastErrorMessage());
    }
}
//-----------------------------------------------------------------------------------------
void lime_sdr::get_rx_rssi(double &rssi_db_)
{
    uint16_t value;

}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_tx_rf_bandwidth(long long int bandwidht_hz_)
{
    float bw = bandwidht_hz_;
    LMS_SetGFIRLPF(device, LMS_CH_TX, 0, true, bw);
}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_tx_sampling_frequency(long long int sampling_frequency_hz_)
{
    tx_sample_rate_hz = sampling_frequency_hz_;
}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_tx_hardwaregain(double hardwaregain_db_)
{
    float gain = 1.0;//hardwaregain_db_;
    if(LMS_SetNormalizedGain(device, LMS_CH_TX, 0, gain)){
        fprintf(stderr, "LimeSDR set_tx_hardwaregain: %s\n", LMS_GetLastErrorMessage());
    }
}
//-----------------------------------------------------------------------------------------
void lime_sdr::set_tx_frequency(long long int frequency_hz_)
{
    float_type fq = frequency_hz_;
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, fq) != 0){
        fprintf(stderr, "LimeSDR set_tx_frequency: %s\n", LMS_GetLastErrorMessage());
    }
}
//-----------------------------------------------------------------------------------------
void lime_sdr::advanced_settings_dialog()
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
int lime_sdr::set_advanced_settings(QMap<key_map_config, QVariant> map_config_)
{
    int ret;
    size_t index = LMS_PATH_LNAW;
    //select antenna port
    if(map_config_[rx_rf_port_select].toString() == "No active") index = LMS_PATH_NONE;
    if(map_config_[rx_rf_port_select].toString() == "LNA_H") index = LMS_PATH_LNAH;
    if(map_config_[rx_rf_port_select].toString() == "LNA_L") index = LMS_PATH_LNAL;
    if(map_config_[rx_rf_port_select].toString() == "LNA_W") index = LMS_PATH_LNAW;
    if(map_config_[rx_rf_port_select].toString() == "Automatic") index = LMS_PATH_AUTO;
    ret = LMS_SetAntenna(device, LMS_CH_RX, 0, index);
    if(ret < 0) {

        return ret;

    }
    if(map_config_[tx_rf_port_select].toString() == "No active") index = LMS_PATH_NONE;
    if(map_config_[tx_rf_port_select].toString() == "TX1") index = LMS_PATH_TX1;
    if(map_config_[tx_rf_port_select].toString() == "TX2") index = LMS_PATH_TX2;
    if(map_config_[tx_rf_port_select].toString() == "Automatic") index = LMS_PATH_AUTO;
    ret = LMS_SetAntenna(device, LMS_CH_TX, 0, index);
    if(ret < 0) {

        return ret;

    }

    return 0;
}
//-----------------------------------------------------------------------------------------
void lime_sdr::save_config()
{
    config->beginGroup("LimeSDR");
    config->setValue("rx_rf_port_select", map_config[rx_rf_port_select]);
    config->setValue("rx_gain_control_mode", map_config[rx_gain_control_mode]);
    config->setValue("tx_rf_port_select", map_config[tx_rf_port_select]);
    config->endGroup();
}
//--------------------------------------------------------------------------------------------------
void lime_sdr::read_config()
{
    config->beginGroup("LimeSDR");
    map_config[rx_rf_port_select] = config->value("rx_rf_port_select", "LNA_H");
    map_config[rx_gain_control_mode] = config->value("rx_gain_control_mode", "manual");
    map_config[tx_rf_port_select] = config->value("tx_rf_port_select", "LMS_PATH_TX1");
    config->endGroup();

    ui->comboBox_rx_rf_port->setCurrentText(map_config[rx_rf_port_select].toString());
    ui->comboBox_rx_gain_control_mode->setCurrentText(map_config[rx_gain_control_mode].toString());
    ui->comboBox_tx_rf_port->setCurrentText(map_config[tx_rf_port_select].toString());
}
//--------------------------------------------------------------------------------------------------
void lime_sdr::on_comboBox_rx_rf_port_currentIndexChanged(const QString &arg1)
{
    map_config[rx_rf_port_select] = arg1;
}
//--------------------------------------------------------------------------------------------------
void lime_sdr::on_comboBox_rx_gain_control_mode_currentTextChanged(const QString &arg1)
{
    map_config[rx_gain_control_mode] = arg1;
}
//---------------------------------- ----------------------------------------------------------------
void lime_sdr::on_comboBox_tx_rf_port_currentTextChanged(const QString &arg1)
{
    map_config[tx_rf_port_select] = arg1;
}
//--------------------------------------------------------------------------------------------------
