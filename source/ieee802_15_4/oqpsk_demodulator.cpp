/*
 *  Copyright 2025 Oleg Malyutin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "oqpsk_demodulator.h"

#include <iostream>
#include <cmath>
#if !(defined(WIN32) || defined(WIN64))
#include <x86intrin.h>
#else
#include <intrin.h>
#endif

#define M_PI_X_2  (M_PI * 2.0)

//#define REITING

//--------------------------------------------------------------------------------------------------
oqpsk_demodulator::oqpsk_demodulator()
{
    preamble_correlation_buffer = new std::complex<float>[LEN_PREAMBLE];
    sfd_correlation_buffer = new std::complex<float>[5];
    sfd_synchronize_buffer = new std::complex<float>[LEN_SFD];
    constelation_buffer = new std::complex<float>[MAX_SAMPLES_PAYLOAD];

    conj_local_sfd = new complex[LEN_CHIP * OVERSAMPLE + SHIFT_IQ / 2];
    local_real = new float[16 * LEN_CHIP];
    local_imag = new float[16 * LEN_CHIP];

    v1_symbols = new uint8_t[MAX_SYMBOLS_PAYLOAD];
    v2_symbols = new uint8_t[MAX_SYMBOLS_PAYLOAD];

    init_local();

    reset_demodulator();


}
//--------------------------------------------------------------------------------------------------
oqpsk_demodulator::~oqpsk_demodulator()
{
    fprintf(stderr, "oqpsk_demodulator::~oqpsk_demodulator() start\n");
    stop();
    delete[] preamble_correlation_buffer;
    delete[] sfd_correlation_buffer;
    delete[] sfd_synchronize_buffer;
    delete[] constelation_buffer;
    delete[] conj_local_sfd;
    delete[] local_real;
    delete[] local_imag;
    delete[] v1_symbols;
    delete[] v2_symbols;
    fprintf(stderr, "oqpsk_demodulator::~oqpsk_demodulator() stop\n");
}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::init_local()
{       
    for(int i = 0; i < 16; ++i){
        for(int j = 0; j < 16; ++j){
            for(int h = 0; h < 4; ++h){
                int k = i * 16 * 4 + j * 4 + h;
                local_real[k] = chip_table[i][j * 2] * h_matched_filter[h];
                local_imag[k] = chip_table[i][j * 2 + 1]  * h_matched_filter[h];
            }
        }
    }
    local_zz_imag[0]= local_imag[LEN_CHIP - 2];
    local_zz_imag[1]= local_imag[LEN_CHIP - 1];
    for(int i = 0; i < len_local; ++i){
        local_zz_real[i] = local_real[i];
        local_zz_imag[i + 2]= local_imag[i];
    }

    conj_local_sfd[0].imag(-local_imag[LEN_CHIP - 2]);
    conj_local_sfd[1].imag(-local_imag[LEN_CHIP - 1]);
    int num_chip = 7;
    int j = 0;
    for(int i = 0; i < LEN_CHIP * 2; ++i){
        if(i == LEN_CHIP) {
            j = 0;
            num_chip = 10;
        }
        conj_local_sfd[i].real(local_real[LEN_CHIP * num_chip + j]);
        conj_local_sfd[i + 2].imag(-local_imag[LEN_CHIP * num_chip + j]);
        ++j;
    }
}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::start(rx_thread_data_t *rx_thread_data_, rx_sap_t *rx_sap_)
{
    stop();
    is_started = true;
    swap_v_psdu = false;
    rx_thread_data = rx_thread_data_;
    rx_sap = rx_sap_;

    work();
}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::stop()
{
    reset_demodulator();
    if(is_started){
        is_started = false;
    }
}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::work()
{
    std::unique_lock<std::mutex> read_lock(rx_thread_data->mutex);
    while (!rx_thread_data->stop_demodulator){
        rx_thread_data->condition.wait(read_lock);
        if(rx_thread_data->ready){
            rx_thread_data->ready = false;
            demodulator();
            rx_sap->rssi_mutex.lock();
            rx_sap->rssi_value.store(rx_thread_data->rssi);
            rx_sap->rssi_ready.store(true);
            rx_sap->rssi_mutex.unlock();
            rx_sap->rssi_condition.notify_all();
        }
    }
    stop();
    callback_device->error_callback(rx_thread_data->status);
    fprintf(stderr, "oqpsk_demodulator::work finish\n");
}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::demodulator()
{
//    fprintf(stderr, "ed %f\n", energy_detect_);

    int channel = rx_thread_data->channel;
    int in_len = rx_thread_data->len_buffer;
    std::complex<float> *iq_data = rx_thread_data->ptr_buffer;
    float energy_signal;
    complex *data_delay_seg;
    complex out_double_correlator;
    double level;
    float angle_local_correlation;
    uint8_t *ptr_symbols;
    if(swap_v_psdu){
        ptr_symbols = v2_symbols;
    }
    else{
        ptr_symbols = v1_symbols;
    }
#ifdef REITING
auto start_time = std::chrono::steady_clock::now();
#endif

    for(int i = 0; i < in_len; ++i){

        complex data = iq_data[i];
        switch (state){

        case detect_signal:

            data_delay_seg = delay_seg_for_double_correlator.buffer(data);
            out_double_correlator = double_correlator(data_delay_seg);
            level = norm(out_double_correlator);

            energy_signal = avg_level(level);

            level_thr =  energy_signal / 7.5f;//7.5 selected from experience
            if(level > level_thr){
                delay_chip.data(data);
                max_level = level;
            }
            else if(level < max_level){
                delay_chip.data(data);
                max_level = 0.0f;
                idx_preamble = 1;
                idx_i = 0;
            }
            else if(idx_preamble == 1){
                delay_chip.data(data);
                // wait for the average level to saturate
                if(++idx_i == (LEN_CHIP - 12)){
                    sum_double_correlation = {0.0f, 0.0f};
                    cross_correlation = {0.0f, 0.0f};
                    sum_cross_correlation = {0.0f, 0.0f};

                    state = detect_preamble;

                }
            }
            plot_data.real(level);
            plot_data.imag(level_thr);
            preamble_correlation_fifo.data(plot_data);

            break;

        case detect_preamble:

            data_delay_seg = delay_seg_for_double_correlator.buffer(data);
            out_double_correlator = double_correlator(data_delay_seg);
            level = norm(out_double_correlator);
            ++idx_i;
            if(level > level_thr){
                max_level = level;
                max_double_correlation = out_double_correlator;
            }
            else if(level < max_level){
                max_level = 0.0;
                if(idx_i == LEN_CHIP || idx_i == (LEN_CHIP - 1) || idx_i == (LEN_CHIP + 1)){
                    idx_i = 0;
                    ++idx_preamble;
                    sum_double_correlation += max_double_correlation;
                    sum_cross_correlation += cross_correlation;
                    cross_correlation = {0.0f, 0.0f};
                    if(idx_preamble == 8){
                        idx_preamble = 0;

                        rx_sap->is_signal = true;

                        state = frequency_offset_estimation;

                        break;

                    }
                }
                else{
                    rx_sap->is_signal = false;
                    reset_demodulator();

                    break;

                }
            }
            if(idx_i > (LEN_CHIP + 1)){
                rx_sap->is_signal = false;
                reset_demodulator();

                break;

            }
            else if(idx_preamble > 2){
                cross_correlation += data * conj(delay_chip.data(data));
            }
            plot_data.real(level);
            preamble_correlation_fifo.data(plot_data);

            break;

        case frequency_offset_estimation:

            fq_offset = arg(sum_double_correlation) / (float)len_segment;
            sum_double_correlation = {0.0, 0.0};
            fine_fq_offset = arg(sum_cross_correlation) / (float)(LEN_CHIP);
            sum_cross_correlation = {0.0f, 0.0f};
            coarse_fq_offset = 0;
            if(fq_offset > INTEGER_CFO / 2.0f){
                if(fine_fq_offset < 0.0f){
                    coarse_fq_offset = INTEGER_CFO * 2.0f;
                }
                else if(fq_offset >= INTEGER_CFO * 2.0f){
                    coarse_fq_offset = INTEGER_CFO * 2.0f;
                }
            }
            else if(fq_offset < -INTEGER_CFO / 2.0f){
                if(fine_fq_offset > 0.0f){
                    coarse_fq_offset = -INTEGER_CFO * 2.0f;
                }
                else if(fq_offset <= INTEGER_CFO * 2.0f){
                    coarse_fq_offset = -INTEGER_CFO * 2.0f;
                }
            }
            fq_offset = coarse_fq_offset + fine_fq_offset;
            idx_margin = LEN_CHIP - len_segment * num_segment - 1;

            state = remove_margin;

            break;

        case remove_margin:

            if(--idx_margin == 1) {

                state = frame_delimiter_phase_synchronisation;

            }

            break;

        case frame_delimiter_phase_synchronisation:

            phase_nco += fq_offset;
            nco_derotate = {cosf(phase_nco), sinf(phase_nco)};
            data *= conj(nco_derotate);
            sfd_correlation[0] += data * conj_local_sfd[idx_i];
            sfd_correlation[1] += data * conj_local_sfd[idx_i + 1];
            sfd_correlation[2] += data * conj_local_sfd[idx_i + 2];
            if(idx_i == LEN_CHIP - 1){
                float max_pwr = 0;
                complex max_correlation = {0.0f, 0.0f};
                for(int i = 0; i < 3; ++i){
                    float pwr = norm(sfd_correlation[i]);
                    if(pwr > max_pwr){
                        max_correlation = sfd_correlation[i];
                        max_pwr = pwr;
                        coarse_timming_correction = 1 - i;
                    }
                    sfd_correlation[i] = {0.0, 0.0};
                }
                angle_offset = arg(max_correlation);
                idx_i -= coarse_timming_correction;

                state = frame_delimiter_timing_synchronisation;

//                complex *ptr_preamble_correlation = preamble_correlation_fifo.buffer();
//                for(int i = 0; i < LEN_PREAMBLE; ++i){
//                    preamble_correlation_buffer[i] = ptr_preamble_correlation[i];
//                }
                callback_device->preamble_correlation_callback(LEN_PREAMBLE, preamble_correlation_fifo.buffer());


            }
            plot_data = {data.real(), conj_local_sfd[idx_i].real()};
            sfd_synchronize_buffer[idx_sfd_sync_buf++] = plot_data;
            ++idx_i;

            break;

        case frame_delimiter_timing_synchronisation:

            phase_nco += fq_offset;
            nco_derotate = {cosf(phase_nco + angle_offset), sinf(phase_nco + angle_offset)};
            data *= conj(nco_derotate);
            if(idx_i < (LEN_CHIP * 2 - 5)){
                sfd_correlation[0] += data * conj_local_sfd[idx_i - 1];
                sfd_correlation[1] += data * conj_local_sfd[idx_i + 1];
                sfd_correlation[2] += data * conj_local_sfd[idx_i + 3];
                sfd_correlation[3] += data * conj_local_sfd[idx_i + 5];
                plot_data = {data.real(), conj_local_sfd[idx_i].real()};
                sfd_synchronize_buffer[idx_sfd_sync_buf++] = plot_data;
            }
            else if(idx_i == (LEN_CHIP * 2 - 5)){
                sfd_correlation_buffer[0] = {0.0f, 0.0f};
                for(int i = 0; i < 4; ++i){
                    cor_z[i] = norm(sfd_correlation[i]);
                    sfd_correlation[i] = {0.0f, 0.0f};
                    sfd_correlation_buffer[i + 1] = {cor_z[i], 0.0f};
                }
                mu = atan2f(cor_z[0] - cor_z[2], cor_z[1] - cor_z[3]) *  2.0f / M_PI_2;
                plot_data = {data.real(), conj_local_sfd[idx_i].real()};
                sfd_synchronize_buffer[idx_sfd_sync_buf++] = plot_data;
            }
            else if(idx_i < (LEN_CHIP * 2)){
                plot_data = {data.real(), conj_local_sfd[idx_i].real()};
                sfd_synchronize_buffer[idx_sfd_sync_buf++] = plot_data;
                interpolator_preset(data);
            }
            else if(idx_i < (LEN_CHIP * 2 + 1)){
                interpolator_preset(data);
            }
            else{
                idx_i = 0;
                interpolator_preset(data);
                // clear
                for(int symbol = 0; symbol < 16; ++symbol){
                    i_correlation[symbol] = 0;
                    q_correlation[symbol] = 0;
                    imag_sum[symbol] = 0;
                }

                state = frame_lenght;

                callback_device->sfd_callback(5, sfd_correlation_buffer, LEN_SFD, sfd_synchronize_buffer);

                break;

            }
            ++idx_i;

            break;

        case frame_lenght:

            phase_nco += fq_offset;
            nco_derotate = {cosf(phase_nco + angle_offset), sinf(phase_nco + angle_offset)};
            data *= conj(nco_derotate);
            data = interpolator(data, mu);
            for(int symbol = 0; symbol < 16; ++symbol){
                const int idx_local = symbol * LEN_CHIP + idx_chip;
                i_correlation[symbol] += data.real() * local_real[idx_local];
                q_correlation[symbol] += data.imag() * local_imag[idx_local];
                imag_sum[symbol] += data.imag() * local_real[idx_local] - data.real() * local_imag[idx_local];
            }
            if(++idx_chip < LEN_CHIP){

                break;

            }
            idx_chip = 0;
            sum_max = 0;
            symbol_detected = 0;
            for(int symbol = 0; symbol < 16; ++symbol){
                float sum = abs(i_correlation[symbol]) + abs(q_correlation[symbol]);
                if(sum > sum_max){
                    sum_max = sum;
                    symbol_detected = symbol;
                }
            }
            symbol_detected %= 8;
            if(q_correlation[symbol_detected] < 0.0f){
               symbol_detected += 8;
            }
            angle_local_correlation = atan2f(imag_sum[symbol_detected],
                                             i_correlation[symbol_detected] +
                                             q_correlation[symbol_detected]) / LEN_CHIP;
            // clear
            for(int symbol = 0; symbol < 16; ++symbol){
                i_correlation[symbol] = 0;
                q_correlation[symbol] = 0;
                imag_sum[symbol] = 0;
            }
            angle_offset += angle_local_correlation;
            // need two symbols for octet
            if(++idx_symbol_frame_lenght == 2){
                symbol_detected &= 0x7;
                symbol_detected <<= 0x4;
                fq_offset += (angle_local_correlation - pre_angle_local_correlation);
                idx_const_buf = 0;
                idx_payload = 0;

                state = payload;

            }
            pre_angle_local_correlation = angle_local_correlation;
            len_symbols += symbol_detected;
            // check frame lenght
            if(state == payload){
                if(len_symbols > MaxPHYPacketSize * 2 || len_symbols < 5){
                    fprintf(stderr, "oqpsk_demodulator::demodulator out len_symbols %d\n", len_symbols);

                    rx_sap->is_signal = false;
                    reset_demodulator();

                }
                else{
                    len_symbols *= 2;
                }
            }

            break;

        case payload:

            phase_nco += fq_offset;
            nco_derotate = {cosf(phase_nco + angle_offset), sinf(phase_nco + angle_offset)};
            data *= conj(nco_derotate);
            data = interpolator(data, mu);
            for(int symbol = 0; symbol < 16; ++symbol){
                const int idx_local = symbol * LEN_CHIP + idx_chip;
                i_correlation[symbol] += data.real() * local_real[idx_local];
                q_correlation[symbol] += data.imag() * local_imag[idx_local];
                imag_sum[symbol] += data.imag() * local_real[idx_local] - data.real() * local_imag[idx_local];
            }
            //show_constelation__
            if((idx_i + 2) % 4 == 0){
                constelation_buffer[idx_const_buf++] = data;
            }
            //___________________
            ++idx_i;
            if(++idx_chip < LEN_CHIP){

                break;

            }
            idx_chip = 0;
            sum_max = 0;
            symbol_detected = 0;
            for(int symbol = 0; symbol < 16; ++symbol){
                float sum = abs(i_correlation[symbol]) + abs(q_correlation[symbol]);
                if(sum > sum_max){
                    sum_max = sum;
                    symbol_detected = symbol;
                }
            }
            symbol_detected %= 8;
            if(q_correlation[symbol_detected] < 0.0f){
               symbol_detected += 8;
            }

            ptr_symbols[idx_payload++] = symbol_detected;

            angle_local_correlation = atan2f(imag_sum[symbol_detected],
                                             i_correlation[symbol_detected] +
                                             q_correlation[symbol_detected]) / LEN_CHIP;
            // clear
            for(int symbol = 0; symbol < 16; ++symbol){
                i_correlation[symbol] = 0;
                q_correlation[symbol] = 0;
                imag_sum[symbol] = 0;
            }
            angle_offset += angle_local_correlation;
            fq_offset += angle_local_correlation - pre_angle_local_correlation;
            pre_angle_local_correlation = angle_local_correlation;

            if(idx_payload == len_symbols) {

                rx_sap->mutex.lock();

                rx_sap->channel = channel;
                rx_sap->len_symbols = len_symbols;
                rx_sap->symbols =  ptr_symbols;
                rx_sap->ready.store(true);

                rx_sap->mutex.unlock();
                rx_sap->condition.notify_all();

                swap_v_psdu = !swap_v_psdu;
                if(swap_v_psdu){
                    ptr_symbols = v2_symbols;
                }
                else{
                    ptr_symbols = v1_symbols;
                }

                callback_device->constelation_callback(idx_const_buf, constelation_buffer);

                rx_sap->is_signal = false;
                reset_demodulator();
            }

            break;

        }

    }

#ifdef REITING
auto end_time = std::chrono::steady_clock::now();
auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
if(elapsed_time > 1024){
    std::cout << "all: " << elapsed_time - 1024 << " us" << std::endl;
}
#endif

}
//--------------------------------------------------------------------------------------------------
complex oqpsk_demodulator::double_correlator(const complex *chip_)
{
    float seg_real[num_segment];
    float seg_imag[num_segment];
    for(int j = 0; j < num_segment; ++j){
        int n = j * len_segment;
        seg_real[j] = 0.0f;
        seg_imag[j] = 0.0f;
        for(int i = n; i < n + len_segment; ++i){
            seg_real[j] += chip_[i].real() * local_zz_real[i] + chip_[i].imag() * local_zz_imag[i];
            seg_imag[j] += chip_[i].imag() * local_zz_real[i] - chip_[i].real() * local_zz_imag[i];
        }
    }
    float sum_real = 0.0f;
    float sum_imag = 0.0f;
    for(int i = 1; i < num_segment; ++i){
        int j = i - 1;
        sum_real += seg_real[i] * seg_real[j] + seg_imag[i] * seg_imag[j];
        sum_imag += seg_imag[i] * seg_real[j] - seg_real[i] * seg_imag[j];
    }

    return {sum_real, sum_imag};

}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::interpolator_preset(const complex &data_)
{
    delay_data_3 = delay_data_2;
    delay_data_2 = delay_data_1;
    delay_data_1 = data_;
}
//--------------------------------------------------------------------------------------------------
complex oqpsk_demodulator::interpolator(const complex &data_, const float &mu_)
{
    // 4-point, 3rd-order Hermite (x-form)
    delay_data_3 = delay_data_2;
    delay_data_2 = delay_data_1;
    delay_data_1 = data_;
    complex c0 = delay_data_2;
    complex c1 = 0.5f * (delay_data_1 - delay_data_3);
    complex c2 = delay_data_3 - 2.5f * delay_data_2 + 2.0f * delay_data_1 - 0.5f * data_;
    complex c3 = 0.5f * (data_ - delay_data_3) + 1.5f * (delay_data_2 - delay_data_1);
    x1 = mu_;
    x2 = x1 * x1;
    x3 = x2 * x1;
    complex x = c3 * x3 + c2 * x2 + c1 * x1 + c0;
    complex out = {i_delay_2, x.imag()};
    i_delay_2 = i_delay_1;
    i_delay_1 = x.real();

    return out;

}
//--------------------------------------------------------------------------------------------------
void oqpsk_demodulator::reset_demodulator()
{
    delay_seg_for_double_correlator.reset();
    sum_double_correlation = {0.0f, 0.0f};
    cross_correlation = {0.0f, 0.0f};
    sum_cross_correlation = {0.0f, 0.0f};

    idx_i = 0;
    idx_preamble = 0;
    level_thr = 0.0f;
    phase_nco = 0.0f;
    fq_offset = 0.0f;
    angle_offset = 0.0f;
    coarse_timming_correction = 0;
    x1 = 0.0f;
    mu = 0.0f;

    idx_sfd_sync_buf = 0;
    len_symbols = 0;
    idx_symbol_frame_lenght = 0;
    state_type = frame_lenght;
    state = detect_signal;

    preamble_correlation_fifo.reset();
}
//--------------------------------------------------------------------------------------------------

