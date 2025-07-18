#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include "device/device.h"
#include "plot/plot.h"

Q_DECLARE_METATYPE(mpdu);

QT_BEGIN_NAMESPACE
namespace Ui { class main_window; }
QT_END_NAMESPACE

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window(QWidget *parent = nullptr);
    ~main_window();

private slots:
    void device_found(QString name_);
    void remove_device(QString name_);
    void device_status();

    void on_comboBox_channel_currentIndexChanged(int index);
    void on_verticalSlider_valueChanged(int value);
    void mac_protocol_data_units(mpdu mpdu_);

    void on_pushButton_start_clicked();

    void on_pushButton_stop_clicked();

    void on_pushButton_shr_clicked();

    void on_pushButton_test_clicked();

    void on_comboBox_tx_channel_currentIndexChanged(int index);

signals:
    void start(QString name_);
    void stop();
    void set_rx_frequency(long long int rx_frequency_);
    void set_rx_hardwaregain(double rx_hardwaregain_);

    void test_shr();
    void test_test();

private:

    Ui::main_window *ui;

    device *dev;

    void select_device(QString name_);
    QString name_select_device;

    plot *plot_preamble_correlation = nullptr;
    plot *plot_constelation = nullptr;
    plot *plot_sfd_correlation = nullptr;
    plot *plot_sfd_synchronize = nullptr;

    plot *plot_test = nullptr;

};
#endif // MAIN_WINDOW_H
