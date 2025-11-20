#include "phy_layer.h"

#include <QDebug>

//--------------------------------------------------------------------------------------------------
phy_layer::phy_layer()
{
    phy_pib = new phy_pib_t;

    rx_sap = new rx_sap_t;

    demodulator = new oqpsk_demodulator;
    demodulator->connect_phy(this);

    tx_sap = new rf_sap_t;
    modulator = new oqpsk_modulator;
    modulator->connect_phy(this);

    mac = new mac_sublayer;
    mac->connect_phy(this);
    mac->connect_plme(this);
    connect_mac(mac);
    pd_data = new pd_data_indication_t;
}
//--------------------------------------------------------------------------------------------------
phy_layer::~phy_layer()
{
    fprintf(stderr, "phy_layer::~phy_layer() start\n");
    rx_sap->condition.notify_all();
    delete demodulator;
    delete modulator;
    delete rx_sap;
    delete tx_sap;
    delete mac;
    delete pd_data;
    delete phy_pib;
    fprintf(stderr, "phy_layer::~phy_layer() stop\n");
}
//--------------------------------------------------------------------------------------------------
void phy_layer::start(rx_thread_data_t *rx_thread_data_,
                      double min_rssi_)
{
    stop();
    is_started = true;

    min_rssi = min_rssi_;

    modulator_thread = new std::thread(&oqpsk_modulator::start, modulator, tx_sap);
    modulator_thread->detach();

    rx_thread_data = rx_thread_data_;
    rx_thread_data->set_mode = rx_mode_data;
    rx_sap->set_mode = rx_mode_data;
    rx_sap->mode = rx_sap->set_mode;
    demodulator_thread = new std::thread(&oqpsk_demodulator::start, demodulator,
                                         rx_thread_data, rx_sap);
    demodulator_thread->detach();

    m_sublayer_thread = new std::thread(&mac_sublayer::start, mac);
    m_sublayer_thread->detach();

    work();
}
//--------------------------------------------------------------------------------------------------
void phy_layer::callback_stop_phy_layer()
{
    stop();
}
//--------------------------------------------------------------------------------------------------
void phy_layer::stop()
{
    if(is_started){

        while(demodulator->is_started){
            rx_thread_data->ready = false;
            rx_thread_data->stop_demodulator = true;
            rx_thread_data->condition_value.notify_all();
            rx_sap->ready = false;
            rx_sap->condition.notify_all();
            rx_sap->ready_confirm = false;
            rx_sap->condition_confirm.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete demodulator_thread;

        while(modulator->is_started){
            tx_sap->ready = false;
            tx_sap->stop = true;
            tx_sap->cond_value.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete modulator_thread;

        while(mac->is_started){
            callback_mac->stop_mac_syblayer();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete m_sublayer_thread;

        event_mutex.lock();
        event_ready = false;
        start_on = false;
        event_mutex.unlock();
        event.notify_one();

        is_started = false;

    }
}
//--------------------------------------------------------------------------------------------------
void phy_layer::work()
{
    start_on = true;
    std::unique_lock<std::mutex> read_lock(rx_sap->mutex);
    while (start_on){
        rx_sap->condition.wait(read_lock);
        if(rx_sap->ready){
            switch (rx_sap->mode) {
            case rx_mode_data:
                rx_data();
                break;
            case rx_mode_rssi:
                break;
            case rx_mode_off:
                break;
            }
        }
//fprintf(stderr, "phy_layer::work() event_mode = %d   rx_sap->mode = %d\n", event_mode, rx_sap->mode);
    }
    stop();
    fprintf(stderr, "phy_layer::work() phy_layer::work() finish\n");
}
//--------------------------------------------------------------------------------------------------
void phy_layer::callback_demodulator_rssi()
{
    rx_sap->mutex_rssi.lock();

    rx_sap->ready_rssi = true;
    rx_sap->set_mode = rx_mode_data;

    rx_sap->mutex_rssi.unlock();
    rx_sap->condition_rssi.notify_one();
}
//--------------------------------------------------------------------------------------------------
plme_ed_confirm_t phy_layer::plme_ed_request()
{
    plme_ed_confirm_t plme_ed_confirm;
    plme_ed_confirm.energy_level = 0;
    plme_ed_confirm.status = SUCCESS;

    std::unique_lock<std::mutex> read_lock(rx_sap->mutex_rssi);
    rx_sap->ready_rssi = false;
    rx_sap->set_mode = rx_mode_rssi;
    while(1){
        if(rx_sap->condition_rssi.wait_for(read_lock, std::chrono::milliseconds(10)) != std::cv_status::timeout){
            if(rx_sap->ready_rssi){
                plme_ed_confirm.energy_level = ((min_rssi - rx_sap->rssi) / min_rssi) * 254.0;

                break;

            }
        }
        else{
            plme_ed_confirm.status = BUSY_RX;

            break;

        }
    }

    return plme_ed_confirm;
}
//--------------------------------------------------------------------------------------------------
status_t phy_layer::plme_cca_request()
{
    status_t status;
    if(rx_sap->is_signal){
        status = BUSY;

        return status;

    }

    status = IDLE;
    std::unique_lock<std::mutex> read_lock(rx_sap->mutex_rssi);
    rx_sap->ready_rssi = false;
    rx_sap->set_mode = rx_mode_rssi;
    while(1){
        if(rx_sap->condition_rssi.wait_for(read_lock, std::chrono::milliseconds(10)) != std::cv_status::timeout){
            if(rx_sap->ready_rssi){
                if(rx_sap->rssi > -75.0){
                    status = BUSY;
                }

                break;

            }
        }
        else{
            status = BUSY_RX;

            break;

        }
    }

    return status;

}
//--------------------------------------------------------------------------------------------------
void phy_layer::callback_demodulator_rx()
{
    rx_sap->mutex.lock();

    rx_sap->ready = true;

    rx_sap->mutex.unlock();
    rx_sap->condition.notify_one();
//    fprintf(stderr, "rx_sap_callback: RX_sap_thread_id %lld\n", std::this_thread::get_id());
}
//--------------------------------------------------------------------------------------------------
void phy_layer::rx_data()
{
    int idx_symbol = 0;
//    const int len = rx_symbols.size();

    int len = rx_sap->len_symbols;
    uint8_t *rx_symbols = rx_sap->symbols;

    int len_psdu = 0;
    if(len > 0 && len <= MaxPHYPacketSize * 2){
        uint8_t octet;// 8 bits
        uint8_t symbol;// 4 bits
        uint16_t crc = 0x0;// 4 x symbol
        for(int i = 0; i < len - 4; ++i){
            symbol = rx_symbols[i];
            // need two symbols for octet
            if(++idx_symbol < 2){
                octet = symbol;
            }
            else{
                idx_symbol = 0;
                octet += symbol << 0x4;
                pd_data->mpdu[len_psdu++] = octet;
                // revert bit to LSB
                uint8_t mask;
                uint8_t b = 0;
                for (int bit = 0; bit < 8; ++bit) {
                    mask = 1 << bit;
                    b |= ((octet & mask) >> bit) << (7 - bit);
                }
                // crc16
                uint16_t idx = ((crc >> 8) ^ b) & 0xff;
                crc = ((crc << 8) ^ CRC_CCITT_TABLE[idx]) & 0xffff;
            }
        }
        //frame check sequence (FCS)
        uint16_t fcs = 0x0;
        idx_symbol = 0;
        int idx_byte = 0;
        for(int i = len - 4; i < len; ++i){
            symbol = rx_symbols[i];
            // need two symbols for octet
            if(++idx_symbol < 2){
                octet = symbol;
            }
            else{
                idx_symbol = 0;
                octet += symbol << 0x4;
                pd_data->mpdu[len_psdu++] = octet;
                // revert bit to LSB
                uint8_t mask;
                uint8_t b = 0;
                for (int bit = 0; bit < 8; ++bit) {
                    mask = 1 << bit;
                    b |= ((octet & mask) >> bit) << (7 - bit);
                }
                if(++idx_byte < 2){
                    fcs = b << 0x8;
                }
                else{
                    fcs += b;
                }
            }
        }

//        bool check_crc = crc == fcs;
//        qDebug() << "fcs" << QString::number(fcs, 16)
//                 << "crc" << QString::number(crc, 16)
//                 << check_crc;

        // check CRC
        if(crc == fcs){
            pd_data->ppdu_link_quality = 0x0;
            pd_data->mpdu_length = len_psdu;

            callback_mac->pd_data_indication(pd_data);

        }
        else{

        }
    }
    else{

    }
}
//--------------------------------------------------------------------------------------------------
status_t phy_layer::pd_data_request(pd_data_request_t *pd_data_)
{
    psdu.resize(pd_data_->mpdu_length);
    uint16_t crc = 0x0;
    for(int i = 0; i < pd_data_->mpdu_length; ++i){
        uint8_t octet = pd_data_->mpdu[i];
        psdu[i] = octet;
        // revert bit to LSB
        uint8_t mask;
        uint8_t b = 0;
        for (int bit = 0; bit < 8; ++bit) {
            mask = 1 << bit;
            b |= ((octet & mask) >> bit) << (7 - bit);
        }
        // crc16
        uint16_t idx = ((crc >> 8) ^ b) & 0xff;
        crc = ((crc << 8) ^ CRC_CCITT_TABLE[idx]) & 0xffff;

    }
    for(int i = 1; i >= 0; --i){
        uint8_t octet = (crc >> 8 * i) & 0xff;
        // revert bit to LSB
        uint8_t mask;
        uint8_t b = 0;
        for (int bit = 0; bit < 8; ++bit) {
            mask = 1 << bit;
            b |= ((octet & mask) >> bit) << (7 - bit);
        }
        psdu.push_back(b);
    }

    tx_data(&psdu);

    std::unique_lock<std::mutex> read_lock(tx_sap->mutex_confirm);
    while(1){
        if(tx_sap->cond_confirm.wait_for(read_lock, std::chrono::milliseconds(10)) != std::cv_status::timeout){
            if(tx_sap->ready_confirm){

                break;

            }
        }
        else{
            tx_sap->status = BUSY_TX;

            break;

        }
    }
    tx_sap->ready_confirm = false;
    tx_sap->mutex_confirm.unlock();

    return tx_sap->status;

}
//--------------------------------------------------------------------------------------------------
void phy_layer::tx_data(std::vector<uint8_t> *psdu_)
{
//    fprintf(stderr, "phy_layer::tx_data()\n");

    tx_sap->mutex.lock();

    tx_sap->symbols = psdu_;

    tx_sap->ready = true;
    tx_sap->mutex.unlock();
    tx_sap->cond_value.notify_one();
}
//--------------------------------------------------------------------------------------------------
status_t phy_layer::plme_set_trx_state_request(status_t state_)
{
    status_t status = SUCCESS;
    switch (state_) {
    case RX_ON:

        rx_sap->set_mode = rx_mode_data;

        break;
    {
        rx_sap->ready_confirm = false;
        std::unique_lock<std::mutex> read_lock(rx_sap->mutex_confirm);
        rx_sap->set_mode = rx_mode_data;
        while(1){
            if(rx_sap->condition_confirm.wait_for(read_lock, std::chrono::milliseconds(10)) != std::cv_status::timeout){
                if(rx_sap->ready_confirm){
                    if(rx_sap->mode == rx_mode_data){
fprintf(stderr, "phy_layer::plme_set_trx_state_request: SUCCESS\n");
                        break;

                    }
                }
            }
            else{
                status = TRX_OFF;
fprintf(stderr, "phy_layer::plme_set_trx_state_request: TRX_OFF\n");
                break;

            }
        }
    }
        break;
    case TRX_OFF:
        break;
    case FORCE_TRX_OFF:
        break;
    case TX_ON:
        rx_sap->mode = rx_mode_off;
        rx_sap->ready = true;
        rx_sap->condition.notify_one();
        break;
    default:
        status = INVALID_PARAMETER;
        break;
    }

    return status;

}
//--------------------------------------------------------------------------------------------------
plme_set_confirm_t phy_layer::plme_set_request(pib_attribute_t attribute_, void *value_)
{
    plme_set_confirm_t plme_set_confirm;
    plme_set_confirm.pib_atribute = attribute_;
    plme_set_confirm.status = SUCCESS;

    switch (plme_set_confirm.pib_atribute) {
    case phyCurrentChannel:
    {
        int channel = *reinterpret_cast<uint8_t*>(value_);
        bool status;
        callback_dev->set_channel(channel, status);
        if(status){
            phy_pib->current_channel = channel;
        }
        else{
            plme_set_confirm.status = INVALID_PARAMETER;
        }
    }
        break;
    case phyChannelsSupported:
        phy_pib->channels_supported = *reinterpret_cast<uint32_t*>(value_);;
        break;
    case phyTransmitPower:
        phy_pib->transmit_power = *reinterpret_cast<uint32_t*>(value_);;
        break;
    case phyCCAMode:
        phy_pib->cca_mode = *reinterpret_cast<cca_mode_t*>(value_);;
        break;
    }

    return plme_set_confirm;

}
//--------------------------------------------------------------------------------------------------
plme_get_confirm_t phy_layer::plme_get_request(pib_attribute_t attribute_)
{
    plme_get_confirm_t plme_get_confirm;
    plme_get_confirm.pib_atribute = attribute_;
    plme_get_confirm.status = SUCCESS;

    switch (plme_get_confirm.pib_atribute) {
    case phyCurrentChannel:
        plme_get_confirm.value = &phy_pib->current_channel;
        break;
    case phyChannelsSupported:
        plme_get_confirm.value = &phy_pib->channels_supported;
        break;
    case phyTransmitPower:
        plme_get_confirm.value = &phy_pib->transmit_power;
        break;
    case phyCCAMode:
        plme_get_confirm.value = &phy_pib->cca_mode;
        break;
    default:
        plme_get_confirm.status = UNSUPPORTED_ATTRIBUTE;
        break;
    }

    return plme_get_confirm;
}
//--------------------------------------------------------------------------------------------------
