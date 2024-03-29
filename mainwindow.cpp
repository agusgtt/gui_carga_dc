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
    flag_waiting_micro_conf=false;
    Cont_request=0;
    num_medicion=0;

    PuertoUSB = new QSerialPort(this);
    PuertoUSB->setBaudRate(QSerialPort::Baud9600);
    PuertoUSB->setDataBits(QSerialPort::Data8);
    PuertoUSB->setParity(QSerialPort::NoParity);
    PuertoUSB->setStopBits(QSerialPort::OneStop);

    disp_ready=false;
    disp_detectado=false;
    disp_conectado=false;
    flag_usb_ready=false;
    band_run=false;
    archivo_abierto=false;

    ui->setupUi(this);
    timer_100_ms = new QTimer(this);
    timer_100_ms->setInterval(100);//seteado en 100ms
    connect(timer_100_ms, &QTimer::timeout, this, &MainWindow::slot_manejo_timer_100ms);
    connect(PuertoUSB, &QSerialPort::errorOccurred, this, &MainWindow::ErrorUSB);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    timer_100_ms->start();
}

void MainWindow::escribir_en_txt(QString modo,int set_point,int tension, int corriente)
{

    QString potencia =  QString("%1").arg((float)(corriente*tension)/10000, 0, 'f', 1);
    *streamArchivo<<num_medicion<<","<<(float)num_medicion/2<<",C"<<modo<<","<<set_point<<",";
    *streamArchivo<<(float)tension/100<<","<<(float)corriente/100<<",";
    *streamArchivo<<potencia<<","<<(float)tension/(float)corriente<<"\n";
    num_medicion++;
}

QString MainWindow::formato_h_m_s(uint segundos)
{
    uint horas = segundos / 3600;
    uint min = (segundos % 3600) / 60;
    uint seg = segundos % 60;

    // Formatear la cadena
    return QString("%1:%2:%3").arg(horas, 2, 10, QChar('0'))
        .arg(min, 2, 10, QChar('0'))
        .arg(seg, 2, 10, QChar('0'));
}

QString MainWindow::generar_nombreArchivo()
{
    QDateTime dateTime = QDateTime::currentDateTime();
        // formato fecha y hora AAAAMMDD_HHMMSS
    QString nombreArchivo = QString("CargaDC_datalog_%1.txt").arg(dateTime.toString("yyyyMMdd_HHmmss"));

    return nombreArchivo;
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
        //PuertoUSB->write("$DIS");
        //PuertoUSB->waitForBytesWritten(10000);
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
    //qDebug()<<"Input usb: "<<datos;
    if(datos_str.startsWith("$USB")){
        PuertoUSB->write("$OK#");
        if(!flag_usb_ready){
            flag_usb_ready=true;
            ui->pushButton_run->setEnabled(true);//cambiar al recibir paquetes
            ui->label_status->setText("Ready");
            ui->label_status->setStyleSheet("color: green; font-weight: bold;");
        }
        else{
            flag_usb_ready=false;
            ui->pushButton_run->setDisabled(true);
        }
    }
    else if(datos_str.startsWith("$END")){
        flag_usb_ready=false;
        ui->pushButton_run->setDisabled(true);
    }
    else if(datos_str.startsWith("$D")){//puede ser que no quiera guardar los datos

        //seteo en cero algun timer de control
        if(archivo_abierto){//si guardo los datos
            QStringList listaDatos = datos_str.split(',');
            escribir_en_txt(listaDatos[1],listaDatos[2].toInt(),listaDatos[3].toInt(),listaDatos[4].toInt());
        }
    }//fin de los case

}//fin leer

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
        QString str_aux;
        QString solicitud;
        int valor;

        for(int i=0; i<5;i++){
            ui->tableWidget->item(tarea,i)->setBackground(QColor(243, 220, 25));//el gris es 245/245/245
        }
        if(ui->tableWidget->item(tarea,4)->text()=="[min]"){
            Cont_tarea_actual=60*ui->tableWidget->item(tarea,3)->text().toInt();
        }else{
            Cont_tarea_actual=ui->tableWidget->item(tarea,3)->text().toInt();
        }

        if(ui->tableWidget->item(tarea,0)->text()=="C.Current"){
            //modo="C";
            valor = 100*ui->tableWidget->item(tarea,1)->text().toFloat();
            //valor = 100*ui->tableWidget->item(tarea,1)->text().toInt();
            solicitud="$C,C,";
        }else if(ui->tableWidget->item(tarea,0)->text()=="C.Power"){
            //modo="P";
            valor = 10*ui->tableWidget->item(tarea,1)->text().toFloat();
            solicitud="$C,P,";
        }else if(ui->tableWidget->item(tarea,0)->text()=="C.Resist."){
            //modo="R";
            valor = ui->tableWidget->item(tarea,1)->text().toFloat();
            solicitud="$C,R,";
        }
        tabla_clean=false;

        str_aux = QString("%1").arg((int)valor, 4, 10, QLatin1Char('0'));
        solicitud+=str_aux;
        solicitud+="#";
        PuertoUSB->write(solicitud.toUtf8());

        qDebug()<<"sol enviada:"<<solicitud<<"\n";
        int tiempoEspera = 10;  // 1000 milisegundos = 1 segundo
        QThread::msleep(tiempoEspera);
        //$C,M,VVVV#
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

void MainWindow::generar_archivo()
{
    archivoActual = new QFile(generar_nombreArchivo());
    if (archivoActual->open(QIODevice::WriteOnly | QIODevice::Text)) {
        streamArchivo = new QTextStream(archivoActual);
        *streamArchivo << "n.medicion,T[s],modo,conf,adc_V[V],adc_C[A],Pot_cal[W],Res_cal[ohm]\n";//formato de columnas
        archivo_abierto=true;
        num_medicion=0;
    }
}

MainWindow::~MainWindow()
{
    PuertoUSB->write("$DIS");
    PuertoUSB->waitForBytesWritten(10000);
    cerrarUSB();
    qDebug() << "Destructor de MainWindow llamado";
    cerrar_archivo();
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
        for (int columna = 0; columna < 5; ++columna) {
            QTableWidgetItem *item = ui->tableWidget->item(Cont_Items, columna);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
        Cont_Items++;
    }
    ui->label_7->setText(formato_h_m_s(total_time_s));
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
    ui->label_7->setText(formato_h_m_s(total_time_s));
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
        ui->label_7->setText(formato_h_m_s(remaining_time_s));
        band_run=true;
        Tarea_actual=0;
        Iniciar_tarea(Tarea_actual);
        ui->pushButton_stop->setEnabled(true);
        if(ui->checkBox_almacenar->isChecked()){
            generar_archivo();
        }

    }

}

void MainWindow::slot_stop()
{
    ui->pushButton_add->setEnabled(true);
    ui->pushButton_del->setEnabled(true);
    ui->pushButton_delete_all->setEnabled(true);
    ui->pushButton_run->setEnabled(true);
    ui->checkBox_almacenar->setEnabled(true);
    Cont_timer_1seg=0;
    band_run=false;
    ui->pushButton_stop->setDisabled(true);
    if(archivo_abierto){
        cerrar_archivo();
    }
    //QString solicitud="$C,C,0000#";
    //PuertoUSB->write(solicitud.toUtf8());
}

void MainWindow::slot_manejo_timer_100ms()
{
    if(band_run){
        Cont_timer_1seg++;
        Cont_request++;
        if(Cont_timer_1seg>9){//paso 1 segundo
            Cont_timer_1seg=0;


            //Control timer global
            remaining_time_s--;

            ui->label_7->setText(formato_h_m_s(remaining_time_s));//update display
            //ui->label_7->setText(QString::number(remaining_time_s));//update display
            if(remaining_time_s==0){
                slot_stop();
                QString solicitud="$C,C,0000#";//esto estaba dentro de stop
                PuertoUSB->write(solicitud.toUtf8());
                LimpiarColor();
                ui->label_7->setText("Fin");
                tabla_clean=true;
            }
            //control timer task
            if(Cont_tarea_actual>0){
                Cont_tarea_actual--;
            }
            //llama otra tarea
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
            PuertoUSB->write(solicitud.toUtf8());
            Cont_request=0;
        }
    }//fin if band_run
    else if(!flag_usb_ready){
        if(!disp_detectado){
            //buscar en los com si esta el dispositivo
            ui->label_status->setText("No Detectado");
            ui->label_status->setStyleSheet("color: red; font-weight: bold;");
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
            ui->label_status->setStyleSheet("color: black; font-weight: bold;");
            //que espere a que el micro le mande el ok
        }

    }
}

void MainWindow::ErrorUSB()
{
    cerrarUSB();
    flag_usb_ready=false;
    disp_detectado=false;
    disp_conectado=false;
    slot_stop();
    ui->pushButton_run->setDisabled(true);
    //ui->label_status->setText("No Detectado");
    //idPuerto=QString();
    qDebug()<<"\nERROR\n";
}

void MainWindow::cerrar_archivo()
{
    if (archivoActual && archivoActual->isOpen()) {
        archivoActual->close();
        delete streamArchivo;
        delete archivoActual;
        archivo_abierto = false;
    }
}

