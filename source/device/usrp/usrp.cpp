#include "usrp.h"
#include "ui_usrp.h"

#include <QThread>
#include <QDebug>

//-----------------------------------------------------------------------------------------
usrp::usrp(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::usrp)
{
    ui->setupUi(this);

    dev_rx = new usrp_rx;
    dev_tx = new usrp_tx;
}
//-----------------------------------------------------------------------------------------
usrp::~usrp()
{
    delete dev_rx;
    delete dev_tx;
}
//-----------------------------------------------------------------------------------------
std::string usrp::error(int _err)
{
    switch (_err) {
    case UHD_ERROR_NONE:
        return "Success";
    case UHD_ERROR_INVALID_DEVICE:
        return "Invalid device";
    case UHD_ERROR_INDEX:
        return "Error index";
    case UHD_ERROR_KEY:
        return "Error key";
    case UHD_ERROR_NOT_IMPLEMENTED:
        return "Not implemented";
    case UHD_ERROR_USB:
        return "Error USB";
    case UHD_ERROR_IO:
        return "Error IO";
    case UHD_ERROR_OS:
        return "Error OS";
    case UHD_ERROR_ASSERTION:
        return "Error assertion";
    case UHD_ERROR_LOOKUP:
        return "Error lookup";
    case UHD_ERROR_TYPE:
        return "Error type";
    case UHD_ERROR_VALUE:
        return "Error value";
    case UHD_ERROR_RUNTIME:
        return "Error runtime";
    case UHD_ERROR_ENVIRONMENT:
        return "Error environment";
    case UHD_ERROR_SYSTEM:
        return "Error system";
    case UHD_ERROR_EXCEPT:
        return "Error exept";
    case UHD_ERROR_BOOSTEXCEPT:
        return "Error boostexept";
    case UHD_ERROR_STDEXCEPT:
        return "Error stdexept";
    case UHD_ERROR_UNKNOWN:
        return "Error unknow";
    default:
        return "Unknown error";
    }
}
//-------------------------------------------------------------------------------------------
bool usrp::open_device(std::string &name_, std::string &err_)
{

    uhd::device_addr_t hint;
    std::string hw_ver;
    for (const uhd::device_addr_t &dev : uhd::device::find(hint)){
        name_ = dev.cast< std::string >("name", "");
        hw_ver = dev.cast< std::string >("type", "");
    }
    if(hw_ver == "") {
        err_ = " open failed: unknonw hw version or busy";

        return false;

    }

    uhd::device_addr_t device_addr{"uhd,recv_frame_size=512,num_recv_frames=2"};
    device = uhd::usrp::multi_usrp::make(device_addr);

    device->set_rx_agc(false, channel);
    device->set_rx_dc_offset(true, channel);
    device->set_rx_iq_balance(true, channel);

    return true;
}
//-----------------------------------------------------------------------------------------
void usrp::close_device()
{

}
//-----------------------------------------------------------------------------------------
bool usrp::check_connect()
{
    return true;
}
//-----------------------------------------------------------------------------------------
void usrp::start(rx_thread_data_t *rx_thread_data_)
{
    rx_thread_data = rx_thread_data_;
    rx_thread_data->reset();
    rx_thread_data->len_buffer = USRP_STREAM_SIZE;
    thread_rx = new std::thread(&usrp_rx::start, dev_rx, device, rx_thread_data);
    thread_rx->detach();

    thread_tx = new std::thread(&usrp_tx::start, dev_tx, device);
    thread_tx->detach();
}
//-----------------------------------------------------------------------------------------
void usrp::stop()
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
void usrp::set_rx_rf_bandwidth(long long int bandwidht_hz_)
{
    double bw = bandwidht_hz_;
    device->set_rx_bandwidth(bw, channel);
}
//-----------------------------------------------------------------------------------------
void usrp::set_rx_sampling_frequency(long long int sampling_frequency_hz_)
{
    double sr = sampling_frequency_hz_;
    device->set_rx_rate(sr, channel);
}
//-----------------------------------------------------------------------------------------
void usrp:: get_rx_max_hardwaregain(double &hardwaregain_db_)
{
    uhd::gain_range_t gain_range = device->get_rx_gain_range(channel);
    hardwaregain_db_ = gain_range.stop();
}
//-----------------------------------------------------------------------------------------
void usrp::set_rx_hardwaregain(double hardwaregain_db_)
{
    device->set_rx_gain(hardwaregain_db_, "", channel);
}
//-----------------------------------------------------------------------------------------
void usrp::set_rx_frequency(long long int frequency_hz_)
{
    double fq = frequency_hz_;
    device->set_rx_freq(uhd::tune_request_t(fq), channel);
    QThread::usleep(30);
}
//-----------------------------------------------------------------------------------------
void usrp::set_tx_rf_bandwidth(long long int bandwidht_hz_)
{
    double bw = bandwidht_hz_;
    device->set_tx_bandwidth(bw, channel);
}
//-----------------------------------------------------------------------------------------
void usrp::set_tx_sampling_frequency(long long int sampling_frequency_hz_)
{
    double sr = sampling_frequency_hz_;
    device->set_tx_rate(sr, channel);
}
//-----------------------------------------------------------------------------------------
void usrp:: get_tx_max_hardwaregain(double &hardwaregain_db_)
{
    uhd::gain_range_t gain_range = device->get_tx_gain_range(channel);
    hardwaregain_db_ = gain_range.stop();
}
//-----------------------------------------------------------------------------------------
void usrp::set_tx_hardwaregain(double hardwaregain_db_)
{
    uhd::gain_range_t gain_range = device->get_tx_gain_range(0);
    device->set_tx_gain(hardwaregain_db_, channel);
}
//-----------------------------------------------------------------------------------------
void usrp::set_tx_frequency(long long int frequency_hz_)
{
    double fq = frequency_hz_;
    device->set_tx_freq(uhd::tune_request_t(fq), channel);
    QThread::usleep(30);
}
//-----------------------------------------------------------------------------------------
void usrp::advanced_settings_dialog()
{

}
//-----------------------------------------------------------------------------------------
