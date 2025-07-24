#include "tx_ieee802_15_4.h"

//--------------------------------------------------------------------------------------------------
tx_ieee802_15_4::tx_ieee802_15_4()
{
    chip_i = new float*[LEN_PN_SEQUENCE / 2];
    chip_q = new float*[LEN_PN_SEQUENCE / 2];
    int len = LEN_PN_SEQUENCE * (OVERSAMPLE * 2);
    for(int c = 0; c < 16; ++c){
        chip_i[c] = new float[len];
        chip_q[c] = new float[len];
    }

    mac_data = new mac_sublayer_data;
    mac_sublayer = new tx_mac_sublayer;
}
//--------------------------------------------------------------------------------------------------
tx_ieee802_15_4::~tx_ieee802_15_4()
{
    for(int c = 0; c < 16; ++c){
        delete[] chip_i[c];
        delete[] chip_q[c];
    }
    delete[] chip_i;
    delete[] chip_q;
}
//--------------------------------------------------------------------------------------------------
void tx_ieee802_15_4::symbol_generate()
{
    for(int c = 0; c < 16; ++c){
        for(int i = 0; i < LEN_PN_SEQUENCE / 2; ++i){
            for(int h = 0; h < 4; ++h){
                int step = 4 * i + h;
                chip_i[c][step] = chip_table[c][2 * i] * h_matched_filter[h];
                chip_q[c][step] = chip_table[c][2 * i + 1] * h_matched_filter[h];
            }
        }
    }
    // SHR
    for(int i = 0; i < LEN_MAX_SAMPLES * 2; ++i){
        ppdu[i] = 0;
    }
    offset_shr = 1;
    for(int c = 16 * 4 - 2; c < 16 * 4; ++c){
        ppdu[offset_shr++] = chip_q[0][c];
        offset_shr++;
    }
    offset_shr = 0;
    for(int z = 0; z < 8; ++z){
        for(int c = 0; c < 16 * 4; ++c){
            ppdu[offset_shr++] = chip_i[0][c];
            ppdu[offset_shr + SHIFT_IQ] = chip_q[0][c];
            ++offset_shr;
        }
    }
    // Start of Frame Delimiter
    for(int c = 0; c < 16 * 4; ++c){
        ppdu[offset_shr++] = chip_i[0x7][c];
        ppdu[offset_shr + SHIFT_IQ] = chip_q[0x7][c];
        ++offset_shr;
    }
//    tx_data[offset_shr - 2] = 2;
    for(int c = 0; c < 16 * 4; ++c){
        ppdu[offset_shr++] = chip_i[0xa][c];
        ppdu[offset_shr + SHIFT_IQ] = chip_q[0xa][c];
        ++offset_shr;
    }
}
//--------------------------------------------------------------------------------------------------
void tx_ieee802_15_4::start(tx_thread_data_t *tx_thread_data_)
{
    symbol_generate();

    mac_data->reset();
    mac_layer_thread = new std::thread(&tx_mac_sublayer::start, mac_sublayer, mac_data);
    mac_layer_thread->detach();

    work(tx_thread_data_, mac_data);
}
//--------------------------------------------------------------------------------------------------
void tx_ieee802_15_4::work(tx_thread_data_t *tx_thread_data_, mac_sublayer_data *mac_data_)
{
    std::unique_lock<std::mutex> read_lock(mac_data_->mutex);
    while (!mac_data_->stop){
        mac_data_->cond_value.wait(read_lock);
        if(mac_data_->ready){
            mac_data_->ready = false;
            tx_data(tx_thread_data_, mac_data_->mpdu);
        }
    }
    fprintf(stderr, "tx_ieee802_15_4::work() finish\n");
}
//--------------------------------------------------------------------------------------------------
void tx_ieee802_15_4::tx_data(tx_thread_data_t *tx_thread_data_, std::vector<uint8_t> *mpdu_)
{
    uint16_t len_data = mpdu_->size();
    if(len_data > MAX_PHY_PAYLOAD){
        fprintf(stderr, "tx_ieee802_15_4::tx_data() error: data size data size is greater than MAX_PHY_PAYLOAD(127)\n");

        return;

    }

    int offset = offset_shr;
    // PHR
    int d = len_data & 0xf;
    for(int c = 0; c < 16 * 4; ++c){
        ppdu[offset++] = chip_i[d][c];
        ppdu[offset + SHIFT_IQ] = chip_q[d][c];
        ++offset;
    }
    d = (len_data & 0xf0) >> 0x4;
    for(int c = 0; c < 16 * 4; ++c){
        ppdu[offset++] = chip_i[d][c];
        ppdu[offset + SHIFT_IQ] = chip_q[d][c];
        ++offset;
    }
    // PSDU
    for(int s = 0; s < len_data; ++s){
        int d = (*mpdu_)[s];
        for(int c = 0; c < 16 * 4; ++c){
            ppdu[offset++] = chip_i[d][c];
            ppdu[offset + SHIFT_IQ] = chip_q[d][c];
            ++offset;
        }
    }

    tx_thread_data_->mutex.lock();
    tx_thread_data_->len_buffer = offset + SHIFT_IQ;
    tx_thread_data_->ptr_buffer = ppdu;
    tx_thread_data_->ready = true;
    tx_thread_data_->mutex.unlock();
    tx_thread_data_->cond_value.notify_one();

    int show_data = len_data == 0 ? 0 : offset_shr;
    callback->plot_test_callback(offset + SHIFT_IQ - show_data, ppdu + show_data);

    fprintf(stderr, "tx_ieee802_15_4::start= %d %d\n", LEN_MAX_SAMPLES * 2, offset_shr);
}
//--------------------------------------------------------------------------------------------------
