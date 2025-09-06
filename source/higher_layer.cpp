#include "higher_layer.h"

#include <QDebug>
//-----------------------------------------------------------------------------------------
higher_layer::higher_layer()
{

}
//-----------------------------------------------------------------------------------------
void higher_layer::start_scan(scan_type_t type_)
{
    mlme_scan_confirm_t scan_confirm;
    scan_confirm.unscanned_channels = 0;
    switch (type_) {
    case energy_detect_scan:
        scan_confirm = callback_mlme->mlme_scan_request(energy_detect_scan, channel_mask, scan_duration);
        break;
    case active_scan:
        scan_confirm = callback_mlme->mlme_scan_request(active_scan, channel_mask, scan_duration);
        break;
    case passive_scan:
        break;
    case orphan_scan:
        break;
    case all_scan:
        callback_mlme->mlme_scan_request(energy_detect_scan, channel_mask, scan_duration);
        scan_confirm = callback_mlme->mlme_scan_request(active_scan, channel_mask, scan_duration);
        break;
    }

    uint8_t current_channel = 0;
    for(uint8_t channel = 0; channel < 27; ++channel) {
        if (channel_mask & (1UL << channel)){
            if((scan_confirm.unscanned_channels & channel) == 0){
                current_channel = channel;
            }
        }
    }

    callback_ui->channel_scan(current_channel, true);

}
//-----------------------------------------------------------------------------------------
void higher_layer::start_new_pan()
{
    mlme_scan_confirm_t scan_confirm;

    scan_confirm = callback_mlme->mlme_scan_request(energy_detect_scan, channel_mask, scan_duration);

    std::vector<uint8_t> energy_detect_list{};
    for(uint i = 0; i < scan_confirm.result_list_size; ++i){
        energy_detect_list.push_back(scan_confirm.energy_detect_list[i]);
    }
    uint32_t unscanned_shannels_energy_detect = scan_confirm.unscanned_channels;

    scan_confirm = callback_mlme->mlme_scan_request(active_scan, channel_mask, scan_duration);

    std::vector<pan_description_t> pan_description_list{};
    for(uint i = 0; i < scan_confirm.result_list_size; ++i){
        pan_description_list.push_back(scan_confirm.pan_description_list[i]);
    }
    uint32_t unscanned_shannels_active_scan = scan_confirm.unscanned_channels;

//    uint8_t energy_threshold = 82;

    uint8_t current_channel = 0;
    uint8_t min_energy = 0xff;
    uint8_t min_pan = 0xff;
    int idx_ed_list = 0;
    uint8_t new_channel = 0xff;
    std::vector<uint16_t> pan_id{};
    for(; current_channel < 27; ++current_channel) {
        if (channel_mask & (1UL << current_channel)){
            if((unscanned_shannels_energy_detect & current_channel) == 0){
                uint8_t energy_detect = energy_detect_list.at(idx_ed_list++);
                if((unscanned_shannels_active_scan & current_channel) == 0){
                    int pan_count = 0;
                    std::vector<uint16_t> current_pan_id{};
                    for(uint p = 0; p < pan_description_list.size(); ++p){
                        if(pan_description_list.at(p).logical_channel == current_channel){
                            current_pan_id.push_back(pan_description_list.at(p).coord_pan_id);
                            ++pan_count;
                        }
                    }
                    if(pan_count <= min_pan){
                       if(energy_detect <= min_energy){
                           min_energy = energy_detect;
                           new_channel = current_channel;
                           pan_id.clear();
                           for(uint i = 0; i < current_pan_id.size(); ++i){
                               pan_id.push_back(current_pan_id.at(i));
                           }
                       }
                    }
                }
            }
        }
    }

    if(new_channel != 0xff){
        mlme_start_request_t start_request;
        start_request.pan_coordinator = true;
        start_request.logical_channel = new_channel;
        start_request.pan_id = 0xAAAA; // TODO

        if(callback_mlme->mlme_start_request(start_request) == SUCCESS){
            // TODO info;
        }
        else{
            // TODO
        }
    }
    else{
        // TODO
    }

}
//-----------------------------------------------------------------------------------------
void higher_layer::wait_confirm()
{
    event.mutex.lock();
    event.condition.wait(&event.mutex);
    event.mutex.unlock();
}
//-----------------------------------------------------------------------------------------
