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
#ifndef RX_IEEE802_15_4_H
#define RX_IEEE802_15_4_H

#include <cmath>
#include <complex>
#include <ctime>

#include "ieee802_15_4.h"
#include "rx_mac_sublayer.h"
#include "utils/buffers.hh"

#include <QDebug>

typedef fifo_buffer<complex, LEN_PREAMBLE> fifo_buffer_256;

class rx_ieee802_15_4
{

public:
    explicit rx_ieee802_15_4();
    ~rx_ieee802_15_4();

    bool is_started = false;
    void connect_callback(ieee802_15_4_callback *cb_){callback = cb_;};
    void start(rx_thread_data_t *rx_thread_data_);
    rx_mac_sublayer *mac_layer;

private:
    void init_local();

    // matched filter
    std::vector<complex> iq_buffer;
    std::vector<complex> iq_data;
    complex buff_matched_filter[3];
    void half_sine_matched_filter(int len_, int16_t *i_buffer_, int16_t *q_buffer_);

    // preamble correlation
    float max_level = 0.0f;
    float level_thr = 0.0f;
    moving_average<float, LEN_CHIP / 2> avg_level;
    complex sum_double_correlation = {0.0f, 0.0f};
    complex max_double_correlation = {0.0f, 0.0f};
    int idx_preamble = 0;
    int idx_margin = 0;
    constexpr static int len_segment = 6 ; //6 should be <= 10
    constexpr static int num_segment = 4;  //4 should be <= (LEN_CHIP / len_segment - 1)
    constexpr static int len_local = len_segment * num_segment;
    constexpr static float k_tr = 1e5 * len_local * len_local;  //2e5 selected from experience
    fifo_buffer<complex, len_local> delay_seg_for_double_correlator;
    float local_zz_real[len_local];
    float local_zz_imag[len_local];
    float seg_real[num_segment];
    float seg_imag[num_segment];
    inline complex double_correlator(const complex* chip_);

    // frequency estimation
    fifo_buffer<complex, LEN_CHIP> delay_chip;
    complex cross_correlation = {0.0f, 0.0f};
    complex sum_cross_correlation = {0.0f, 0.0f};
    float coarse_fq_offset = 0.0f;
    float fine_fq_offset = 0.0f;
    float fq_offset = 0.0f;
    float phase_nco = 0.0f;
    float nco = 0.0f;
    complex nco_derotate = {1.0f, 0.0f};

    // sfd correlation
    float angle_offset = 0.0f;
    int coarse_timming_correction = 0;
    int idx_cor_z = 0;
    float cor_z[4];
    complex *conj_local_sfd;
    complex sfd_correlation[4] = {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};

    // interpolator
    float x2, x3, x1 = 0.0f;
    complex delay_data_1 = {0.0f, 0.0f}, delay_data_2 = {0.0f, 0.0f}, delay_data_3 = {0.0f, 0.0f};
    float mu = 0.0f;
    float i_delay_1 = 0.0f;
    float i_delay_2 = 0.0f;
    inline complex interpolator(const complex &data_, const float &mu_);

    // symbol detect
    int idx_chip = 0;
    int idx_symbol = 0;
    float i_correlation[16];
    float q_correlation[16];
    float imag_sum[16];
    float temp_sum_detect = 0;
    float *local_real;
    float *local_imag;
    float pre_angle_local_correlation = 0.0f;
    complex old_local_correlation = {0.0f, 0.0f};
    float sum_max = 0.0f;
    uint8_t symbol_detected = 0;

    mac_sublayer_data_t *mac_data;
    std::thread *mac_layer_thread;
    std::vector<uint8_t> v1_mpdu;
    std::vector<uint8_t> v2_mpdu;
    bool swap_mpdu;

    enum rx_state{
        detect_signal,
        detect_preamble,
        frequency_offset_estimation,
        frame_delimiter_phase_synchronisation,
        frame_delimiter_timing_synchronisation,
        frame_lenght,
        payload
    };
    rx_state state = detect_signal;
    rx_state state_type = frame_lenght;
    int idx_i = 0;
    int len_psdu = 0;
    void detection(int &in_len_, int16_t *iq_data_);
    void reset_detection();

    complex plot_data;
    fifo_buffer_256 preamble_correlation_fifo;
    std::complex<float> *preamble_correlation_buffer;
    int idx_sfd_corr_buf;
    std::complex<float> *sfd_correlation_buffer;
    int idx_sfd_sync_buf;
    std::complex<float> *sfd_synchronize_buffer;
    int idx_const_buf;
    std::complex<float> *constelation_buffer;

    // thread
    ieee802_15_4_callback *callback;
    void work(rx_thread_data_t *rx_thread_data_);
    void rx_data(int len_, int16_t *i_buffer_);
    void stop();

    // todo
    void tx_ptr_data(int16_t* ptr_, int len_);



};

#endif // RX_IEEE802_15_4_H
