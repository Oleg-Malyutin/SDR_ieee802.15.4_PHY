#include "lime_sdr_tx.h"

//-----------------------------------------------------------------------------------------
lime_sdr_tx::lime_sdr_tx()
{

}
//-----------------------------------------------------------------------------------------
lime_sdr_tx::~lime_sdr_tx()
{
    fprintf(stderr, "lime_sdr_tx::~pluto_sdr_tx() start\n");
    fprintf(stderr, "lime_sdr_tx::~pluto_sdr_tx() stop\n");
}
//-----------------------------------------------------------------------------------------
void lime_sdr_tx::start(lms_device_t *device_, double bandwidth_calibrating_)
{
    stop();
    device = device_;
    buffer = new int16_t[buffer_size];
    reset_buffer();
    //initialize stream
    stream.channel = 0;
    stream.fifoSize = buffer_size;
    stream.throughputVsLatency = 0.1;//optimize for max throughput
    stream.isTx = LMS_CH_TX;
    stream.dataFmt = lms_stream_t::LMS_FMT_I16;
    stream.linkFmt = lms_stream_t::LMS_LINK_FMT_I16;
    if(LMS_SetupStream(device, &stream) < 0){
        fprintf(stderr, "lime_sdr_tx::start: LMS_SetupStream: %s\n", LMS_GetLastErrorMessage());
        stop();

        return;

    }
    //start stream
    tx_metadata.flushPartialPacket = true; //do not force sending of incomplete packet
    tx_metadata.waitForTimestamp = false; //Enable synchronization to HW timestamp
    tx_metadata.timestamp = 0;
    int err = LMS_StartStream(&stream);
    if(err != 0){
        fprintf(stderr, "lime_sdr_tx::start:LMS_StartStream: %s\n", LMS_GetLastErrorMessage());

        return;

    }

    is_started = true;
}
//-----------------------------------------------------------------------------------------
void lime_sdr_tx::stop()
{
    if(is_started){
        is_started = false;
        shutdown();
    }
}
//-----------------------------------------------------------------------------------------
void lime_sdr_tx::shutdown()
{
    LMS_StopStream(&stream);
    LMS_DestroyStream(device, &stream);
    delete [] buffer;
    is_started = false;
}
//-----------------------------------------------------------------------------------------
bool lime_sdr_tx::tx_data(uint &len_buffer_, const float *ptr_buffer_)
{
    bool succes = true;

//    LMS_EnableChannel(device, LMS_CH_TX, 0, true);

    for (uint i = 0; i < len_buffer_; ++i){
        buffer[i] = ptr_buffer_[i] * 32767.0f;
    }

    LMS_SendStream(&stream, buffer, buffer_size, &tx_metadata, 1000);

    reset_buffer();

//    LMS_StopStream(&stream);
//    LMS_EnableChannel(device, LMS_CH_TX, 0, false);

    return succes;
}
//-----------------------------------------------------------------------------------------
void lime_sdr_tx::reset_buffer()
{
    for (int i = 0; i < buffer_size; ++i){
        buffer[i] = 0.0f;
    }
}
//-----------------------------------------------------------------------------------------
