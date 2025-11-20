#include "lime_sdr_rx.h"

#include <iostream>

//-----------------------------------------------------------------------------------------
lime_sdr_rx::lime_sdr_rx()
{

}
//-----------------------------------------------------------------------------------------
lime_sdr_rx::~lime_sdr_rx()
{
    fprintf(stderr, "lime_sdr_rx::~lime_sdr_rx() start\n");
    fprintf(stderr, "lime_sdr_rx::~lime_sdr_rx() stop\n");
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::start(lms_device_t *device_, rx_thread_data_t *rx_thread_data_)
{
    device = device_;
    rx_thread_data = rx_thread_data_;
    samples_count_iq = LMS_FIFO_SIZE;//rx_thread_data->len_buffer;
    stream_buffer_a = new int16_t [samples_count_iq * 2];
    stream_buffer_b = new int16_t [samples_count_iq * 2];
    buffer_a = new std::complex<float>[samples_count_iq];
    buffer_b = new std::complex<float>[samples_count_iq];
    //initialize stream
    stream.channel = 0;
    stream.fifoSize = LMS_FIFO_SIZE;//rx_thread_data->len_buffer;
    stream.throughputVsLatency = 0.1;//optimize for max throughput
    stream.isTx = LMS_CH_RX;
    stream.dataFmt = lms_stream_t::LMS_FMT_I16;
    stream.linkFmt = lms_stream_t::LMS_LINK_FMT_I16;
    timeout_ms = samples_count_iq * 1000.0 / (samplerate * 0.5);
    rx_metadata.flushPartialPacket = false; //do not force sending of incomplete packet
    rx_metadata.waitForTimestamp = false; //Enable synchronization to HW timestamp
    // setup stream
    if (LMS_SetupStream(device, &stream) < 0){
        fprintf(stderr, "lime_sdr_rx::start:LMS_SetupStream: %s\n", LMS_GetLastErrorMessage());
        stop();

        return;

    }
    // start stream
    int err = LMS_StartStream(&stream);
    if(err != 0){
        fprintf(stderr, "lime_sdr_rx::start:LMS_StartStream: %s\n", LMS_GetLastErrorMessage());

        return;

    }
    rx_thread_data->status = Connect;
    rx_thread_data->stop = false;
    rx_thread_data->set_mode = rx_mode_data;
    thread_buffer_refill = std::thread(&lime_sdr_rx::buffer_refill, this);
    // RSSI
    err |= LMS_WriteParam(device, LMS7_AGC_BYP_RXTSP , 1);
    err |= LMS_WriteParam(device, LMS7_AGC_MODE_RXTSP, 1);
    err |= LMS_WriteParam(device, LMS7_AGC_AVG_RXTSP, 1);
    if(err != 0){
        fprintf(stderr, "lime_sdr_rx::start:LMS_StartStream: %s\n", LMS_GetLastErrorMessage());

        return;

    }

    is_started = true;

    work();
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::stop()
{
    rx_thread_data->ready = false;
    rx_thread_data->stop = true;
    rx_event.ready = false;
    rx_event.mutex.unlock();
    rx_event.condition.notify_all();
    if(thread_buffer_refill.joinable()){
        thread_buffer_refill.join();
    }
    LMS_StopStream(&stream);
    LMS_DestroyStream(device, &stream);
    delete [] stream_buffer_a;
    delete [] stream_buffer_b;
    delete [] buffer_a;
    delete [] buffer_b;
    is_started = false;
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::work()
{
    std::unique_lock<std::mutex> lock(rx_event.mutex);

    while(!rx_thread_data->stop){

        if(rx_thread_data->set_mode == rx_mode_data){
            rx_data(lock);
        }
        else if(rx_thread_data->set_mode == rx_mode_rssi){
            rx_rssi();
        }
        else if(rx_thread_data->set_mode == rx_mode_off){
            rx_off();
        }
        if(rx_thread_data->status != Connect){

            break;

        }

    }

    stop();

    fprintf(stderr, "lime_sdr_rx::work() finish\n");
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::buffer_refill()
{
    while(!rx_thread_data->stop){
//        #define REITING
#ifdef REITING
        auto start_time = std::chrono::steady_clock::now();
#endif
        int16_t *ptr_buffer = rx_event.swap ? stream_buffer_b : stream_buffer_a;
        rx_event.swap = !rx_event.swap;
        rx_event.samples_read = LMS_RecvStream(&stream, ptr_buffer, samples_count_iq, nullptr, 1000/*timeout_ms*/);
        rx_event.ptr_buffer = ptr_buffer;
        rx_event.ready = true;
        rx_event.condition.notify_all();

#ifdef REITING
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        fprintf(stderr, "pluto_sdr_rx::work: elapsed_time  %ld  samples=%d\n", elapsed_time, rx_event.samples_read );
#endif
    }
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::rx_data(std::unique_lock<std::mutex> &lock_)
{
    static int c = 0;

    rx_event.condition.wait(lock_);
    if(rx_event.ready){
        rx_event.ready = false;

        int samples_read = rx_event.samples_read;
        int16_t *ptr_stream_buffer = rx_event.ptr_buffer;

        if(rx_thread_data->stop || samples_read == -1) {
            rx_thread_data->stop = true;
            if(LMS_GetDeviceList(nullptr) == -1){
                rx_thread_data->status = Disconnect;
            }
            else{
                rx_thread_data->status = Unknow;
            }

            return;
        }
        else if(samples_read != samples_count_iq){
            fprintf(stderr,
                    "LimeSDR: number of samples received on failure %d\n",
                    samples_count_iq - samples_read);
            lms_stream_status_t status;
            LMS_GetStreamStatus(&stream, &status);
            //            std::cout << "RX rate: " << status.linkRate / 1e6 << " MB/s\n";
            //            std::cout << "RX fifo: " << 100 * status.fifoFilledCount / status.fifoSize << "%" << std::endl;
            fprintf(stderr, "status fifoSize=%d active=%d overrun=%d underrun=%d droppedPackets=%d\n",
                    status.fifoSize, status.active, status.overrun, status.underrun, status.droppedPackets);
        }
        ptr_buffer = swap ? buffer_b : buffer_a;
        swap = !swap;
        int j = 0;
        for(int i = 0; i < samples_read; ++i){
            float im = ptr_stream_buffer[j];
            float qt = ptr_stream_buffer[j + 1];
            ptr_buffer[i].real(im * 2e-4f);
            ptr_buffer[i].imag(qt * 2e-4f);
            //            energy_detect = avg_energy_detect(norm(data));
            j += 2;
        }
//        if (rx_thread_data->mutex.try_lock()){

            rx_thread_data->mutex.lock();

            rx_thread_data->len_buffer = samples_read;
            rx_thread_data->ptr_buffer = ptr_buffer;
            rx_thread_data->rssi = -126;//energy_detect;
            rx_thread_data->mode = rx_mode_data;
            rx_thread_data->ready = true;
            rx_thread_data->mutex.unlock();
            rx_thread_data->condition_value.notify_one();
            c = 0;
//        }
//        else{
//            ++c;
//            fprintf(stderr, "lime_sdr_rx::rx_data: skip buffer %d\n", c);
//        }

        //        lms_stream_status_t status;
        //        LMS_GetStreamStatus(&stream, &status);
        //        std::cout << "RX rate: " << status.linkRate / 1e6 << " MB/s\n";
        //        std::cout << "RX fifo: " << 100 * status.fifoFilledCount / status.fifoSize << "%" << std::endl;
    }

}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::rx_rssi()
{
    double rssi = 126;
    int err = 0;
    uint16_t msb_reg;
    uint16_t lsb_reg;

    LMS_WriteParam(device, LMS7_AGC_BYP_RXTSP , 0);

    //Read RSSI
//        LMS_WriteParam(device, LMS7_CAPSEL_ADC, 0);
    err |= LMS_WriteParam(device, LMS7_CAPSEL, 0);
    err |= LMS_WriteParam(device, LMS7_CAPTURE, 0);
    err |= LMS_WriteParam(device, LMS7_CAPTURE, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    err |= LMS_WriteParam(device, LMS7_CAPTURE, 0);
    err |= LMS_ReadLMSReg(device, 0x040f, &msb_reg);
    err |= LMS_ReadLMSReg(device, 0x040e, &lsb_reg);
    uint32_t result = ((msb_reg << 0x2) | (lsb_reg & 0x3)) & 0x1ffff;
    err |= LMS_WriteParam(device, LMS7_AGC_BYP_RXTSP , 1);

//    if(result > 62000){
//        fprintf(stderr, "lime_sdr_rx::rx_rssi() : err=%d result %d %d %d\n", err, result, msb_reg, lsb_reg);
//    }

    rssi = 126.0 - result * 1.5e-3f;

    rx_thread_data->mutex.lock();
    rx_thread_data->rssi = -rssi;
    if(err != 0){
        rx_thread_data->status = Unknow;
    }
    rx_thread_data->ready = true;
    rx_thread_data->mode = rx_mode_rssi;
    rx_thread_data->mutex.unlock();
    rx_thread_data->condition_value.notify_one();
}
//-----------------------------------------------------------------------------------------
void lime_sdr_rx::rx_off()
{
    rx_thread_data->mutex.lock();
    rx_thread_data->ready = true;
    rx_thread_data->mode = rx_mode_off;
    rx_thread_data->mutex.unlock();
    rx_thread_data->condition_value.notify_one();

    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // TODO 8 symbols
}
//-----------------------------------------------------------------------------------------



