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
#include "main_window.h"
#include "ui_main_window.h"

//-----------------------------------------------------------------------------------------
main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::main_window)
{
    ui->setupUi(this);

    qRegisterMetaType<mpdu>();

    dev = new device;
    connect(dev, &device::device_found, this, &main_window::device_found);
    connect(dev, &device::remove_device, this, &main_window::remove_device);
    connect(dev, &device::device_status, this, &main_window::device_status);
    connect(this, &main_window::start, dev, &device::start);
    connect(this, &main_window::stop, dev, &device::stop);
    connect(this, &main_window::test_shr, dev, &device::test_shr);
    connect(this, &main_window::test_test, dev, &device::test_test);
    connect(ui->actionAdvanced_device_settings, &QAction::triggered,
            dev, &device::advanced_settings_dialog);
    connect(dev, &device::mac_protocol_data_units, this, &main_window::mac_protocol_data_units);

    plot_constelation = new plot(ui->plot_constelation,
                                 type_graph::type_constelation, "Packet detection");
    connect(dev, &device::plot_constelation,
            plot_constelation, &plot::replace_constelation, Qt::QueuedConnection);

    plot_sfd_correlation = new plot(ui->plot_sfd_correlation,
                                    type_graph::type_oscilloscope_2, "SFD correlation");
    connect(dev, &device::plot_sfd_correlation,
            plot_sfd_correlation, &plot::replace_oscilloscope, Qt::QueuedConnection);

    plot_sfd_synchronize = new plot(ui->plot_sfd_synchronize,
                                    type_graph::type_oscilloscope_2, "SFD synchronize");
    connect(dev, &device::plot_sfd_synchronize,
            plot_sfd_synchronize, &plot::replace_oscilloscope, Qt::QueuedConnection);

    plot_preamble_correlation = new plot(ui->plot_preamble_correlaion,
                                         type_graph::type_oscilloscope_2, "Preamble correlaion");
    connect(dev, &device::plot_preamble_correlaion,
            plot_preamble_correlation, &plot::replace_oscilloscope, Qt::QueuedConnection);

    plot_test = new plot(ui->plot_trnsmitter,
                         type_graph::type_oscilloscope_2, "Test");
    connect(dev, &device::plot_test,
            plot_test, &plot::replace_oscilloscope, Qt::QueuedConnection);

    for(int i = 0; i < ieee802_15_4_info::number_of_channels; ++i){
        ui->comboBox_channel->addItem(QString::number(i));
        ui->comboBox_tx_channel->addItem(QString::number(i));
    }
    ui->comboBox_channel->setCurrentIndex(ieee802_15_4_info::channel);
    ui->verticalSlider->setValue(69);
}
//-----------------------------------------------------------------------------------------
main_window::~main_window()
{
    qDebug() << "main_window::~main_window() start";
//    disconnect(dev, &device::plot_constelation, plot_constelation, &plot::replace_constelation);
//    disconnect(dev, &device::plot_sfd_correlation, plot_sfd_correlation, &plot::replace_oscilloscope);
//    disconnect(dev, &device::plot_sfd_synchronize, plot_sfd_synchronize, &plot::replace_oscilloscope);
//    disconnect(dev, &device::plot_preamble_correlaion, plot_preamble_correlation, &plot::replace_oscilloscope);
    delete plot_constelation;
    delete plot_sfd_correlation;
    delete plot_sfd_synchronize;
    delete plot_preamble_correlation;
    delete dev;
    delete ui;
    qDebug() << "main_window::~main_window() stop";
}
//-----------------------------------------------------------------------------------------
void main_window::device_found(QString name_)
{
    for(const auto &action : ui->menuOpen_device->actions()){
        if(action->text() == name_){
            return;
        }
    }
    QAction *action = new QAction(name_);
    ui->menuOpen_device->addAction(action);
    connect(action, &QAction::triggered, this, [this, v = name_]{ select_device(v);});
}
//-----------------------------------------------------------------------------------------
void main_window::select_device(QString name_)
{
    name_select_device = name_;
    for(auto &action : ui->menuOpen_device->actions()){
        if(action->text() ==  name_select_device){
            action->setEnabled(false);
        }
        else{
            action->setEnabled(true);
        }
    }
}
//-----------------------------------------------------------------------------------------
void main_window::remove_device(QString name_)
{
    for(auto &action : ui->menuOpen_device->actions()){
        if(action->text() == name_){
            ui->menuOpen_device->removeAction(action);
            if(name_select_device == name_){
                name_select_device = "";
                ui->actionAdvanced_device_settings->setEnabled(false);
            }
        }
    }
}
//-----------------------------------------------------------------------------------------
void main_window::device_status()
{
    dev->stop();
}
//-----------------------------------------------------------------------------------------
void main_window::on_comboBox_channel_currentIndexChanged(int index)
{
    dev->set_rx_frequency(index);
}
//-----------------------------------------------------------------------------------------
void main_window::on_comboBox_tx_channel_currentIndexChanged(int index)
{
    dev->set_tx_frequency(index);
}
//-----------------------------------------------------------------------------------------
void main_window::on_verticalSlider_valueChanged(int value)
{
    double gain_db = value;
    dev->set_rx_hardwaregain(gain_db);
}
//-----------------------------------------------------------------------------------------
void main_window::mac_protocol_data_units(mpdu mpdu_)
{
    QString text_capture = "Frame version: ";
    QStringList list;
    list.append("");
    switch (mpdu_.fcf.frame_version) {
    case 0:
        text_capture += "IEEE Std 802.15.4-2003.";
        break;
    case 1:
        text_capture += "IEEE Std 802.15.4-2006.";
        break;
    case 2:
        text_capture += "IEEE Std 802.15.4.";
        break;
    case 3:
        text_capture += "Reserved.";
        break;
    }
    QString frame_type;
    switch (mpdu_.fcf.type){
    case Frame_type_beacon:
        frame_type = "Frame type: beacon.";
        break;
    case Frame_type_data:
        frame_type = "Frame type: data";
        break;
    case Frame_type_acknowledgment:
        frame_type = "Frame type: acknowledgment.";
        break;
    case Frame_type_MAC_command:
        frame_type = "Frame type: MAC_command.";
        break;
    case Frame_type_reserved:
        frame_type = "Frame type: reserved.";
        break;
    }
    text_capture += " " + frame_type;
    list.append(text_capture);

    QString address;
    if(mpdu_.af.dest_address == 0){
        address = "not present";
    }
    else{
        address = "0x" + QString::number(mpdu_.af.dest_address, 16);
    }
    text_capture = "Destination PAN 0x" + QString::number(mpdu_.af.dest_pan_id, 16) +
                   ",  Destination address " + address + ". ";

    if(mpdu_.af.source_pan_id == 0){
        address = "not present";
    }
    else{
        address = "0x" + QString::number(mpdu_.af.source_pan_id, 16);
    }
    text_capture += "Source PAN " + address;
    if(mpdu_.af.source_address == 0){
        address = "not present";
    }
    else{
        address = "0x" + QString::number(mpdu_.af.source_address, 16);
    }
    text_capture += ",  Source address " + address;
    list.append(text_capture);
    QVector<QString> data;
    data.push_back(QString());

    int idx_char = 0;
    int slice = 0;
    QString text("  ");
    while(idx_char < mpdu_.len_data){
        uint16_t d = mpdu_.data[idx_char];
        if(++slice > 32){
            slice = 0;
            data.last() += text;
            text.clear();
            data.push_back(QString());
        }
        if(idx_char % 8 == 0){
            data.last().append("  ");
        }
        if(mpdu_.data[idx_char] < 0x10){
            data.last().append("0");
        }
        data.last().append(QString::number(d, 16) + " ");
        if(d > 0x1f && d < 0x7f){
            text.append(d);
        }
        else{
            text.append(".");
        }

        ++idx_char;
    }
    if(slice > 0 && slice < 32){
        data.last().append("  ");
        int add_space = (31 - slice) / 8;
        while(add_space){
            data.last().append("  ");
            --add_space;
        }
        while(++slice < 32){
            data.last().append("   ");
        }
        data.last() += text;
    }
    for(auto &it : data){
        list.append(it);
    }

   for(const auto &it : list){
       ui->textBrowser_capture->append(it);
   }
}
//-----------------------------------------------------------------------------------------
void main_window::on_pushButton_start_clicked()
{
    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(true);
    ui->actionAdvanced_device_settings->setEnabled(true);

    emit start(name_select_device);

    on_comboBox_channel_currentIndexChanged(ui->comboBox_channel->currentIndex());
    on_verticalSlider_valueChanged(ui->verticalSlider->value());
}
//-----------------------------------------------------------------------------------------
void main_window::on_pushButton_stop_clicked()
{
    ui->pushButton_stop->setEnabled(false);
    ui->pushButton_start->setEnabled(true);
    ui->actionAdvanced_device_settings->setEnabled(false);

    emit stop();
}
//-----------------------------------------------------------------------------------------
void main_window::on_pushButton_shr_clicked()
{
    emit test_shr();
}
//-----------------------------------------------------------------------------------------
void main_window::on_pushButton_test_clicked()
{
    emit test_test();
}
//-----------------------------------------------------------------------------------------


