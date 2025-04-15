#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "csvreader.h"
#include <QFileDialog>
#include "convertors.cpp"

#define dirAngleIdentifier "\"205\""
#define sensorAngleIdentifier "\"233\""
#define GNSSIdentifier "\"203\""

mainWindow::~mainWindow()
{
    delete ui;
}

mainWindow::mainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->customPlot->legend->setVisible(true);
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setPen(QPen(Qt::blue));
    ui->customPlot->graph(0)->setName("Угол датчика");
    ui->customPlot->addGraph();
    ui->customPlot->graph(1)->setPen(QPen(Qt::red));
    ui->customPlot->graph(1)->setName("Дирекционный угол");
    ui->customPlot->xAxis2->setVisible(true);
    ui->customPlot->xAxis2->setTickLabels(false);
    ui->customPlot->yAxis2->setVisible(true);
    ui->customPlot->yAxis2->setTickLabels(false);
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));
}

void mainWindow::parse()
{
    fileName = QFileDialog::getOpenFileName(this,
          tr("Open Image"), "/home/log", tr("Image Files (*.csv)"));
    if (fileName == "")
    { std::cout << "File not selected" << std::endl;return;}
    CSVReader csv(fileName.toStdString(), 7);
    std::vector<CANRecord> CANLog;

    //Вектор исходных данных GNSS меток
    std::vector<CANRecord> GNSSVector;

    //Вектор исходных данных дирекционных углов
    std::vector<CANRecord> firstAngleVector;

    //Вектор исходных данных углов датчика
    std::vector<CANRecord> secondAngleVector;

    //Чтение лога
    CANLog = csv.readAll();

    //Вектор временных меток
    std::vector<uint32_t> timeVector;

    //Вектор углов датчика
    std::vector<int16_t> sensorAngleVector;

    //Вектор дирекционных углов
    std::vector<float> dirAngleVector;

    int indexOfFirstStationaryState = -1;
    int firstIndexCount = 0;
    for(CANRecord CAN: CANLog){

        //Парсинг данных угла датчика
        if(CAN.identifier == sensorAngleIdentifier){
            secondAngleVector.push_back(CAN);
            std::vector<std::string> secondAngleString;
            //Разделение строки на байты
            secondAngleString = csv.split_line(CAN.data, ' ');
            std::string hexSecondAngle;
            std::string binSecondAngle;
            int16_t angle;
            //Определение состояния для поиска сообщения с неподвижным состоянием и последующим нахождением смещения угла
            std::string state;
            state = secondAngleString[6];
            if (state == "01\"" and indexOfFirstStationaryState ==-1){ indexOfFirstStationaryState = firstIndexCount;}

            //Для определения временной метки нужны 1-2 байты
            hexSecondAngle = secondAngleString[2] + secondAngleString[1];
            //Перевод в двоичное представление
            binSecondAngle = hexToBin(hexSecondAngle);
            //Перевод из двоичной строки в int16_t и перевод из 0.1 мрадиан в градусы
            angle = binToInt16(binSecondAngle)*180/(10000*3.14159);
            //Добавление в вектор
            sensorAngleVector.push_back(angle);
            firstIndexCount++;
        }

        //Парсинг данных дирекционного угла
        if(CAN.identifier == dirAngleIdentifier){
            firstAngleVector.push_back(CAN);
            std::vector<std::string> firstAngleString;
            std::string hexFirstAngle;
            std::string binFirstAngle;
            float angle;
            //Разделение строки на байты
            firstAngleString = csv.split_line(CAN.data, ' ');
            //Для опрделения дирекционного угла нужны 1-4 байты
            hexFirstAngle = firstAngleString[4]+firstAngleString[3]+firstAngleString[2]+firstAngleString[1];
            //Перевод в двоичное представление
            binFirstAngle = hexToBin(hexFirstAngle);
            //Перевод из двоичной строки в float
            angle = binToFloat(binFirstAngle);
//                std::cout << string << "||" << hexFirstAngle << "||" << angle << "||" << binFirstAngle << std::endl;
            //Добавление в вектор
            dirAngleVector.push_back(angle);
        }

        //Парсинг данных GNSS меток
        if(CAN.identifier == GNSSIdentifier){
            GNSSVector.push_back(CAN);
            std::vector<std::string> GNSSString;
            std::string messageNum;
            //Разделение строки на байты
            GNSSString = csv.split_line(CAN.data, ' ');
            messageNum = GNSSString[0];
            //Отбор нужных сообщений
            if (messageNum == "\"01"){
                std::string hexGNSS;
                std::string binGNSS;
                uint32_t GNSS;
                //Для определения временной метки нужны 1-4 байты
                hexGNSS = GNSSString[4] + GNSSString[3] + GNSSString[2] + GNSSString[1];
                //Перевод в двоичное представление
                binGNSS = hexToBin(hexGNSS);
                //Перевод из двоичной строки в uint32_t
                GNSS = binToUint32(binGNSS);
//                    std::cout << string << "||" << GNSS << std::endl;
                //Добавление в вектор
                timeVector.push_back(GNSS);
            }
        }
    }

    //Привязка параметров к временным меткам GNSS

    //Вектор временных меток с дробной частью секунды
    std::vector<float> timeVectorExtended;

    //Расширенный вектор диррекционных углов
    std::vector<float> dirAngleVectorExtended;

    int count = 0;
    for(CANRecord CAN: secondAngleVector)
    {
        std::string millisec;

        //Отделение из локального времени миллисекунды
        millisec = csv.split_line(CAN.time, '.')[1];

        //удаление кавычек
        millisec.pop_back();

        //Присоединение миллисекунд к GNSS меткам
        timeVectorExtended.push_back(static_cast<float>(std::stod(millisec)/1000 + timeVector[count/10]));

        //Добавление в вектор
        dirAngleVectorExtended.push_back(dirAngleVector[count/10]);
        count++;
    }

    //удаление "мусорных" данных из векторов
    while(timeVectorExtended.size()%10!=0){
        timeVectorExtended.pop_back();
        dirAngleVectorExtended.pop_back();
        sensorAngleVector.pop_back();
    }

    //Экспорт обработанных данных в TXT
    std::ofstream fout;
    fout.open( "Ugol.txt");
    fout << "Time\tUgol(grad)\n";
    for (int i=0; i<timeVectorExtended.size(); i++){
        fout << std::setprecision(5) << std::fixed << timeVectorExtended[i] << "\t" << sensorAngleVector[i] << std::endl;
    }
    fout.close();
    fout.open( "dirUgol.txt");
    fout << "Time\tdirUgol(grad)\n";
    for (int i=0; i<timeVectorExtended.size(); i++){
        fout << std::setprecision(5) << std::fixed << timeVectorExtended[i] << "\t" << dirAngleVectorExtended[i] << std::endl;
    }
    fout.close();

    //Построение графиков сравнения углов

    //Учет сдвига угла и перевод данных дирекционного угла в диапазон [-180;180]
    double offset = dirAngleVector[indexOfFirstStationaryState] - sensorAngleVector[indexOfFirstStationaryState];
    std::vector<float> dirAngleVectorShifted;
    for(auto i: dirAngleVectorExtended){
        if(i-offset<-180){dirAngleVectorShifted.push_back(360 + i - offset);}
        else if(i-offset>180) {dirAngleVectorShifted.push_back(-360 + i - offset);}
        else{dirAngleVectorShifted.push_back(i - offset);}
    }
    ui->customPlot->graph(0)->setData(QVector<double>(timeVectorExtended.begin(), timeVectorExtended.end()),
                                      QVector<double>(sensorAngleVector.begin(), sensorAngleVector.end()));
    ui->customPlot->graph(1)->setData(QVector<double>(timeVectorExtended.begin(), timeVectorExtended.end()),
                                      QVector<double>(dirAngleVectorShifted.begin(), dirAngleVectorShifted.end()));
    ui->customPlot->graph(0)->rescaleAxes();
    ui->customPlot->graph(1)->rescaleAxes(true);
    double min = *std::min_element(timeVectorExtended.begin(),timeVectorExtended.end());
    double max = *std::max_element(timeVectorExtended.begin(),timeVectorExtended.end());
    ui->customPlot->xAxis->setRange(min, max);
    ui->customPlot->yAxis->setRange(-180, 180);
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->customPlot->replot();
}



void mainWindow::browse()
{
    //Считывание информации из .csv файла
    fileName = QFileDialog::getOpenFileName(this,
          tr("Open Image"), "/home/log", tr("Image Files (*.csv)"));
    CSVReader csv(fileName.toStdString(), 7);
    //Вектор исходных данных GNSS меток
    std::vector<std::vector<std::string>> GNSSVector;
    //Вектор исходных данных дирекционных углов
    std::vector<std::vector<std::string>> firstAngleVector;
    //Вектор исходных данных углов датчика
    std::vector<std::vector<std::string>> secondAngleVector;
    //Чтение лога и запись сообщений в соответствующие векторы
    GNSSVector = csv.readAllByKey("\"203\"",1);
    firstAngleVector = csv.readAllByKey("\"205\"",1);
    secondAngleVector = csv.readAllByKey("\"233\"",1);
    //Вектор временных меток
    std::vector<uint32_t> timeVector;
    //Вектор углов датчика
    std::vector<int16_t> sensorAngleVector;
    //Вектор дирекционных углов
    std::vector<float> dirAngleVector;
    int index;
    int indexOfFirstStationaryState = -1;
    int firstIndexCount = 0;
    //Парсинг данных угла датчика
    for(std::vector<std::string> vec: secondAngleVector)
    {
        index = 0;
        for(std::string string: vec){
            index++;
            if(index == 5){
                std::vector<std::string> secondAngleString;
                //Разделение строки на байты
                secondAngleString = csv.split_line(string, ' ');
                std::string hexSecondAngle;
                std::string binSecondAngle;
                int16_t angle;
                //Определение состояния для поиска сообщения с неподвижным состоянием и последующим нахождением смещения угла
                std::string state;
                state = secondAngleString[6];
                if (state == "01\"" and indexOfFirstStationaryState ==-1){ indexOfFirstStationaryState = firstIndexCount;}

                //Для определения временной метки нужны 1-2 байты
                hexSecondAngle = secondAngleString[2] + secondAngleString[1];
                //Перевод в двоичное представление
                binSecondAngle = hexToBin(hexSecondAngle);
                //Перевод из двоичной строки в int16_t и перевод из 0.1 мрадиан в градусы
                angle = binToInt16(binSecondAngle)*180/(10000*3.14159);
                //Добавление в вектор
                sensorAngleVector.push_back(angle);
                firstIndexCount++;
            }
        }
    }
    //Парсинг данных дирекционного угла
    for(std::vector<std::string> vec: firstAngleVector)
    {
        index = 0;
        for(std::string string: vec){
            index++;
            if(index == 5){
                std::vector<std::string> firstAngleString;
                std::string hexFirstAngle;
                std::string binFirstAngle;
                float angle;
                //Разделение строки на байты
                firstAngleString = csv.split_line(string, ' ');
                //Для опрделения дирекционного угла нужны 1-4 байты
                hexFirstAngle = firstAngleString[4]+firstAngleString[3]+firstAngleString[2]+firstAngleString[1];
                //Перевод в двоичное представление
                binFirstAngle = hexToBin(hexFirstAngle);
                //Перевод из двоичной строки в float
                angle = binToFloat(binFirstAngle);
//                std::cout << string << "||" << hexFirstAngle << "||" << angle << "||" << binFirstAngle << std::endl;
                //Добавление в вектор
                dirAngleVector.push_back(angle);
            }
        }
    }
    //Парсинг данных GNSS меток
    for(std::vector<std::string> vec: GNSSVector)
    {
        index = 0;
        for(std::string string: vec){
            index++;
            if(index == 5){
                std::vector<std::string> GNSSString;
                std::string messageNum;
                //Разделение строки на байты
                GNSSString = csv.split_line(string, ' ');
                messageNum = GNSSString[0];
                //Отбор нужных сообщений
                if (messageNum == "\"01"){
                    std::string hexGNSS;
                    std::string binGNSS;
                    uint32_t GNSS;
                    //Для определения временной метки нужны 1-4 байты
                    hexGNSS = GNSSString[4] + GNSSString[3] + GNSSString[2] + GNSSString[1];
                    //Перевод в двоичное представление
                    binGNSS = hexToBin(hexGNSS);
                    //Перевод из двоичной строки в uint32_t
                    GNSS = binToUint32(binGNSS);
//                    std::cout << string << "||" << GNSS << std::endl;
                    //Добавление в вектор
                    timeVector.push_back(GNSS);
                }
                else{break;}
            }
        }
    }


    //Привязка параметров к временным меткам GNSS
    //Вектор временных меток с дробной частью секунды
    std::vector<float> timeVectorExtended;
    //Расширенный вектор диррекционных углов
    std::vector<float> dirAngleVectorExtended;
    int count = 0;
    for(std::vector<std::string> vec: secondAngleVector)
    {
        index = 0;
        std::string millisec;
        for(std::string string: vec){
            index++;
            if(index == 1){
                //Отделение из локального времени миллисекунды
                millisec = csv.split_line(string, '.')[1];
                //удаление кавычек
                millisec.pop_back();
                //Присоединение миллисекунд к GNSS меткам
                timeVectorExtended.push_back(static_cast<float>(std::stod(millisec)/1000 + timeVector[count/10]));
                //Добавление в вектор
                dirAngleVectorExtended.push_back(dirAngleVector[count/10]);
                count++;
                break;
            }
        }
    }
    //удаление "мусорных" данных из векторов
    while(timeVectorExtended.size()%10!=0){
        timeVectorExtended.pop_back();
        dirAngleVectorExtended.pop_back();
        sensorAngleVector.pop_back();
    }

    //Экспорт обработанных данных в TXT
    std::ofstream fout;
    fout.open( "Ugol.txt");
    fout << "Time\tUgol(grad)\n";
    for (int i=0; i<timeVectorExtended.size(); i++){
        fout << std::setprecision(5) << std::fixed << timeVectorExtended[i] << "\t" << sensorAngleVector[i] << std::endl;
    }
    fout.close();
    fout.open( "dirUgol.txt");
    fout << "Time\tdirUgol(grad)\n";
    for (int i=0; i<timeVectorExtended.size(); i++){
        fout << std::setprecision(5) << std::fixed << timeVectorExtended[i] << "\t" << dirAngleVectorExtended[i] << std::endl;
    }
    fout.close();

    //Построение графиков сравнения углов
    //Учет сдвига угла
    double offset = dirAngleVector[indexOfFirstStationaryState] - sensorAngleVector[indexOfFirstStationaryState];
    std::vector<float> dirAngleVectorShifted;
    for(auto i: dirAngleVectorExtended){
        if(i-offset<-180){dirAngleVectorShifted.push_back(360 + i - offset);}
        else if(i-offset>180) {dirAngleVectorShifted.push_back(-360 + i - offset);}
        else{dirAngleVectorShifted.push_back(i - offset);}
    }
    ui->customPlot->graph(0)->setData(QVector<double>(timeVectorExtended.begin(), timeVectorExtended.end()),
                                      QVector<double>(sensorAngleVector.begin(), sensorAngleVector.end()));
    ui->customPlot->graph(1)->setData(QVector<double>(timeVectorExtended.begin(), timeVectorExtended.end()),
                                      QVector<double>(dirAngleVectorShifted.begin(), dirAngleVectorShifted.end()));
    ui->customPlot->graph(0)->rescaleAxes();
    ui->customPlot->graph(1)->rescaleAxes(true);
    double min = *std::min_element(timeVectorExtended.begin(),timeVectorExtended.end());
    double max = *std::max_element(timeVectorExtended.begin(),timeVectorExtended.end());
    ui->customPlot->xAxis->setRange(min, max);
    ui->customPlot->yAxis->setRange(-180, 180);
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    update();
}

