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
    for(int i = 0; i < LEN_MAX_SAMPLES * 2; ++i){
        tx_data[i] = 0;
    }
    offset_shr = 1;
    for(int c = 16 * 4 - 2; c < 16 * 4; ++c){
        tx_data[offset_shr++] = chip_q[0][c];
        offset_shr++;
    }
    offset_shr = 0;
    for(int z = 0; z < 8; ++z){
        for(int c = 0; c < 16 * 4; ++c){
            tx_data[offset_shr++] = chip_i[0][c];
            tx_data[offset_shr + SHIFT_IQ] = chip_q[0][c];
            ++offset_shr;
        }
    }
    for(int c = 0; c < 16 * 4; ++c){
        tx_data[offset_shr++] = chip_i[0x7][c];
        tx_data[offset_shr + SHIFT_IQ] = chip_q[0x7][c];
        ++offset_shr;
    }
//    tx_data[offset_shr - 2] = 2;
    for(int c = 0; c < 16 * 4; ++c){
        tx_data[offset_shr++] = chip_i[0xa][c];
        tx_data[offset_shr + SHIFT_IQ] = chip_q[0xa][c];
        ++offset_shr;
    }
}
//--------------------------------------------------------------------------------------------------
void tx_ieee802_15_4::start(tx_thread_data_t *tx_thread_data_)
{
    return;


    symbol_generate();

    callback->plot_test_callback(offset_shr + SHIFT_IQ, tx_data);

    tx_thread_data_->mutex.lock();
    tx_thread_data_->len_buffer = offset_shr + SHIFT_IQ;
    tx_thread_data_->ptr_buffer = tx_data;
    tx_thread_data_->ready = true;
    tx_thread_data_->mutex.unlock();
    tx_thread_data_->cond_value.notify_one();

    fprintf(stderr, "tx_ieee802_15_4::start= %d %d\n", LEN_MAX_SAMPLES * 2, offset_shr);
}
//--------------------------------------------------------------------------------------------------
