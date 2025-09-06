#ifndef HIGHER_LAYER_H
#define HIGHER_LAYER_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <vector>

#include "callback_higher_layer.h"
#include "callback_ui.h"
#include "ieee802_15_4/callback_layer.h"
#include "ieee802_15_4/mac_sublayer/mac_types.h"

class higher_layer : public QObject,
                     public higher_layer_callbck
{
    Q_OBJECT
public:
    higher_layer();
    void connect_ui(ui_callback *cb_){callback_ui = cb_;};
    void connect_mlme(mlme_sap_callback *cb_){callback_mlme = cb_;};
    void connect_mac(mac_sublayer_callback *cb_){callback_mac = cb_;};

public slots:
    void start_scan(scan_type_t type_);
    void start_new_pan();

private:
    enum event_mode{
        SCAN,
        NEW_PAN
    };
    typedef struct{
       event_mode mode;
       QMutex mutex;
       QWaitCondition condition;
    } event_t;
    event_t event;
    uint32_t channel_mask = 0x7FFF800;// 11-26
    uint8_t scan_duration = 0x07;
    ui_callback *callback_ui;
    mlme_sap_callback *callback_mlme;
    mac_sublayer_callback *callback_mac;



    void wait_confirm();
};

#endif // HIGHER_LAYER_H
