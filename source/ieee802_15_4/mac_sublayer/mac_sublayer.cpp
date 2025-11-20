#include "mac_sublayer.h"

#include <cstdio>
#include <iostream>

//--------------------------------------------------------------------------------------------------
mac_sublayer::mac_sublayer()
{
    mac_pib = new mac_pib_t;
    mac_pib->reset();
    pd_data = new pd_data_indication_t;
    spdu_indication = new spdu_indication_t;
    mpdu = new mpdu_analysis_t;
    timer_scan = new timer_cb;
}
//--------------------------------------------------------------------------------------------------
mac_sublayer::~mac_sublayer()
{
    fprintf(stderr, "mac_sublayer::~mac_sublayer() start\n");
    delete pd_data;
    delete spdu_indication;
    delete mpdu;
    delete timer_scan;
    fprintf(stderr, "mac_sublayer::~mac_sublayer() stop\n");
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::start()
{
    stop();
    is_started = true;

    work();
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::stop_mac_syblayer()
{
    stop();
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::stop()
{
    if(is_started){
        timer_scan->stop();
        wait_confirm.condition.notify_all();
        event_mutex.lock();
        event_ready = false;
        start_on = false;
        event_mutex.unlock();
        event.notify_one();

        is_started = false;

    }
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::work()
{
    start_on = true;
    event_mode = Idle;
    std::unique_lock<std::mutex> read_lock(event_mutex);
    while (start_on){
        event.wait(read_lock);
        if(event_ready){
            event_ready = false;
            switch (event_mode) {
            case Rx:
                mac_filtering_mpdu();
                break;
            case Tx:
                break;
            case CCA_confirm:
                break;
            case Idle:
                break;
            }
        }

//        fprintf(stderr, "mac_sublayer::work() event_mode = %d\n", event_mode);
    }
    stop();
    fprintf(stderr, "mac_sublayer::work() finish\n");
}
//--------------------------------------------------------------------------------------------------
mlme_start_confirm_t mac_sublayer::mlme_start_request(mlme_start_request_t start_request_)
{
    status_t status = SUCCESS;
    if(start_request_.pan_coordinator){
        if(callback_plme->plme_set_request(phyCurrentChannel, &start_request_.logical_channel).status == SUCCESS){
            // beaconless, security disable
            mac_pib->reset();
            mac_pib->pan_id = start_request_.pan_id;
        }
        else{
          status = INVALID_PARAMETER;
        }
    }
    else{
        status = INVALID_PARAMETER;
    }

    return status;

}
//--------------------------------------------------------------------------------------------------
mcps_data_confirm_t mac_sublayer::mcps_data_request(mcps_data_request_t *mcps_data_)
{
    mpdu_t mpdu;
    mcps_data_confirm_t mcps_data_confirm;

    // TODO mac generate

    mcps_data_confirm = send_mpdu(mpdu);

    return mcps_data_confirm;

}
//--------------------------------------------------------------------------------------------------
mcps_data_confirm_t mac_sublayer::send_mpdu(mpdu_t mpdu_)
{
    mcps_data_confirm_t mcps_data_confirm;
    std::unique_lock<std::mutex> read_lock(wait_confirm.mutex);
    int nb = 0;

    if(callback_plme->plme_set_trx_state_request(RX_ON) == SUCCESS){

        for (nb = 0; nb < aMaxCsmaBackoffs; ++nb){

            if(callback_plme->plme_cca_request() == IDLE){

                if(callback_plme->plme_set_trx_state_request(TX_ON) == SUCCESS){

                    callback_phy->pd_data_request(&mpdu_);

                    if(callback_plme->plme_set_trx_state_request(RX_ON) == SUCCESS){

                        // TODO confirm

                    }

                }

                break;

            }
            else{

                uint16_t delay;
                do{
                    delay = (uint16_t)rand();
                }while (delay == 0);
                int csma_time_ms = delay % (uint16_t)((1 << mac_pib->min_be) - 1);
                timer_scan->start(csma_time_ms, callback_timer_scan, this);
                while(timer_scan->is_active){
                    wait_confirm.condition.wait(read_lock);
                }
                wait_confirm.mutex.unlock();

            }

        }

    }

    return mcps_data_confirm;

}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::pd_data_indication(pd_data_indication_t *pd_data_)
{
        event_mutex.lock();
        pd_data->mpdu_length = pd_data_->mpdu_length;
        for(int i = 0; i < pd_data->mpdu_length; ++i){
            pd_data->mpdu[i] = pd_data_->mpdu[i];
        }
        pd_data->ppdu_link_quality = pd_data_->ppdu_link_quality;

        event_ready = true;
        event_mode = Rx;
        event_mutex.unlock();
        event.notify_one();
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::mac_filtering_mpdu()
{
    frame_control_field fcf = frame_control_parser();

    switch (fcf.type) {
    case Frame_type_beacon:
        mac_beacon(fcf);
        break;
    case Frame_type_data:
        mcps_data_rx(fcf);
        break;
    case Frame_type_acknowledgment:
        mac_acknowledgment(fcf);
        break;
    case Frame_type_MAC_command:
        mac_command(fcf);
        break;
    case Frame_type_reserved:
        break;
    }
}
//--------------------------------------------------------------------------------------------------
frame_control_field mac_sublayer::frame_control_parser()
{
    frame_control_field fcf;
    uint16_t frame_control = pd_data->mpdu[0];
    frame_control += static_cast<uint16_t>(pd_data->mpdu[1]) << 0x8;
    uint8_t type = frame_control & 0x4;
    if(type == 0){
        type = frame_control & 0x3;
    }
    fcf.type = static_cast<frame_type>(type);
    fcf.security_enabled = (frame_control & 0x8) >> 0x3;
    fcf.frame_pending = (frame_control & 0x10) >> 0x4 ;
    fcf.ask_request = (frame_control & 0x20) >> 0x5;
    fcf.intra_pan = (frame_control & 0x40) >> 0x6;
    fcf.dest_mode = static_cast<adressing_mode>((frame_control & 0xc00) >> 0xa);
    fcf.frame_version = (frame_control & 0x3000) >> 0xc;
    fcf.source_mode = static_cast<adressing_mode>((frame_control & 0xc000) >> 0xe);

    return fcf;
}
//--------------------------------------------------------------------------------------------------
addressing_fields mac_sublayer::parse_addressing(frame_control_field &fcf_)
{
    addressing_fields af;
    uint8_t offset = 3; //frame_control_field 2(oct) + sequence_number 1(oct)
    uint8_t *data = pd_data->mpdu;
    uint8_t *p_d = data + offset;
    af.dest_pan_id = p_d[0] + (p_d[1] << 0x8);
    offset += 2;
    p_d = data + offset;
    bool  da = true;
    switch(fcf_.dest_mode){
    case Adressing_mode_not_present:
        da = false;
        af.dest_address = 0x0;
        break;
    case Adressing_mode_reserved:
        da = false;
        af.dest_address = 0x0;
        break;
    case Adressing_mode_16bit_short:
        af.dest_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data + offset;
        break;
    case Adressing_mode_64bit_extended:
        af.dest_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data + offset;
        uint64_t address = p_d[0] + (p_d[1] << 0x8);
        af.dest_address += (address << 0x10) + af.dest_address;
        offset += 2;
        p_d = data + offset;
        break;
    }
    af.dest_address_mode = fcf_.dest_mode;

    bool sa = (fcf_.source_mode > Adressing_mode_reserved) && da;
    if(fcf_.intra_pan == 0 && sa){
        af.source_pan_id = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data + offset;
    }
    else{
        af.source_pan_id = 0x0;
    }
    switch(fcf_.source_mode){
    case Adressing_mode_not_present:
        af.source_address = 0x0;
        break;
    case Adressing_mode_reserved:
        af.source_address = 0x0;
        break;
    case Adressing_mode_16bit_short:
        af.source_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        break;
    case Adressing_mode_64bit_extended:
        af.source_address = p_d[0] + (p_d[1] << 0x8);
        offset += 2;
        p_d = data + offset;
        uint64_t address = p_d[0] + (p_d[1] << 0x8);
        af.source_address += (address << 0x10) + af.source_address;
        offset += 2;
        break;
    }
    af.source_address_mode = fcf_.source_mode;

    af.offset = offset;

    int len_data = pd_data->mpdu_length;
    for(int i = 0; i < len_data; ++i){
        mpdu->msdu[i] = data[i];
    }
    mpdu->fcf = fcf_;
    mpdu->af = af;
    mpdu->len_data = len_data;

    callback_mpdu->rx_mpdu(mpdu);

    return af;

}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::mac_beacon(frame_control_field fcf_)
{
    addressing_fields af = parse_addressing(fcf_);
    if(1){
        pan_description_list.back().coord_pan_id = af.dest_pan_id;
        pan_description_list.back().coord_address = af.source_address;

        callback_ui->active_detect(pan_description_list.back().logical_channel, af.dest_pan_id, af.source_address);

        pan_description_t pan_description;
        pan_description.logical_channel = pan_description_list.back().logical_channel;
        pan_description.coord_pan_id = 0x0;
        pan_description.coord_address = 0x0;
        pan_description_list.push_back(pan_description);
    }
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::mcps_data_rx(frame_control_field fcf_)
{
    addressing_fields af = parse_addressing(fcf_);
    spdu_indication->src_addr_mode = af.source_address_mode;
    spdu_indication->src_pan_id = af.source_pan_id;
    spdu_indication->src_addr = af.source_address;
    spdu_indication->dst_addr_mode = af.dest_address_mode;
    spdu_indication->dst_pan_id = af.dest_pan_id;
    spdu_indication->dst_addr = af.dest_address;
    int len_msdu = pd_data->mpdu_length - af.offset;
    uint8_t *data = &pd_data->mpdu[af.offset];
    for(int i = 0; i < len_msdu; ++i){
        spdu_indication->msdu[i] = data[i];
    }
    spdu_indication->msdu_length = len_msdu;
    spdu_indication->mpdu_link_quality = pd_data->ppdu_link_quality;
    spdu_indication->security_use = false;
    spdu_indication->acl_entry = 0x8;

    // ->mcps_data_indication(spdu_indication);

}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::mac_acknowledgment(frame_control_field fcf_)
{
    int len_data = pd_data->mpdu_length;
    for(int i = 0; i < len_data; ++i){
        mpdu->msdu[i] = pd_data->mpdu[i];
    }
    mpdu->fcf = fcf_;
//    mpdu->af = NULL;
    mpdu->len_data = len_data;
    callback_mpdu->rx_mpdu(mpdu);
}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::mac_command(frame_control_field fcf_)
{
     addressing_fields af = parse_addressing(fcf_);

}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::mlme_set_request(pib_attribute_t attribute_, void *value_)
{
    mac_pib_attribute = attribute_;
    switch (mac_pib_attribute) {
    case macAckWaitDuration:
        mac_pib->ack_wait_duration = *reinterpret_cast<uint16_t*>(value_);
        break;
    case macAssociationPermit:
        mac_pib->associaton_permit = *reinterpret_cast<bool*>(value_);
        break;
    case macAutoRequest:
        mac_pib->auto_request = *reinterpret_cast<bool*>(value_);
        break;
    case macBattLifeExt:
        mac_pib->batt_life_ext = *reinterpret_cast<bool*>(value_);
        break;
    case  macBattLifeExtPeriods:
        break;
    case  macBeaconPayload:
        break;
    case  macBeaconPayloadLength:
        break;
    case  macBeaconOrder:
        break;
    case  macBeaconTxTime:
        break;
    case  macBSN:
        break;
    case  macCoordExtendedAddress:
        mac_pib->coord_extendet_address = *reinterpret_cast<uint64_t*>(value_);
        break;
    case  macCoordShortAddress:
        mac_pib->coord_short_address = *reinterpret_cast<uint16_t*>(value_);
        break;
    case  macDSN:
        mac_pib->dsn = *reinterpret_cast<uint8_t*>(value_);
        break;
    case  macGTSPermit:
        break;
    case  macMaxCSMABackoffs:
        mac_pib->max_csma_backoffs = *reinterpret_cast<uint8_t*>(value_);
        break;
    case  macMinBE:
        mac_pib->min_be = *reinterpret_cast<uint8_t*>(value_);
        break;
    case  macPANId:
        mac_pib->pan_id = *reinterpret_cast<uint16_t*>(value_);
        break;
    case  macPromiscuousMode:
        mac_pib->promiscuous_mode = *reinterpret_cast<bool*>(value_);
        break;
    case  macRxOnWhenIdle:
        mac_pib->rx_on_when_idle = *reinterpret_cast<bool*>(value_);
        break;
    case  macShortAddress:
        mac_pib->short_address = *reinterpret_cast<uint16_t*>(value_);
        break;
    case  macSuperframeOrder:
        mac_pib->superframe_order = *reinterpret_cast<uint8_t*>(value_);
        break;
    case  macTransactionPersistenceTime:
        mac_pib->transaction_persistence_time = *reinterpret_cast<uint16_t*>(value_);
        break;
    }
}
//--------------------------------------------------------------------------------------------------
mlme_scan_confirm_t mac_sublayer::mlme_scan_request(scan_type_t scan_type_, uint32_t scan_channels_,
                                                    uint8_t scan_duration_)
{
    uint16_t scan_duration_ms = aBaseSuperframeDuration * ((1UL << scan_duration_) + 1);
    uint8_t scan_current_channel = 0;

    mlme_scan_confirm_t mlme_scan_confirm;

    switch (scan_type_) {
    case energy_detect_scan:
        mlme_scan_confirm = mlme_energy_detect_scan(scan_type_, scan_channels_, scan_duration_ms, scan_current_channel);
        break;
    case active_scan:
        mlme_scan_confirm = mlme_active_scan(scan_type_, scan_channels_, scan_duration_ms, scan_current_channel);
        break;
    case passive_scan:
        mlme_scan_confirm = mlme_passive_scan(scan_type_, scan_channels_, scan_duration_ms, scan_current_channel);
        break;
    case orphan_scan:
        break;
    }

    return mlme_scan_confirm;
}
//--------------------------------------------------------------------------------------------------
mlme_scan_confirm_t mac_sublayer::mlme_energy_detect_scan(scan_type_t scan_type_, uint32_t scan_channels_,
                                                          uint16_t scan_duration_ms_, uint8_t scan_current_channel_)
{
    energy_detect_list.clear();
    uint8_t scan_current_channel = scan_current_channel_;
    uint32_t unscanned_shannels = 0;
    for(; scan_current_channel < 27; ++scan_current_channel) {
        // shift the bitmask and compare to the channel mask
        if (scan_channels_ & (1UL << scan_current_channel)){

            if(callback_plme->plme_set_request(phyCurrentChannel, &scan_current_channel).status == SUCCESS){

                callback_ui->channel_scan(scan_current_channel, false);

                timer_scan->start(scan_duration_ms_);
                uint8_t max_energy_level = 0;
                while(timer_scan->is_active){

                    plme_ed_confirm_t plme_ed_confirm = callback_plme->plme_ed_request();

                    if(plme_ed_confirm.status == SUCCESS){
                        if(plme_ed_confirm.energy_level > max_energy_level){
                            max_energy_level = plme_ed_confirm.energy_level;
                        }
                    }
                    else{
                        // TODO
                    }

                }
                energy_detect_list.push_back(max_energy_level);

                callback_ui->energy_detect(scan_current_channel, max_energy_level);

            }
            else{
                unscanned_shannels |= 1UL << scan_current_channel;
            }

        }

    }
    // TODO status
    mlme_scan_confirm_t scan_confirm;
    scan_confirm.status = SUCCESS;
    scan_confirm.scan_type = scan_type_;
    scan_confirm.result_list_size = energy_detect_list.size();
    scan_confirm.energy_detect_list = energy_detect_list.data();
    scan_confirm.unscanned_channels = unscanned_shannels;

    return scan_confirm;

}
//--------------------------------------------------------------------------------------------------
mlme_scan_confirm_t mac_sublayer::mlme_active_scan(scan_type_t scan_type_, uint32_t scan_channels_,
                                                    uint16_t scan_duration_ms_, uint8_t scan_current_channel_)
{
    uint8_t scan_current_channel = scan_current_channel_;
    mac_pib->original_pan_id = mac_pib->pan_id;
    mac_pib->pan_id = MAC_BROADCAST_ADDR;
    uint16_t fcf = 0x03 | (0x02 << 10) /*| (0x00 << 14)*/;
    pd_data_request_t beacon_request;
    uint8_t idx = 0;
    beacon_request.mpdu[idx++] = ((fcf >> 8*0) & 0xFF);
    beacon_request.mpdu[idx++] = ((fcf >> 8*1) & 0xFF);
    // sequence number
    uint8_t sn = mac_pib->dsn;
    uint8_t idx_sn = idx;
    beacon_request.mpdu[idx++] = sn;
    // dst PAN ID
    beacon_request.mpdu[idx++] = 0xFF;
    beacon_request.mpdu[idx++] = 0xFF;
    // dst address
    beacon_request.mpdu[idx++] = 0xFF;
    beacon_request.mpdu[idx++] = 0xFF;
    // beacon request
    beacon_request.mpdu[idx++] = 0x07;

    beacon_request.mpdu_length = idx;

    pan_description_list.clear();

    std::unique_lock<std::mutex> read_lock(wait_confirm.mutex);
    uint32_t unscanned_shannels = 0;

//    scan_current_channel = 15;

    for(; scan_current_channel < 27; ++scan_current_channel) {
        // shift the bitmask and compare to the channel mask
        if (scan_channels_ & (1UL << scan_current_channel)){

            int nb = 0;
            bool scaned = false;
            int num_scan = 1;

            if(callback_plme->plme_set_request(phyCurrentChannel, &scan_current_channel).status == SUCCESS){

                if(callback_plme->plme_set_trx_state_request(RX_ON) == SUCCESS){

                    for (nb = 0; nb < aMaxCsmaBackoffs; ++nb){

                        if(callback_plme->plme_cca_request() == IDLE){

                            if(callback_plme->plme_set_trx_state_request(TX_ON) == SUCCESS){
//#define REITING
#ifdef REITING
auto start_time = std::chrono::steady_clock::now();
#endif
                                callback_phy->pd_data_request(&beacon_request);
#ifdef REITING
auto end_time = std::chrono::steady_clock::now();
auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
//if(elapsed_time > 1024){
    std::cout << "all: " << elapsed_time/* - 1024 */<< " us" << std::endl;
//}
#endif
                                if(callback_plme->plme_set_trx_state_request(RX_ON) == SUCCESS){

                                    callback_ui->channel_scan(scan_current_channel, false);

                                    pan_description_t pan_description;
                                    pan_description.logical_channel = scan_current_channel;
                                    pan_description.coord_pan_id = 0x0;
                                    pan_description.coord_address = 0x0;
                                    pan_description_list.push_back(pan_description);

                                    timer_scan->start(scan_duration_ms_, callback_timer_scan, this);
                                    while(timer_scan->is_active){
                                        wait_confirm.condition.wait(read_lock);
                                    }
                                    wait_confirm.mutex.unlock();
                                    fprintf(stderr, "mac_sublayer::mlme_active_scan()4 channel %d\n", scan_current_channel);

                                }

                                beacon_request.mpdu[idx_sn] = ++sn;

                                scaned = true;

                            }

                            break;

                        }
                        else{

                            uint16_t delay;
                            do{
                                delay = (uint16_t)rand();
                            }while (delay == 0);
                            int csma_time_ms = delay % (uint16_t)((1 << mac_pib->min_be) - 1);
                            timer_scan->start(csma_time_ms, callback_timer_scan, this);
                            while(timer_scan->is_active){
                                wait_confirm.condition.wait(read_lock);
                            }
                            wait_confirm.mutex.unlock();

                        }

                    }

                }

            }

            if(nb == aMaxCsmaBackoffs || scaned != true){
                // TODO
                unscanned_shannels |= 1UL << scan_current_channel;

                continue;

            }

        }

    }
    mlme_scan_confirm_t scan_confirm;
    scan_confirm.status = SUCCESS;
    scan_confirm.scan_type = scan_type_;
    scan_confirm.result_list_size = pan_description_list.size();
    scan_confirm.pan_description_list = pan_description_list.data();
    scan_confirm.unscanned_channels = unscanned_shannels;

    return scan_confirm;
}
//--------------------------------------------------------------------------------------------------
mlme_scan_confirm_t mac_sublayer::mlme_passive_scan(scan_type_t scan_type_, uint32_t scan_channels_,
                             uint16_t scan_duration_ms_, uint8_t scan_current_channel_)
{

}
//--------------------------------------------------------------------------------------------------
void mac_sublayer::callback_timer_scan(void *ctx_)
{
    mac_sublayer *m = reinterpret_cast<mac_sublayer*>(ctx_);
    m->wait_confirm.condition.notify_all();
}
//--------------------------------------------------------------------------------------------------


