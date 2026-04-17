#ifndef USRP_RX_H
#define USRP_RX_H

#include <uhd/usrp/multi_usrp.hpp>

#include "device/device_type.h"

#define USRP_STREAM_SIZE 512

class usrp_rx
{
public:
    usrp_rx();
    ~usrp_rx();

    bool is_started = false;
    void start(uhd::usrp::multi_usrp::sptr _device, rx_thread_data_t *_rx_thread_data);

    double max_rssi = -1000;
    double min_rssi = 1000;

private:
    uhd::usrp::multi_usrp::sptr device;
    uhd::rx_streamer::sptr rx_stream;
    uhd::rx_metadata_t metadata;
    rx_thread_data_t *rx_thread_data;
    struct event{
        bool ready = false;
        int samples_read;
        int16_t *ptr_buffer;
        bool swap = false;
        std::condition_variable condition;
        std::mutex mutex;
    };
    std::thread thread_buffer_refill;
    int samples_count_iq;
    int16_t *stream_buffer_a;
    int16_t *stream_buffer_b;
    std::complex<float> *ptr_buffer;
    std::complex<float> *buffer_a;
    std::complex<float> *buffer_b;
    bool swap = false;
    constexpr static float scale = 1.0f / 32767.0f;
    event rx_event;
    void work();
    void buffer_refill();
    void rx_data(std::unique_lock<std::mutex> &lock_);
    void stop();
};

#endif // USRP_RX_H
