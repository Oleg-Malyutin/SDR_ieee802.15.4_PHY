#include "oqpsk_modulator.h"

#include <iostream>

//--------------------------------------------------------------------------------------------------
oqpsk_modulator::oqpsk_modulator()
{
    chip_i = new float* [LEN_PN_SEQUENCE / 2];
    chip_q = new float* [LEN_PN_SEQUENCE / 2];
    int len = LEN_PN_SEQUENCE * OVERSAMPLE * 2;
    for(int c = 0; c < 16; ++c){
        chip_i[c] = new float[len];
        chip_q[c] = new float[len];
    }
    ppdu_samples = new float[LEN_MAX_SAMPLES * 2];
}
//--------------------------------------------------------------------------------------------------
oqpsk_modulator::~oqpsk_modulator()
{
    fprintf(stderr, "oqpsk_modulator::~oqpsk_modulator() start\n");
    for(int c = 0; c < 16; ++c){
        delete[] chip_i[c];
        delete[] chip_q[c];
    }
    delete[] chip_i;
    delete[] chip_q;
    delete[] ppdu_samples;
    fprintf(stderr, "oqpsk_modulator::~oqpsk_modulator() stop\n");
}
//--------------------------------------------------------------------------------------------------
void oqpsk_modulator::symbol_generate()
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
        ppdu_samples[i] = 0;
    }
    offset_shr = 1;
    for(int c = 16 * 4 - 2; c < 16 * 4; ++c){
        ppdu_samples[offset_shr++] = chip_q[0][c];
        offset_shr++;
    }
    offset_shr = 0;
    for(int z = 0; z < 8; ++z){
        for(int c = 0; c < 16 * 4; ++c){
            ppdu_samples[offset_shr++] = chip_i[0][c];
            ppdu_samples[offset_shr + SHIFT_IQ] = chip_q[0][c];
            ++offset_shr;
        }
    }
    // Start of Frame Delimiter
    for(int c = 0; c < 16 * 4; ++c){
        ppdu_samples[offset_shr++] = chip_i[0x7][c];
        ppdu_samples[offset_shr + SHIFT_IQ] = chip_q[0x7][c];
        ++offset_shr;
    }
//    tx_data[offset_shr - 2] = 2;
    for(int c = 0; c < 16 * 4; ++c){
        ppdu_samples[offset_shr++] = chip_i[0xa][c];
        ppdu_samples[offset_shr + SHIFT_IQ] = chip_q[0xa][c];
        ++offset_shr;
    }
}
//--------------------------------------------------------------------------------------------------
void oqpsk_modulator::start(rf_sap_t *tx_sap_)
{
    is_started = true;

    symbol_generate();

    work(tx_sap_);
}
//--------------------------------------------------------------------------------------------------
void oqpsk_modulator::work(rf_sap_t *tx_sap_)
{
    std::unique_lock<std::mutex> read_lock(tx_sap_->mutex);
    while (!tx_sap_->stop){
        tx_sap_->cond_value.wait(read_lock);
        if(tx_sap_->ready){
            tx_sap_->ready = false;
            tx_sap_->status = tx_data(tx_sap_->symbols);
            tx_sap_->ready_confirm = true;
            tx_sap_->cond_confirm.notify_all();
        }
    }
    is_started = false;
    fprintf(stderr, "oqpsk_modulator::work() finish\n");
}
//--------------------------------------------------------------------------------------------------
status_t oqpsk_modulator::tx_data(std::vector<uint8_t> *psdu_symbols_)
{
//    fprintf(stderr, "oqpsk_modulator::tx_data  \n");

    status_t status = SUCCESS;
    std::vector<uint8_t> *psdu_symbols = psdu_symbols_;
    uint16_t len_symbols = psdu_symbols->size();

    if(len_symbols > MAX_SYMBOLS_PAYLOAD){
        fprintf(stderr,
                "oqpsk_modulator::tx_data() error: \
                 data size(symbols) is greater than 2 * MaxPHYPacketSize(127)\n");

        status = INVALID_PARAMETER;

        return status;

    }

    int offset = offset_shr;
    // PHR add payload length
    int d = len_symbols & 0xf;
    for(int c = 0; c < 16 * 4; ++c){
        ppdu_samples[offset++] = chip_i[d][c];
        ppdu_samples[offset + SHIFT_IQ] = chip_q[d][c];
        ++offset;
    }
    d = (len_symbols & 0xf0) >> 0x4;
    for(int c = 0; c < 16 * 4; ++c){
        ppdu_samples[offset++] = chip_i[d][c];
        ppdu_samples[offset + SHIFT_IQ] = chip_q[d][c];
        ++offset;
    }
    // PSDU symbols to samples
    for(int octet = 0; octet < len_symbols; ++octet){
        for(int symbol = 0; symbol < 2; ++symbol){
            int d = ((*psdu_symbols)[octet] >> 4 * symbol) & 0xf;
            for(int c = 0; c < 16 * 4; ++c){
                ppdu_samples[offset++] = chip_i[d][c];
                ppdu_samples[offset + SHIFT_IQ] = chip_q[d][c];
                ++offset;
            }
        }
    }

    uint len_samples = offset + SHIFT_IQ;

    callback_tx_data->tx_data(len_samples, ppdu_samples);

    if(len_symbols > 0){

        callback_device->plot_test_callback(len_samples - offset_shr, ppdu_samples + offset_shr);

    }

    return status;

    fprintf(stderr, "oqpsk_modulator::tx_data len=%d\n", offset + SHIFT_IQ);
}
//--------------------------------------------------------------------------------------------------
