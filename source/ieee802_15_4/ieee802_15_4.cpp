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
#include "ieee802_15_4.h"

#include <iostream>
#include <cmath>
#include <x86intrin.h>

#define M_PI_X_2  (M_PI * 2.0)

//#define REITING

//--------------------------------------------------------------------------------------------------
ieee802_15_4::ieee802_15_4()
{
    preamble_correlation_buffer = new std::complex<float>[LEN_PREAMBLE];
    sfd_correlation_buffer = new std::complex<float>[5];
    sfd_synchronize_buffer = new std::complex<float>[LEN_BUFF_SFD];
    constelation_buffer = new std::complex<float>[MAX_BITS_PACKET];
    idx_const_buf = 0;

    preamble_correlation_fifo.reset();

    conj_local_sfd = new complex[LEN_CHIP * OVERSAMPLE + OVERSAMPLE];
    local_real = new float[16 * LEN_CHIP];
    local_imag = new float[16 * LEN_CHIP];

    mac_data = new mac_sublayer_data;
    mac_layer = new mac_sublayer;

    init_local();
}
//--------------------------------------------------------------------------------------------------
ieee802_15_4::~ieee802_15_4()
{
    qDebug() << "ieee802_15_4::~ieee802_15_4() start";
    stop();
    delete mac_data;
    delete mac_layer;
    delete[] preamble_correlation_buffer;
    delete[] sfd_correlation_buffer;
    delete[] sfd_synchronize_buffer;
    delete[] constelation_buffer;
    delete[] conj_local_sfd;
    delete[] local_real;
    delete[] local_imag;
    qDebug() << "ieee802_15_4::~ieee802_15_4() stop";
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::init_local()
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
void ieee802_15_4::start(rx_thread_data_t *rx_thread_data_)
{
    stop();
    is_started = true;
    swap_mpdu = false;
    mac_layer_thread = new std::thread(&mac_sublayer::start, mac_layer, mac_data);
    mac_layer_thread->detach();

    work(rx_thread_data_);
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::stop()
{
    if(is_started){
        mac_data->stop = true;
        mac_data->cond_value.notify_one();
        while(mac_data->is_started){
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        delete mac_layer_thread;
        is_started = false;
    }
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::work(rx_thread_data_t *rx_thread_data_)
{
    rx_thread_data_t *rx_thread_data = rx_thread_data_;
    std::unique_lock<std::mutex> read_lock(rx_thread_data->mutex);
    while (!rx_thread_data->stop){
        rx_thread_data->cond_value.wait(read_lock);
        if(rx_thread_data->ready){
            rx_thread_data->ready = false;
            rx_data(rx_thread_data->len_buffer, rx_thread_data->ptr_buffer);
        }
    }
    stop();
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::rx_data(int len_, int16_t *i_buffer_)
{
    detection(len_, i_buffer_);
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::detection(int &in_len_, int16_t *iq_data_)
{
    int in_len = in_len_;
    int16_t *iq_data = iq_data_;
    complex data;
    complex *data_delay_seg;
    complex out_double_correlator;
    float level;
    float angle_local_correlation;
    std::vector<uint8_t> *p_mpdu;
    if(swap_mpdu){
        p_mpdu = &v2_mpdu;
    }
    else{
        p_mpdu = &v1_mpdu;
    }
#ifdef REITING
auto start_time = std::chrono::steady_clock::now();
#endif
    for(int i = 0; i < in_len; ++i){

        data.real(iq_data[i * 2]);
        data.imag(iq_data_[i * 2 + 1]);

        switch (state){

        case detect_signal:

            data_delay_seg = delay_seg_for_double_correlator.buffer(data);
            out_double_correlator = double_correlator(data_delay_seg);
            level = norm(out_double_correlator);
            level_thr =  k_tr * avg_level(norm(data));
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

                        state = frequency_offset_estimation;

                        break;

                    }
                }
                else{
                    reset_detection();

                    break;

                }
            }
            if(idx_i > (LEN_CHIP + 1)){
                reset_detection();

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
            idx_sfd_sync_buf = 0;

            state = frame_delimiter_phase_synchronisation;

            break;

        case frame_delimiter_phase_synchronisation:

            if(--idx_margin > 0) {

                break;

            }
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

            }
            plot_data = {data.real(), conj_local_sfd[idx_i].real() * 500};
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
                plot_data = {data.real(), conj_local_sfd[idx_i].real() * 500};
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
                plot_data = {data.real(), conj_local_sfd[idx_i].real() * 500};
                sfd_synchronize_buffer[idx_sfd_sync_buf++] = plot_data;
            }
            else if(idx_i < (LEN_CHIP * 2)){
                plot_data = {data.real(), conj_local_sfd[idx_i].real() * 500};
                sfd_synchronize_buffer[idx_sfd_sync_buf++] = plot_data;
            }
            else if(idx_i < (LEN_CHIP * 2 + 1)){
                data = interpolator(data, mu);
            }
            else{
                idx_i = 0;
                data = interpolator(data, mu);
                // clear
                for(int symbol = 0; symbol < 16; ++symbol){
                    i_correlation[symbol] = 0;
                    q_correlation[symbol] = 0;
                    imag_sum[symbol] = 0;
                }

                state = frame_lenght;

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
                i_correlation[symbol] += data.real() * local_real[symbol * LEN_CHIP + idx_chip];
                q_correlation[symbol] += data.imag() * local_imag[symbol * LEN_CHIP + idx_chip];
                imag_sum[symbol] += data.imag() * local_real[symbol * LEN_CHIP + idx_chip] -
                                    data.real() * local_imag[symbol * LEN_CHIP + idx_chip];
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
            // need two symbol for byte
            if(++idx_symbol == 2){
                idx_symbol = 0;
                symbol_detected &= 0x7;
                symbol_detected <<= 0x4;
                fq_offset += (angle_local_correlation - pre_angle_local_correlation);
                idx_const_buf = 0;

                state = payload;

            }
            pre_angle_local_correlation = angle_local_correlation;
            len_psdu += symbol_detected;

            break;

        case payload:

            phase_nco += fq_offset;
            nco_derotate = {cosf(phase_nco + angle_offset), sinf(phase_nco + angle_offset)};
            data *= conj(nco_derotate);
            data = interpolator(data, mu);
            for(int symbol = 0; symbol < 16; ++symbol){
                i_correlation[symbol] += data.real() * local_real[symbol * LEN_CHIP + idx_chip];
                q_correlation[symbol] += data.imag() * local_imag[symbol * LEN_CHIP + idx_chip];
                imag_sum[symbol] += data.imag() * local_real[symbol * LEN_CHIP + idx_chip] -
                                    data.real() * local_imag[symbol * LEN_CHIP + idx_chip];
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
            p_mpdu->push_back(symbol_detected);
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

            if(idx_i == (len_psdu * LEN_CHIP * 2)){

                if (mac_data->mutex.try_lock()){
                    mac_data->mpdu =  p_mpdu;
                    mac_data->ready = true;
                    mac_data->mutex.unlock();
                    mac_data->cond_value.notify_one();
                    if(swap_mpdu){
                        v1_mpdu.clear();
                    }
                    else{
                        v2_mpdu.clear();
                    }
                    swap_mpdu = !swap_mpdu;
                }
                else{
                    fprintf(stderr, " mac_data : skip buffer\n");
                }
                complex *ptr_preamble_correlation = preamble_correlation_fifo.buffer();
                for(int i = 0; i < LEN_PREAMBLE; ++i){
                    preamble_correlation_buffer[i] = ptr_preamble_correlation[i];
                }
                callback->plot_callback(LEN_PREAMBLE, preamble_correlation_buffer,
                                      5, sfd_correlation_buffer,
                                      LEN_BUFF_SFD, sfd_synchronize_buffer,
                                      idx_const_buf, constelation_buffer);
                preamble_correlation_fifo.reset();
//                qDebug() << "idx_i=" << idx_i
//                         << "len_psdu="
//                         << len_psdu << "reset------------------";
                reset_detection();

            }
            else if(idx_i > MAX_BITS_PACKET){
//                qDebug() << "idx_i > MAX_BITS_PACKET" << idx_i
//                         << "len_psdu="
//                         << len_psdu << "------------------error!";
                reset_detection();
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
complex ieee802_15_4::double_correlator(const complex *chip_)
{
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
complex ieee802_15_4::interpolator(const complex &data_, const float &mu_)
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
void ieee802_15_4::reset_detection()
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

    len_psdu = 0;
    state_type = frame_lenght;
    state = detect_signal;
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::half_sine_matched_filter(int len_, int16_t *i_buffer_, int16_t *q_buffer_)
{
    if(iq_buffer.size() != (ulong)len_){
        iq_buffer.resize(len_);
        iq_data.resize(len_);
    }
    for(int i = 0; i < len_; ++i){
        float v1 = i_buffer_[i * 2];
        float v2 = i_buffer_[i * 2 + 1];
        iq_buffer[i].real(v1);
        iq_buffer[i].imag(v2);

    }

    complex sum0, sum1, sum2, sum3;

    sum0 = h_matched_filter[0] * buff_matched_filter[0];
    sum1 = h_matched_filter[1] * buff_matched_filter[1];
    sum2 = h_matched_filter[2] * buff_matched_filter[2];
    sum3 = h_matched_filter[3] * iq_buffer[0];
    iq_data[0] = sum0 + sum1 + sum2 + sum3;

    sum0 = h_matched_filter[0] * buff_matched_filter[1];
    sum1 = h_matched_filter[1] * buff_matched_filter[2];
    sum2 = h_matched_filter[2] * iq_buffer[0];
    sum3 = h_matched_filter[3] * iq_buffer[1];
    iq_data[1] = sum0 + sum1 + sum2 + sum3;

    sum0 = h_matched_filter[0] * buff_matched_filter[2];
    sum1 = h_matched_filter[1] * iq_buffer[0];
    sum2 = h_matched_filter[2] * iq_buffer[1];
    sum3 = h_matched_filter[3] * iq_buffer[2];
    iq_data[2] = sum0 + sum1 + sum2 + sum3;

    for(int i = 0; i < len_ - 3; ++i){
        sum0 = h_matched_filter[0] * iq_buffer[i];
        sum1 = h_matched_filter[1] * iq_buffer[i + 1];
        sum2 = h_matched_filter[2] * iq_buffer[i + 2];
        sum3 = h_matched_filter[3] * iq_buffer[i + 3];

        iq_data[i + 3] = sum0 + sum1 + sum2 + sum3;
    }

    buff_matched_filter[0] = iq_buffer[len_ - 3];
    buff_matched_filter[1] = iq_buffer[len_ - 2];
    buff_matched_filter[2] = iq_buffer[len_ - 1];
}
//--------------------------------------------------------------------------------------------------
void ieee802_15_4::tx_ptr_data(int16_t* ptr_, int len_)
{

}
//--------------------------------------------------------------------------------------------------
