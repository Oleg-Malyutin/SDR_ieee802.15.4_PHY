#ifndef USRP_TX_H
#define USRP_TX_H

#include <uhd/usrp/multi_usrp.hpp>

#include "device/device_type.h"

class usrp_tx : public rf_tx_callback
{
public:
    usrp_tx();
    ~usrp_tx();

    bool is_started = false;
    void start(uhd::usrp::multi_usrp::sptr _device);
    void stop();

private:
    uhd::usrp::multi_usrp::sptr device;
    uhd::tx_streamer::sptr tx_stream;
    uhd::tx_metadata_t metadata;
    const int buffer_size = 1024 * 8;
    int16_t *buffer;
    double gain_max;
    double gain_min;
    bool tx_data(uint &len_buffer_, const float *ptr_buffer_);
    void reset_buffer();

};

#endif // USRP_TX_H
