#ifndef LIME_SDR_TX_H
#define LIME_SDR_TX_H

#include "driver/LimeSuite.h"
#include "device/device_type.h"

class lime_sdr_tx
{
public:
    explicit lime_sdr_tx();
    ~lime_sdr_tx();

    bool is_started = false;
    void start(lms_device_t *device_, tx_thread_data_t *tx_thread_data_,
               double bandwidth_calibrating_, float latency_);

private:
    lms_device_t *device;
    lms_stream_t tx_stream;
    void work(tx_thread_data_t *tx_thread_data_);
    void tx_data(int &len_buffer_, const float *ptr_buffer);
    void stop();
    void shutdown();
};

#endif // LIME_SDR_TX_H
