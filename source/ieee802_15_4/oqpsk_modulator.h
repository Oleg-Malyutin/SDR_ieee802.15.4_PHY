#ifndef OQPSK_MODULATOR_H
#define OQPSK_MODULATOR_H

#include "rf_ieee802_15_4_constants.h"
#include "callback_layer.h"
#include "phy_layer/phy_types.h"
#include "../device/callback_device.h"

class oqpsk_modulator
{
public:
    explicit oqpsk_modulator();
    ~oqpsk_modulator();

    bool is_started = false;
    void connect_tx(rf_tx_callback *cb_){callback_tx_data = cb_;};
    void connect_device(device_callback *cb_){callback_device = cb_;};
    void connect_phy(phy_layer_callback *cb_){callback_phy = cb_;};
    void start(rf_sap_t *tx_sap_);

private:
    float **chip_i;
    float **chip_q;
    int offset_shr;
    float *ppdu_samples;
    void symbol_generate();
    rf_tx_callback *callback_tx_data;
    device_callback *callback_device;
    phy_layer_callback *callback_phy;
    void work(rf_sap_t *tx_sap_);
    status_t tx_data(std::vector<uint8_t> *psdu_symbols_);

};

#endif // OQPSK_MODULATOR_H
