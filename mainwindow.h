#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    ~MainWindow();

private:
    Ui::MainWindow *ui;

public slots:
    void slot_Update_unit(int dato);
    void slot_add();
    void slot_validate_val();
};
#endif // MAINWINDOW_H
