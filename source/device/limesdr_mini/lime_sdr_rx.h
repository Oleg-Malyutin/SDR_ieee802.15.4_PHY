#ifndef LIME_SDR_RX_H
#define LIME_SDR_RX_H

#include "driver/LimeSuite.h"
#include "device/device_type.h"

class lime_sdr_rx
{
public:
    explicit lime_sdr_rx();
    ~lime_sdr_rx();

    bool is_started = false;
    void start(lms_device_t *device_, rx_thread_data_t *rx_thread_data_);

private:
    double samplerate;
    lms_device_t *device;
    lms_stream_t stream;
    short *buffer[2] = {nullptr, nullptr};
    void work(rx_thread_data_t *rx_thread_data_);
    void stop();
};

#endif // LIME_SDR_RX_H
