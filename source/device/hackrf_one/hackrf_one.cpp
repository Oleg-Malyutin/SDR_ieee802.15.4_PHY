#include "hackrf_one.h"
#include "ui_hackrf_one.h"

#include <QDebug>

//-----------------------------------------------------------------------------------------
hackrf_one::hackrf_one(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::hackrf_one)
{
    ui->setupUi(this);

    dev_rx = new hackrf_one_rx;
    dev_tx = new hackrf_one_tx;
    rx_tx_mutex = new std::mutex;
}
//-----------------------------------------------------------------------------------------
hackrf_one::~hackrf_one()
{
    qDebug() << "hackrf_one::~hackrf_one() start";
    delete ui;
    stop();
    delete dev_rx;
    delete dev_tx;
    delete rx_tx_mutex;
    qDebug() << "hackrf_one::~hackrf_one() stop";
}
//-----------------------------------------------------------------------------------------
std::string hackrf_one::error (int err)
{
    switch (err) {
    case HACKRF_SUCCESS:
        return "Success";
    case HACKRF_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case HACKRF_ERROR_NOT_FOUND:
        return "HackRF not found";
    case HACKRF_ERROR_BUSY:
        return "Device busy";
    case HACKRF_ERROR_NO_MEM:
        return "Out of memory error";
    case HACKRF_ERROR_LIBUSB:
        return "Libusb error";
    case HACKRF_ERROR_THREAD:
        return "HackRF thread error";
    case HACKRF_ERROR_STREAMING_THREAD_ERR:
        return "HackRF streaming thread error";
    case HACKRF_ERROR_STREAMING_STOPPED:
        return "HackRF error: streaming stopped";
    case HACKRF_ERROR_STREAMING_EXIT_CALLED:
        return "HackRF error: exit called";
    case HACKRF_ERROR_USB_API_VERSION:
        return "HackRF wrong USB api version";
    case HACKRF_ERROR_OTHER:
        return "HackRF error: other";
    default:
        return "Unknown error";
    }
}
//-----------------------------------------------------------------------------------------
bool hackrf_one::open_device(std::string &name_, std::string &err_)
{
    name_.clear();
    int err;

    err = hackrf_init();
    if(err != HACKRF_SUCCESS){
        err_ = "HackRF One open failed: " + std::string(hackrf_error_name(static_cast<hackrf_error>(err)));

        return false;

    }

    hackrf_device_list_t *list = hackrf_device_list();
    if(list->devicecount < 1){
        err_ = "HackRF One open failed: no HackRF boards found";

        return false;

    }

    std::string serial("");
    if(list->serial_numbers[0]){
        serial = list->serial_numbers[0];
        if(serial.length() > 6){
            serial = serial.substr(serial.length() - 6, 6);
        }
    }

    device = NULL;
    err = hackrf_device_list_open(list, 0, &device);
    if(err != HACKRF_SUCCESS){
        err_ = "HackRF One open failed: " + std::string(hackrf_error_name(static_cast<hackrf_error>(err)));

        return false;

    }

    uint8_t board_id = BOARD_ID_UNDETECTED;
    uint8_t board_rev = BOARD_REV_UNDETECTED;
    uint32_t supported_platform = 0;
    char version[255 + 1];
    err = hackrf_board_id_read(device, &board_id);
    if(err != HACKRF_SUCCESS){
        err_ = "HackRF One open failed: " + std::string(hackrf_error_name(static_cast<hackrf_error>(err)));

        return false;

    }
    fprintf(stderr,"Board ID Number: %d (%s)\n", board_id, hackrf_board_id_name(static_cast<hackrf_board_id>(board_id)));
    fprintf(stderr,"Board rev: %d (%s)\n", board_id, hackrf_board_rev_name(static_cast<hackrf_board_rev>(board_id)));

    name_.append(hackrf_board_id_name(static_cast<hackrf_board_id>(board_id)));

    read_partid_serialno_t read_partid_serialno;
    err = hackrf_board_partid_serialno_read(device, &read_partid_serialno);
    if(err != HACKRF_SUCCESS){
        err_ = "HackRF One open failed: " + std::string(hackrf_error_name(static_cast<hackrf_error>(err)));

        return false;

    }
    fprintf(stderr, "Part ID Number: 0x%08x 0x%08x\n", read_partid_serialno.part_id[0], read_partid_serialno.part_id[1]);

    err = hackrf_version_string_read(device, &version[0], 255);
    if(err != HACKRF_SUCCESS){
        err_ = "HackRF One open failed: " + std::string(hackrf_error_name(static_cast<hackrf_error>(err)));

        return false;

    }
    fprintf(stderr, "Firmware Version: %s\n", version);

    std::stringstream stream;
    stream << std::hex << read_partid_serialno.part_id[0];
    stream << std::hex << read_partid_serialno.part_id[1];
//    _ser_no.append(stream.str());
//    _hw_ver.append(hackrf_board_rev_name(static_cast<hackrf_board_rev>(board_id)));

    hackrf_device_list_free(list);

    hackrf_close(device);

    err = hackrf_open(&device, &usb_device);
    if(err != HACKRF_SUCCESS){
        err_ = "HackRF One open failed: " + std::string(hackrf_error_name(static_cast<hackrf_error>(err)));

        return false;

    }

    err_ = "";

    return true;
}
//-----------------------------------------------------------------------------------------
void hackrf_one::close_device()
{
    if (device != nullptr) {
//        hackrf_stop_rx(device);
        hackrf_close(device);
    }
}
//-----------------------------------------------------------------------------------------
bool hackrf_one::check_connect()
{
    uint8_t value;
    int err = hackrf_get_clkin_status(device, &value);
    if(err != HACKRF_SUCCESS){

        return false;

    }

    return true;
}
//-----------------------------------------------------------------------------------------
void hackrf_one::start(rx_thread_data_t *rx_thread_data_)
{
    rx_thread_data = rx_thread_data_;
    rx_thread_data->reset();
    rx_thread_data->len_buffer = 1024 * 2;
    thread_rx = new std::thread(&hackrf_one_rx::start, dev_rx, rx_thread_data, usb_device, rx_tx_mutex);
    thread_rx->detach();

    thread_tx = new std::thread(&hackrf_one_tx::start, dev_tx, usb_device, rx_tx_mutex);
    thread_tx->detach();
}
//-----------------------------------------------------------------------------------------
void hackrf_one::stop()
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
void hackrf_one::set_rx_rf_bandwidth(long long int bandwidht_hz_)
{
    uint32_t bw_hz = bandwidht_hz_;
    uint32_t bw = hackrf_compute_baseband_filter_bw(bw_hz);
    hackrf_set_baseband_filter_bandwidth(device, bw );
}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_rx_sampling_frequency(long long int sampling_frequency_hz_)
{
    const double rx_sample_rate_hz = sampling_frequency_hz_;
    hackrf_set_sample_rate(device, rx_sample_rate_hz);
}
//-----------------------------------------------------------------------------------------
void hackrf_one::get_rx_max_hardwaregain(double &hardwaregain_db_)
{
    hardwaregain_db_ = 40 + 62;
    hardwaregain_db_ -= 30.0; // correction from practice
}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_rx_hardwaregain(double hardwaregain_db_)
{
    int gain = hardwaregain_db_ + 11;
    uint32_t clip_gain = 0;
    int err = 0;
    int amp_enable = 0;
    if(gain >= 12){
        amp_enable = 1;
        gain -= 12;
    }
    err |= hackrf_set_amp_enable(device, amp_enable);
    fprintf(stderr, "AMP=%d\n", amp_enable);
    if(gain >= 40){
        clip_gain = 40;
    }
    else{
        clip_gain = gain - gain % 8;
    }
    err |= hackrf_set_lna_gain(device, clip_gain);
    gain -= clip_gain;
    fprintf(stderr, "LNA=%d\n", clip_gain);
    if(gain >= 62){
        clip_gain = 62;
    }
    else{
        clip_gain = gain - gain % 2;
    }
    err |= hackrf_set_vga_gain(device, uint32_t(clip_gain));
    fprintf(stderr, "VGA=%d\n", clip_gain);
}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_rx_frequency(long long int frequency_hz_)
{
    uint32_t fq = frequency_hz_;
    if(hackrf_set_freq(device, fq) != 0){
        fprintf(stderr, "hackrf_one::set_rx_frequency fail!\n");
    }
    fprintf(stderr, "hackrf_one::set_rx_frequency: %u\n", fq);
}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_tx_rf_bandwidth(long long int bandwidht_hz_)
{

}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_tx_sampling_frequency(long long int sampling_frequency_hz_)
{

}
//-----------------------------------------------------------------------------------------
void hackrf_one::get_tx_max_hardwaregain(double &hardwaregain_db_)
{
    hardwaregain_db_ = 47;
}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_tx_hardwaregain(double hardwaregain_db_)
{
    uint32_t vga_gain = hardwaregain_db_;
    if(vga_gain > 47){
        vga_gain = 47;
    }
    int err = 0;
    err |= hackrf_set_txvga_gain(device, vga_gain);
    err |= hackrf_set_amp_enable(device, 1);
    err |= hackrf_set_antenna_enable(device, 0);

    fprintf(stderr, "hackrf_one::set_tx_hardwaregain err %d\n", err);
}
//-----------------------------------------------------------------------------------------
void hackrf_one::set_tx_frequency(long long int frequency_hz_)
{

}
//-----------------------------------------------------------------------------------------
void hackrf_one::advanced_settings_dialog()
{

}
//-----------------------------------------------------------------------------------------
