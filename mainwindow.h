#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>

#define CC "C.Current"
#define CP "C.Power"
#define CR "C.Resist."

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    uint Cont_Items;
    uint Tarea_actual;
    uint Cont_tarea_actual;
    int Selected_row;
    bool tabla_clean;

    //banderas control de flujo
    bool disp_ready;
    bool disp_conectado;

    //com serie
    QSerialPort *PuertoUSB;
    void abrirUSB();
    void cerrarUSB();
    void leerDatosSerial();

    //timer
    QTimer *timer_100_ms;
    void Iniciar_tarea(int tarea);
    void LimpiarColor();

    //contadores tareas
    uint total_time_s;//acumulador de las tareas
    uint remaining_time_s;//cont global de stop
    uint Cont_timer_1seg;//timer del reloj
    uint Cont_request;//contador para pedir al micro

    ~MainWindow();

private:
    Ui::MainWindow *ui;

public slots:
    void slot_Update_unit(int dato);
    void slot_add();
    void slot_delete();
    void slot_validate_val();
    void slot_validate_time();
    void slot_cambio_select();
    void slot_delete_all();
    void slot_run();
    void slot_stop();
    void slot_manejo_timer_100ms();


};
#endif // MAINWINDOW_H
