#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QThread>

#include "callback_ui.h"
#include "higher_layer.h"
#include "device/device.h"
#include "plot/plot.h"

Q_DECLARE_METATYPE(scan_type_t);
Q_DECLARE_METATYPE(mpdu_analysis_t);
Q_DECLARE_METATYPE(uint8_t);
Q_DECLARE_METATYPE(uint16_t);
Q_DECLARE_METATYPE(uint32_t);
Q_DECLARE_METATYPE(uint64_t);

QT_BEGIN_NAMESPACE
namespace Ui { class main_window; }
QT_END_NAMESPACE

class main_window : public QMainWindow,
                    public ui_callback
{
    Q_OBJECT

public:
    main_window(QWidget *parent = nullptr);
    ~main_window();

private slots:
    void device_found(QString name_);
    void device_open();
    void remove_device(QString name_);
    void device_status(QString status_);

    void preamble_correlaion(int len_plot_, std::complex<float>* plot_buffer_);
    void sfd_correlation(int len_plot_, std::complex<float>* plot_buffer_);
    void sfd_synchronize(int len_plot_, std::complex<float>* plot_buffer_);
    void constelation(int len_plot_, std::complex<float>* plot_buffer_);

    void on_comboBox_channel_currentIndexChanged(int index);
    void on_verticalSlider_valueChanged(int value);
    void mac_protocol_data_units(mpdu_analysis_t *mpdu_);

    void on_pushButton_start_clicked();

    void on_pushButton_stop_clicked();

    void on_pushButton_shr_clicked();

    void on_pushButton_test_clicked();

    void on_comboBox_tx_channel_currentIndexChanged(int index);

    void on_pushButton_scan_clicked();

    void show_channel_scan(int channel_, bool done_);
    void show_energy_detect(uint8_t channel_, uint8_t energy_level_);
    void show_active_detect(uint8_t channel_,  uint16_t pan_id_, uint64_t address_);

    void detach_tab_receiver();
    void attach_tab_receiver();

    void on_pushButton_start_new_pan_clicked();

signals:
    void open_device(QString name_);
    void device_start(QString name_);
    void device_stop();
    void finished();
    void set_rx_frequency(long long int rx_frequency_);
    void set_rx_hardwaregain(double rx_hardwaregain_);
    void start_scan(scan_type_t scan_type_);
    void start_new_pan();

    void send_channel_scan(int channel_, bool done_);
    void send_energy_detect(uint8_t channel_, uint8_t energy_level);
    void send_active_scan(uint8_t channel_,  uint16_t pan_id_, uint64_t address_);



    void test_shr();
    void test_test();

private:

    Ui::main_window *ui;

    higher_layer *hig_layer;
    QThread *thread_hig_layer;

    QThread *thread_dev;
    device *dev;
    void select_device(QString name_);
    QString name_select_device;
    QStringList show_status;

    plot *plot_preamble_correlation = nullptr;
    plot *plot_constelation = nullptr;
    plot *plot_sfd_correlation = nullptr;
    plot *plot_sfd_synchronize = nullptr;

    QLayout *layout_tab_receiver;
    QDialog *dialog_receiver;

    void channel_scan(int channel_, bool done_);
    void energy_detect(uint8_t channel_, uint8_t energy_level_);
    void active_detect(uint8_t channel_,  uint16_t pan_id_, uint64_t address_);


    plot *plot_test = nullptr;

};
#endif // MAIN_WINDOW_H
