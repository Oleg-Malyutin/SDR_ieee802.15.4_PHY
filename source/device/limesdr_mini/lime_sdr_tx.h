#ifndef LIME_SDR_TX_H
#define LIME_SDR_TX_H

//#include <lime/LimeSuite.h>
#include "driver/LimeSuite.h"
#include "device/device_type.h"

#define LMS_FIFO_SIZE 1020

class lime_sdr_tx : public rf_tx_callback
{
public:
    explicit lime_sdr_tx();
    ~lime_sdr_tx();

    bool is_started = false;
    void start(lms_device_t *device_, double bandwidth_calibrating_);
    void stop();

private:
    lms_device_t *device;
    lms_stream_t stream;
    const int buffer_size = LMS_FIFO_SIZE * 9;
    int16_t *buffer;
    lms_stream_meta_t tx_metadata;
    void reset_buffer();
    bool tx_data(uint &len_buffer_, const float *ptr_buffer_);

    void shutdown();
};

#endif // LIME_SDR_TX_H
