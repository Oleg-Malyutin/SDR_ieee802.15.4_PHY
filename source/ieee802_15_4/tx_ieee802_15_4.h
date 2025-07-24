#ifndef TX_IEEE802_15_4_H
#define TX_IEEE802_15_4_H

#include "ieee802_15_4.h"
#include "tx_mac_sublayer.h"
#include "mac_sublayer.h"

class tx_ieee802_15_4
{
public:
    tx_ieee802_15_4();
    ~tx_ieee802_15_4();

    bool is_started = false;
    void connect_callback(ieee802_15_4_callback *cb_){callback = cb_;};
    void start(tx_thread_data_t *tx_thread_data_);
    tx_mac_sublayer *mac_sublayer;

private:
    float **chip_i;
    float **chip_q;
    int offset_shr;
    float ppdu[LEN_MAX_SAMPLES * 2];
    void symbol_generate();
    ieee802_15_4_callback *callback;
    void work(tx_thread_data_t *tx_thread_data_, mac_sublayer_data *mac_data_);
    void tx_data(tx_thread_data_t *tx_thread_data_, std::vector<uint8_t> *mpdu_);

    mac_sublayer_data_t *mac_data;
    std::thread *mac_layer_thread;
};

#endif // TX_IEEE802_15_4_H
