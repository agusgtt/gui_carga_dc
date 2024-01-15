#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    Cont_Items=0;
    Selected_row=-1;
    total_time_s=0;
    Cont_timer_1seg=0;
    remaining_time_s=0;
    tabla_clean=true;
    flag_waiting_micro=false;
    Cont_request=0;

    PuertoUSB = new QSerialPort(this);
    //PuertoUSB->setPortName("COM4"); // Cambia "COMx" por el nombre del puerto serial real
    PuertoUSB->setBaudRate(QSerialPort::Baud9600);
    PuertoUSB->setDataBits(QSerialPort::Data8);
    PuertoUSB->setParity(QSerialPort::NoParity);
    PuertoUSB->setStopBits(QSerialPort::OneStop);

    disp_ready=false;
    disp_detectado=false;
    disp_conectado=false;
    band_run=false;


    //encontrarDispositivoUSB();
    //abrirUSB();//mover al timer
    ui->setupUi(this);
    timer_100_ms = new QTimer(this);
    timer_100_ms->setInterval(100);//seteado en 100ms
    connect(timer_100_ms, &QTimer::timeout, this, &MainWindow::slot_manejo_timer_100ms);
    connect(PuertoUSB, &QSerialPort::errorOccurred, this, &MainWindow::ErrorUSB);
    timer_100_ms->start();
}

void MainWindow::escribir_en_txt(int tension, int corriente)
{
    qDebug()<<"Dato1: "<<tension<<"\nDato2: "<<corriente<<"\n";
}

void MainWindow::abrirUSB(QString Puerto_com)
{
    PuertoUSB->setPortName(Puerto_com);
    if (PuertoUSB->open(QIODevice::ReadWrite)) {
        // El puerto serial está abierto
        connect(PuertoUSB, &QSerialPort::readyRead, this, &MainWindow::leerDatosSerial);
        disp_conectado=true;
    } else {
        // Error al abrir el puerto serial
        disp_detectado=false;
        qDebug() << "abrirUSB():Error al abrir el puerto serial: " << PuertoUSB->errorString();
    }
}

void MainWindow::cerrarUSB()
{
    if (PuertoUSB->isOpen()) {
        // Desconectar la señal readyRead antes de cerrar el puerto
        disconnect(PuertoUSB, &QSerialPort::readyRead, this, &MainWindow::leerDatosSerial);

        // Cerrar el puerto serial
        PuertoUSB->close();
    }
}

void MainWindow::leerDatosSerial()
{
    QByteArray datos = PuertoUSB->readAll();
    QString datos_str = QString::fromUtf8(datos);
    qDebug()<<"Input usb: "<<datos;
    if(datos_str.startsWith("$R")){
        flag_waiting_micro=false;
    }
    else if(datos_str.startsWith("$D")){//puede ser que no quiera guardar los datos

        //seteo en cero algun timer de control
        if(ui->checkBox_almacenar->isChecked()){//si guardo los datos
            QStringList listaDatos = datos_str.split(',');
            escribir_en_txt(listaDatos[1].toInt(),listaDatos[2].toInt());
        }
        flag_waiting_micro=false;
    }

}

void MainWindow::encontrarDispositivoUSB()
{
    QList<QSerialPortInfo> listaPuertos = QSerialPortInfo::availablePorts();

    qDebug() << "Dispositivos USB disponibles:";

    for (const QSerialPortInfo &info : listaPuertos) {
        qDebug() << "Puerto: " << info.portName();
        qDebug() << "Descripción: " << info.description();
                                       qDebug() << "Fabricante: " << info.manufacturer();
        qDebug() << "ID de producto: " << info.productIdentifier();
        qDebug() << "ID de vendedor: " << info.vendorIdentifier();
        qDebug() << "--------------------------------";
    }
}

QString MainWindow::buscarpuerto(const int idVendedor,const int idProducto)
{
    QList<QSerialPortInfo> listaPuertos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : listaPuertos) {
        if (info.vendorIdentifier() == idVendedor && info.productIdentifier() == idProducto) {
            // Se encontro el dispositivo, devolver el nombre del puerto
            return info.portName();
        }
    }
    // Si no se encuentra el dispositivo, devolver una cadena vacía
    return QString();
}


void MainWindow::Iniciar_tarea(int tarea)
{
    if((uint)tarea<Cont_Items){
        for(int i=0; i<5;i++){
            ui->tableWidget->item(tarea,i)->setBackground(QColor(243, 220, 25));//el gris es 245/245/245
        }
        if(ui->tableWidget->item(tarea,4)->text()=="[min]"){
            Cont_tarea_actual=60*ui->tableWidget->item(tarea,3)->text().toInt();// ojo que sea la columna correcta
        }else{
            Cont_tarea_actual=ui->tableWidget->item(tarea,3)->text().toInt();
        }
        tabla_clean=false;
        //enviar config al micro
        //flag_waiting_micro=true;
        //$C,Mx,Vxxxx#
    }
}

void MainWindow::LimpiarColor()
{
    for(int i=0; i<5;i++){
        for(uint j=0; j<Cont_Items;j++){
            if(j%2==1){
                ui->tableWidget->item(j,i)->setBackground(QColor(245, 245, 245));//el gris es 245/245/245
            }else{
                ui->tableWidget->item(j,i)->setBackground(QColor(255, 255, 255));//el gris es 245/245/245
            }
        }
    }
}

MainWindow::~MainWindow()
{
    cerrarUSB();
    timer_100_ms->stop();
    delete timer_100_ms;
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
    if(!tabla_clean){
        LimpiarColor();
        tabla_clean=true;
    }
    //leemos los datos
    int modo= ui->comboBox_modo->currentIndex();//0 para CC/1 para CP/2 para CR
    QString input=ui->lineEdit_input_val->text();
    QString unit_input=ui->label_input_val->text();
    QString time=ui->lineEdit_input_time->text();
    int unit_time=ui->comboBox_time_unit->currentIndex();//0 para seg/1 para min

    if(Cont_Items<10 && (input.toFloat()>0) && (time.toFloat()>0)){
        //añadimos un renglon
        ui->tableWidget->insertRow(Cont_Items);

        //cargamos en la tabla
        if(modo==0) ui->tableWidget->setItem(Cont_Items,0,new QTableWidgetItem(CC));
        else if(modo==1) ui->tableWidget->setItem(Cont_Items,0,new QTableWidgetItem(CP));
        else if(modo==2) ui->tableWidget->setItem(Cont_Items,0,new QTableWidgetItem(CR));

        ui->tableWidget->setItem(Cont_Items,1,new QTableWidgetItem(input));
        ui->tableWidget->setItem(Cont_Items,2,new QTableWidgetItem(unit_input));
        ui->tableWidget->setItem(Cont_Items,3,new QTableWidgetItem(time));

        if(unit_time==0){
            ui->tableWidget->setItem(Cont_Items,4,new QTableWidgetItem("[seg]"));
            total_time_s=total_time_s+time.toUInt();
        }else{
            ui->tableWidget->setItem(Cont_Items,4,new QTableWidgetItem("[min]"));
            total_time_s=total_time_s+60*time.toUInt();
        }
        Cont_Items++;
    }
    ui->label_7->setText(QString::number(total_time_s));
}

void MainWindow::slot_delete()
{
//hay dos casos, que este seleccionado un renglon y se borre ese
//o que se borre el ultimo elemento de la lista
//int selectedRow = ui->tableWidget->currentRow();
    if(!tabla_clean){
        LimpiarColor();
        tabla_clean=true;
    }

    bool remove_ok = false;
    QString time;
    QString time_unit;
    if(Selected_row>=0){//&& selectedRow<Cont_Items
        time=ui->tableWidget->item(Selected_row,3)->text();
        time_unit=ui->tableWidget->item(Selected_row,4)->text();
        ui->tableWidget->removeRow(Selected_row);
        ui->tableWidget->selectRow(-1);
        ui->tableWidget->clearSelection();
        Cont_Items--;
        Selected_row=-1;
        remove_ok=true;
    }
    else if(Cont_Items>0){
        Cont_Items--;
        time=ui->tableWidget->item(Cont_Items,3)->text();
        time_unit=ui->tableWidget->item(Cont_Items,4)->text();
        ui->tableWidget->removeRow(Cont_Items);
        remove_ok=true;
    }
    if(remove_ok){
        if(time_unit=="[seg]"){
            total_time_s=total_time_s-time.toUInt();
        }else if(time_unit=="[min]"){
            total_time_s=total_time_s-60*time.toUInt();
        }

    }
    ui->label_7->setText(QString::number(total_time_s));
}

void MainWindow::slot_validate_val()
{
    QString modo_str;
    float dato_float;
    bool dato_valido;
    dato_float=ui->lineEdit_input_val->text().toFloat(&dato_valido);
    modo_str=ui->label_input_val->text();
    if(dato_valido && dato_float>=0){
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

void MainWindow::slot_validate_time()
{
    float dato_int;
    bool dato_valido;
    dato_int=ui->lineEdit_input_time->text().toInt(&dato_valido);

    if(!dato_valido || (dato_int>200) || (dato_int<=0)){
       ui->lineEdit_input_time->setText("");
    }
}

void MainWindow::slot_cambio_select()
{
int selectedRow = ui->tableWidget->currentRow();
    Selected_row=selectedRow;
}

void MainWindow::slot_delete_all()
{
    while(Cont_Items>0){
        Cont_Items--;
        ui->tableWidget->removeRow(Cont_Items);
    }
    total_time_s=0;
    ui->label_7->setText(QString::number(total_time_s));
}

void MainWindow::slot_run()
{
    if(!tabla_clean){
        LimpiarColor();
        tabla_clean=true;
    }
    if(Cont_Items>0){//Si no hay items no permite ejecutar
        ui->pushButton_add->setEnabled(false);
        ui->pushButton_del->setEnabled(false);
        ui->pushButton_delete_all->setEnabled(false);
        ui->pushButton_run->setEnabled(false);
        ui->checkBox_almacenar->setEnabled(false);

        //start timer global
        remaining_time_s=total_time_s;
        ui->label_7->setText(QString::number(remaining_time_s));
        //timer_100_ms->start();
        band_run=true;
        Tarea_actual=0;
        Iniciar_tarea(Tarea_actual);
    }

}

void MainWindow::slot_stop()
{
    ui->pushButton_add->setEnabled(true);
    ui->pushButton_del->setEnabled(true);
    ui->pushButton_delete_all->setEnabled(true);
    ui->pushButton_run->setEnabled(true);
    ui->checkBox_almacenar->setEnabled(true);
    //timer_100_ms->stop();
    Cont_timer_1seg=0;
    band_run=false;
    if(remaining_time_s==0){
        ui->label_7->setText("Fin");
    }
    //while quita el color//245/245/245 para el gris

}

void MainWindow::slot_manejo_timer_100ms()
{
    if(band_run){
        Cont_timer_1seg++;
        Cont_request++;
        if(Cont_timer_1seg>9){//paso 1 segundo
            //update contadores
            remaining_time_s--;
            if(Cont_tarea_actual>0){
                Cont_tarea_actual--;
            }
            Cont_timer_1seg=0;
            //update display
            ui->label_7->setText(QString::number(remaining_time_s));
            //Control timer global
            if(remaining_time_s==0){
                slot_stop();
                LimpiarColor();
                tabla_clean=true;
            }
            //control timer task
            if(Cont_tarea_actual<=0 && Tarea_actual<(Cont_Items-1)){
                for(int i=0; i<5;i++){
                    ui->tableWidget->item(Tarea_actual,i)->setBackground(QColor(148, 255, 58));//el gris es 245/245/245

                }
                tabla_clean=false;
                Tarea_actual++;
                Iniciar_tarea(Tarea_actual);
            }

        }//fin cont 1 seg

        //pedir dato al micro
        if(Cont_request>4){//cada 500 ms va a pedir datos del micro, el 5 cambia por
            //pedir dato al micro
            QString solicitud = "$Req#";
            qDebug()<<"solicitud enviada\n";

            if(flag_waiting_micro){
                qDebug()<<"Error de timeout\n";
            }
            PuertoUSB->write(solicitud.toUtf8());
            Cont_request=0;
            flag_waiting_micro=true;
        }
    }//fin if band_run
    else{
        if(!disp_detectado){
            //buscar en los com si esta el dispositivo
            idPuerto=buscarpuerto(idVendedor,idProducto);
            if(!idPuerto.isEmpty()){
                disp_detectado=true;
                //ui->label_status->setText("Detectado");
            }
        }else if(!disp_conectado){
            abrirUSB(idPuerto);
            //esperar hasta que el dispositivo ceda el control
            //activar boton de run en la gui
            //esto deberia estar en el slot de leer segun el mensaje del micro
        }else{
            ui->label_status->setText("Conectado");

            ui->pushButton_run->setEnabled(true);//cambiar al recibir paquetes

            //que espere a que el micro le mande el ok
        }

    }
}

void MainWindow::ErrorUSB()
{
    cerrarUSB();
    disp_detectado=false;
    disp_conectado=false;
    slot_stop();
    ui->pushButton_run->setDisabled(true);
    ui->label_status->setText("No Detectado");
    //idPuerto=QString();
    qDebug()<<"\nERROR\n";
}

