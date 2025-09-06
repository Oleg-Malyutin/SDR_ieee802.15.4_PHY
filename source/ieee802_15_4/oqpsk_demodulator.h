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
#ifndef OQPSK_DEMODULATOR_H
#define OQPSK_DEMODULATOR_H

#include <cmath>
#include <complex>
#include <ctime>
#include <vector>

#include "rf_ieee802_15_4_constants.h"
#include "callback_layer.h"
#include "../device/callback_device.h"
#include "utils/buffers.hh"

#include <QDebug>

typedef std::complex<float> complex;

class oqpsk_demodulator
{

public:
    explicit oqpsk_demodulator();
    ~oqpsk_demodulator();

    bool is_started = false;
    void connect_device(device_callback *cb_){callback_device = cb_;};
    void connect_phy(phy_layer_callback *cb_){callback_phy = cb_;};
    void start(rx_thread_data_t *rx_thread_data_, rx_psdu_t *rx_sap_);

private:
    void init_local();

    enum rx_state{
        detect_signal,
        detect_preamble,
        frequency_offset_estimation,
        remove_margin,
        frame_delimiter_phase_synchronisation,
        frame_delimiter_timing_synchronisation,
        frame_lenght,
        payload
    };
    rx_state state = detect_signal;
    rx_state state_type = frame_lenght;
    int idx_i = 0;
    int len_symbols = 0;

    // preamble correlation
    float max_level = 0.0f;
    float level_thr = 0.0f;

    constexpr static int kk = 4;//4 selected from experience
    moving_average<double, LEN_CHIP * kk> avg_level;//4
    complex sum_double_correlation = {0.0f, 0.0f};
    complex max_double_correlation = {0.0f, 0.0f};
    int idx_preamble = 0;
    int idx_margin = 0;
    constexpr static int len_segment = 6 ; //6 should be <= 10
    constexpr static int num_segment = 4;  //4 should be <= (LEN_CHIP / len_segment - 1)
    constexpr static int len_local = len_segment * num_segment;
    fifo_buffer<complex, len_local> delay_seg_for_double_correlator;
    float local_zz_real[len_local];
    float local_zz_imag[len_local +  + OVERSAMPLE];
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
    inline void  interpolator_preset(const complex &data_);
    inline complex interpolator(const complex &data_, const float &mu_);

    // symbol detect
    int idx_chip = 0;
    int idx_symbol_frame_lenght = 0;
    int octet = 0;
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

    int idx_payload;
    uint8_t *v1_symbols;
    uint8_t *v2_symbols;
    bool swap_v_psdu;

    // thread
    phy_layer_callback *callback_phy;
    device_callback *callback_device;
    void work(rx_thread_data_t *rx_thread_data_, rx_psdu_t *rx_sap_);
    void stop();
    void demodulator(int &channel_, int &in_len_, std::complex<float> *iq_data_, rx_psdu_t *rx_sap_);
    void reset_demodulator();

    // plot
    complex plot_data;
    fifo_buffer<complex, LEN_PREAMBLE> preamble_correlation_fifo;
    std::complex<float> *preamble_correlation_buffer;
    std::complex<float> *sfd_correlation_buffer;
    int idx_sfd_sync_buf;
    std::complex<float> *sfd_synchronize_buffer;
    int idx_const_buf;
    std::complex<float> *constelation_buffer;

};

#endif // OQPSK_DEMODULATOR_H
