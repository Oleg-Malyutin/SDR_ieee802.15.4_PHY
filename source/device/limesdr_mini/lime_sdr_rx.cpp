#include "lime_sdr_rx.h"

//-----------------------------------------------------------------------------------------
lime_sdr_rx::lime_sdr_rx()
{

}
//-----------------------------------------------------------------------------------------
lime_sdr_rx::~lime_sdr_rx()
{
    fprintf(stderr, "lime_sdr_rx::~lime_sdr_rx() stop\n");
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::start(lms_device_t *device_, rx_thread_data_t *rx_thread_data_)
{
    //initialize stream
    stream.channel = 0;
    stream.fifoSize = rx_thread_data_->len_buffer;
    stream.throughputVsLatency = 0.1;//optimize for max throughput
    stream.isTx = LMS_CH_RX;
    stream.dataFmt = lms_stream_t::LMS_FMT_I12;
    stream.linkFmt = lms_stream_t::LMS_LINK_FMT_DEFAULT;
    if (LMS_SetupStream(device_, &stream) < 0){
        fprintf(stderr, "lime_sdr_rx::start: LMS_SetupStream() : %s\n", LMS_GetLastErrorMessage());
        stop();

        return;

    }
    device = device_;
    // enable rx channels
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0){
        fprintf(stderr, "rx_limesdr::open_device: Do not enable channel TX!\n");
        stop();

        return;

    }
//    //configure LPF
//    if (LMS_SetLPFBW(device, LMS_CH_RX, 0, 2000000.0) != 0){
//        fprintf(stderr, "rx_limesdr::open_device: Do not set LPFBW! \n");

//        return;

//    }
    float_type host_hz, rf_hz;
    LMS_GetSampleRate(device, LMS_CH_RX, 0, &host_hz, &rf_hz);
    samplerate = host_hz;
    // perform automatic calibration 
    if (LMS_Calibrate(device, LMS_CH_RX, 0, 2500000.0, 0) < 0){
        fprintf(stderr, "LMS_Calibrate() : %s\n", LMS_GetLastErrorMessage());
        stop();

        return;

    }
    is_started = true;
    work(rx_thread_data_);
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::work(rx_thread_data_t *rx_thread_data_)
{
//#define RETING
    static int c = 0;
    rx_thread_data_t *rx_thread_data = rx_thread_data_;
    int samples_count_iq = rx_thread_data->len_buffer;
    int16_t *buffer_a = new int16_t [samples_count_iq * 2];
    int16_t *buffer_b = new int16_t [samples_count_iq * 2];
    bool swap = true;
    int16_t *ptr_buffer = buffer_a;
    uint32_t timeout_ms = samples_count_iq * 1000.0 / samplerate * 0.9;
    LMS_StartStream(&stream);

    while(!rx_thread_data->stop){

#ifdef RETING
        auto start_time = std::chrono::steady_clock::now();
#endif

        int samples_read = LMS_RecvStream(&stream, ptr_buffer, samples_count_iq, nullptr, timeout_ms);

#ifdef RETING
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        auto elapsed_time = elapsed_ns.count();
        fprintf(stderr, "LMS_RecvStream reting=  %f\n", elapsed_time / 1000.f);
#endif
        if(rx_thread_data->stop || samples_read == -1) {
            rx_thread_data->stop = true;
            if(LMS_GetDeviceList(nullptr) == -1){
                rx_thread_data->status = Disconnect;
            }
            else{
                rx_thread_data->status = Unknow;
            }
            break;
        }
        else if(samples_read != samples_count_iq){
            fprintf(stderr, "LimeSDR: number of samples received on failure %d\n", samples_count_iq - samples_read);
        }

        if (rx_thread_data->mutex.try_lock()){
            rx_thread_data->len_buffer = samples_read;
            if(swap){
                swap = false;
                rx_thread_data->ptr_buffer = buffer_a;
                ptr_buffer = buffer_b;
            }
            else{
                swap = true;
                rx_thread_data->ptr_buffer = buffer_b;
                ptr_buffer = buffer_a;
            }
            rx_thread_data->ready = true;
            rx_thread_data->mutex.unlock();
            rx_thread_data->cond_value.notify_one();
            c = 0;
        }
        else{
            ++c;
            fprintf(stderr, "lime_sdr_rx::work: skip buffer %d\n", c);
        }

    }
    rx_thread_data->ready = false;
    rx_thread_data->cond_value.notify_all();
    stop();

    fprintf(stderr, "lime_sdr_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::stop()
{
    LMS_StopStream(&stream);
    LMS_DestroyStream(device, &stream);
    is_started = false;
}
//-----------------------------------------------------------------------------------------


