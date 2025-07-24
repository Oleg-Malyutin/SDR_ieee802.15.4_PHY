#ifndef TX_MAC_SUBLAYER_H
#define TX_MAC_SUBLAYER_H

#include "mac_sublayer.h"

class tx_mpdu_sublayer_callback
{
public:
    virtual void tx_mpdu_callback(mpdu mpdu_)=0;
};

class tx_mac_sublayer
{
public:
    tx_mac_sublayer();
    ~tx_mac_sublayer();

    void start(mac_sublayer_data_t *data_);
    void connect_callback(tx_mpdu_sublayer_callback *cb_){callback = cb_;};

private:
    tx_mpdu_sublayer_callback *callback;
};

#endif // TX_MAC_SUBLAYER_H
