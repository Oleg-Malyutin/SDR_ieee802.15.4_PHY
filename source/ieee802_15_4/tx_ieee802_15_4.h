#ifndef TX_IEEE802_15_4_H
#define TX_IEEE802_15_4_H

#include "ieee802_15_4.h"

class tx_ieee802_15_4
{
public:
    tx_ieee802_15_4();
    ~tx_ieee802_15_4();

    bool is_started = false;
    void connect_callback(ieee802_15_4_callback *cb_){callback = cb_;};
    void start(tx_thread_data_t *tx_thread_data_);

private:
    float **chip_i;
    float **chip_q;
    int offset_shr;
    float tx_data[LEN_MAX_SAMPLES * 2];
    void symbol_generate();
    ieee802_15_4_callback *callback;
};

#endif // TX_IEEE802_15_4_H
