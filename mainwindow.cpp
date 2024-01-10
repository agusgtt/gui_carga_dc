#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    Cont_Items=0;
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_Update_unit(int dato)
{
    if(dato==0){
        ui->label_input_val->setText("[A]");
    }
    else if(dato==1){
        ui->label_input_val->setText("[W]");
    }
    else if(dato==2){
        ui->label_input_val->setText("[R]");
    }
}

void MainWindow::slot_add()
{
    //leemos los datos
    int modo= ui->comboBox_modo->currentIndex();//0 para CC/1 para CP/2 para CR
    QString input=ui->lineEdit_input_val->text();
    QString unit_input=ui->label_input_val->text();
    QString time=ui->lineEdit_input_time->text();
    int unit_time=ui->comboBox_time_unit->currentIndex();//0 para seg/1 para min

    if(Cont_Items<10 && (input.toFloat()>0) && (time.toFloat()>0)){
        //cargamos en la tabla
        if(modo==0){
            ui->tableWidget->setItem(Cont_Items,0,new QTableWidgetItem(CC));
        }else if(modo==1){
            ui->tableWidget->setItem(Cont_Items,0,new QTableWidgetItem(CP));
        }else if(modo==2){
            ui->tableWidget->setItem(Cont_Items,0,new QTableWidgetItem(CR));
        }
        ui->tableWidget->setItem(Cont_Items,1,new QTableWidgetItem(input));
        ui->tableWidget->setItem(Cont_Items,2,new QTableWidgetItem(unit_input));
        ui->tableWidget->setItem(Cont_Items,3,new QTableWidgetItem(time));
        if(unit_time==0){
            ui->tableWidget->setItem(Cont_Items,4,new QTableWidgetItem("[seg]"));
        }else{
            ui->tableWidget->setItem(Cont_Items,4,new QTableWidgetItem("[min]"));
        }
        Cont_Items++;
    }
}

void MainWindow::slot_validate_val()
{
    QString modo_str;
    float dato_float;
    bool dato_valido;
    dato_float=ui->lineEdit_input_val->text().toFloat(&dato_valido);
    modo_str=ui->label_input_val->text();
    if(dato_valido){
        if(modo_str=="[A]" && dato_float>20){
            ui->lineEdit_input_val->setText("");
        }else if(modo_str=="[W]" && dato_float>150){
            ui->lineEdit_input_val->setText("");
        }else if(modo_str=="[R]" && dato_float>1000){
            ui->lineEdit_input_val->setText("");
        }
    }else{
         ui->lineEdit_input_val->setText("");
    }
}


