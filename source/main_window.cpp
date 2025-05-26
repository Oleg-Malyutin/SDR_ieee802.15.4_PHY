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

    dev = new device;
    connect(dev, &device::device_found, this, &main_window::device_found);
    connect(dev, &device::remove_device, this, &main_window::remove_device);
    connect(this, &main_window::start, dev, &device::start);
    connect(this, &main_window::stop, dev, &device::stop);
    connect(ui->actionAdvanced_device_settings, &QAction::triggered,
            dev, &device::advanced_settings_dialog);
    connect(dev, &device::get_frame_capture, this, &main_window::get_frame_capture);

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

    for(int i = 0; i < ieee802_15_4_info::number_of_channels; ++i){
        ui->comboBox_channel->addItem(QString::number(i));
    }
    ui->comboBox_channel->setCurrentIndex(ieee802_15_4_info::channel);
    ui->verticalSlider->setValue(26);
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
    ui->actionAdvanced_device_settings->setEnabled(true);
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
                ui->actionAdvanced_device_settings->setEnabled(false);
            }
        }
    }
}
//-----------------------------------------------------------------------------------------
void main_window::on_comboBox_channel_currentIndexChanged(int index)
{
    ieee802_15_4_info info;
    uint64_t rf_fq = info.fq_channel_mhz[index] * 1e6;
    dev->set_rx_frequency(rf_fq);
}
//-----------------------------------------------------------------------------------------
void main_window::on_verticalSlider_valueChanged(int value)
{
    double gain_db = value;
    dev->set_rx_hardwaregain(gain_db);
}
//-----------------------------------------------------------------------------------------
void main_window::get_frame_capture(QStringList list_)
{
   for(const auto &it : list_){
       ui->textBrowser_capture->append(it);
   }
}
//-----------------------------------------------------------------------------------------
void main_window::on_pushButton_start_clicked()
{
    emit start(name_select_device);
    on_comboBox_channel_currentIndexChanged(ui->comboBox_channel->currentIndex());
    on_verticalSlider_valueChanged(ui->verticalSlider->value());
}
//-----------------------------------------------------------------------------------------
void main_window::on_pushButton_stop_clicked()
{
    emit stop();
}
//-----------------------------------------------------------------------------------------
