#include "usrp_tx.h"

#include <uhd/stream.hpp>

//-----------------------------------------------------------------------------------------
usrp_tx::usrp_tx()
{
    buffer = new int16_t[buffer_size];
}
//-----------------------------------------------------------------------------------------
usrp_tx::~usrp_tx()
{
    delete[] buffer;
}
//-----------------------------------------------------------------------------------------
void usrp_tx::start(uhd::usrp::multi_usrp::sptr _device)
{
    device = _device;
    uhd::stream_args_t stream_args("sc16","sc16");
    tx_stream = device->get_tx_stream(stream_args);
    metadata.has_time_spec = false;
    metadata.start_of_burst = true;
    metadata.end_of_burst = true;
    uhd::gain_range_t gain_range = device->get_tx_gain_range(0);
    gain_min = gain_range.start();
    gain_max = gain_range.stop();
    device->set_tx_gain(gain_min);
    is_started = true;

}
//-----------------------------------------------------------------------------------------
void usrp_tx::stop()
{
    is_started = false;
}
//-----------------------------------------------------------------------------------------
bool usrp_tx::tx_data(uint &len_buffer_, const float *ptr_buffer_)
{
    bool succes = true;

    for (uint i = 0; i < len_buffer_; ++i){
        buffer[i] = ptr_buffer_[i] * 32767.0f;
    }
    size_t nsamps_per_buff = len_buffer_ / 2;
    device->set_tx_gain(gain_max);
    tx_stream->send(buffer, nsamps_per_buff, metadata);
    // Wait for the end of burst ACK followed by an underflow
    bool got_ack = false;
    bool got_underflow = false;
    uhd::async_metadata_t async_metadata;
    while(!got_ack && !got_underflow && tx_stream->recv_async_msg(async_metadata, 1))
    {
        got_ack = (async_metadata.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK);
        got_underflow = (got_ack && async_metadata.event_code == uhd::async_metadata_t::EVENT_CODE_UNDERFLOW);
    }
    device->set_tx_gain(gain_min);
    reset_buffer();

    return succes;
}
//-----------------------------------------------------------------------------------------
void usrp_tx::reset_buffer()
{
    for (int i = 0; i < buffer_size; ++i){
        buffer[i] = 0.0f;
    }
}
//-----------------------------------------------------------------------------------------
