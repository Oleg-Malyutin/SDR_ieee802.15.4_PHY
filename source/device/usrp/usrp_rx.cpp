#include "usrp_rx.h"

//-----------------------------------------------------------------------------------------
usrp_rx::usrp_rx()
{
    samples_count_iq = USRP_STREAM_SIZE;
    stream_buffer_a = new int16_t [samples_count_iq * 2];
    stream_buffer_b = new int16_t [samples_count_iq * 2];
    buffer_a = new std::complex<float>[samples_count_iq];
    buffer_b = new std::complex<float>[samples_count_iq];
}
//-----------------------------------------------------------------------------------------
usrp_rx::~usrp_rx()
{
    delete[] stream_buffer_a;
    delete[] stream_buffer_b;
    delete[] buffer_a;
    delete[] buffer_b;
}
//-----------------------------------------------------------------------------------------
void usrp_rx::start(uhd::usrp::multi_usrp::sptr _device, rx_thread_data_t *_rx_thread_data)
{
    device = _device;
    uhd::stream_args_t stream_args("sc16","sc16");
    rx_stream = device->get_rx_stream(stream_args);
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);
    rx_thread_data = _rx_thread_data;
    rx_thread_data->status = Connect;
    rx_thread_data->stop = false;
    rx_thread_data->set_mode = rx_mode_data;
    thread_buffer_refill = std::thread(&usrp_rx::buffer_refill, this);
    is_started = true;

    uhd::gain_range_t gain_range = device->get_rx_gain_range(0);

    fprintf(stderr, "usrp_rx::start() gain_range %f %f\n", gain_range.start(), gain_range.stop());

    work();
}
//-----------------------------------------------------------------------------------------
void usrp_rx::stop()
{
    if(is_started){
        is_started = false;
        rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    }
}
//-----------------------------------------------------------------------------------------
void usrp_rx::work()
{
    std::unique_lock<std::mutex> lock(rx_event.mutex);

    while(!rx_thread_data->stop){

        if(rx_thread_data->status != Connect){

            break;

        }
        rx_data(lock);

    }

    stop();

    fprintf(stderr, "usrp_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void usrp_rx::buffer_refill()
{
    const double timeout = 0.001;
    const bool one_packet = false;

    while(!rx_thread_data->stop){

//        auto start_time = std::chrono::steady_clock::now();

        int16_t *ptr_buffer = rx_event.swap ? stream_buffer_b : stream_buffer_a;
        rx_event.swap = !rx_event.swap;
        int samples_read = rx_stream->recv(ptr_buffer, samples_count_iq, metadata, timeout, one_packet);

        rx_event.mutex.lock();

        rx_event.samples_read = samples_read;
        rx_event.ptr_buffer = ptr_buffer;
        rx_event.ready = true;

        rx_event.mutex.unlock();
        rx_event.condition.notify_all();

        if(metadata.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
            fprintf(stderr, "metadata.error_code %d\n", metadata.error_code);
        }

//        auto end_time = std::chrono::steady_clock::now();
//        auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
//        fprintf(stderr, "usrp_rx::buffer_refill: elapsed_time  %ld  samples=%d\n", elapsed_time, rx_event.samples_read );

    }
}
//-----------------------------------------------------------------------------------------
void usrp_rx::rx_data(std::unique_lock<std::mutex> &lock_)
{
    rx_event.condition.wait(lock_);
    if(rx_event.ready){
        rx_event.ready = false;

        int samples_read = rx_event.samples_read;
        int16_t *ptr_stream_buffer = rx_event.ptr_buffer;

//        if(rx_thread_data->stop || metadata.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
//            rx_thread_data->stop = true;
//            if(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT){
//                rx_thread_data->status = Disconnect;
//            }
//            else{
//                rx_thread_data->status = Unknow;
//            }

//            fprintf(stderr, "usrp_rx::rx_data thread stop %d\n", metadata.error_code);

//            return;
//        }

        ptr_buffer = swap ? buffer_b : buffer_a;
        swap = !swap;
        for(int i = 0; i < samples_read; ++i){
            ptr_buffer[i].real(ptr_stream_buffer[0] * scale);
            ptr_buffer[i].imag(ptr_stream_buffer[1] * scale);
            ptr_stream_buffer += 2;
        }
        float sum = 0.0f;
        for(int i = 0; i < samples_read; ++i){
            sum += norm(ptr_buffer[i]);
        }
        if(sum == 0.0f) {
            fprintf(stderr, "usrp_rx::rx_data sum %f\n", sum);

            return;

        }
        sum += 0.000512f;
        float len_avg = samples_read;
        float rssi = 10.0f * log10(sum / len_avg * 0.707f) - 52.0f;

        rx_thread_data->mutex.lock();

        rx_thread_data->len_buffer = samples_read;
        rx_thread_data->ptr_buffer = ptr_buffer;
        rx_thread_data->rssi = rssi;
        rx_thread_data->ready = true;

        rx_thread_data->mutex.unlock();
        rx_thread_data->condition.notify_one();

    }

}
//-----------------------------------------------------------------------------------------

