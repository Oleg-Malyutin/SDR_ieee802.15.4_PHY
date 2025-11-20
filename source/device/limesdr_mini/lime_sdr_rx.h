#ifndef LIME_SDR_RX_H
#define LIME_SDR_RX_H

//#include <lime/LimeSuite.h>
#include "driver/LimeSuite.h"
#include "device/device_type.h"

#define LMS_FIFO_SIZE 1020//1360//1080//1024 * 8
//#define LMS_FIFO_SIZE 1024 * 8
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
//    short *buffer[2] = {nullptr, nullptr};
    std::complex<float> *ptr_buffer;
    std::complex<float> *buffer_a;
    std::complex<float> *buffer_b;
    bool swap = false;
    moving_average<float, 32> avg_energy_detect;
    float energy_detect = 0;
    rx_thread_data_t *rx_thread_data;
    int samples_count_iq;
    int16_t *stream_buffer_a;
    int16_t *stream_buffer_b;
    uint32_t timeout_ms;
    lms_stream_meta_t rx_metadata;
    std::thread thread_buffer_refill;
    struct event{
        bool ready = false;
        int samples_read;
        int16_t *ptr_buffer;
        bool swap = false;
        std::condition_variable condition;
        std::mutex mutex;
    };
    event rx_event;
    void work();
    void buffer_refill();
    void rx_data(std::unique_lock<std::mutex> &lock_);
    void rx_rssi();
    void rx_off();
    void stop();
};

#endif // LIME_SDR_RX_H
