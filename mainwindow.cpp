#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "csvreader.h"
#include <QFileDialog>
#include "convertors.cpp"

#define DIR_ANGLE_IDENTIFIER "\"205\""
#define SENSOR_ANGLE_IDENTIFIER "\"233\""
#define GNSS_IDENTIFIER "\"203\""

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
        if(CAN.identifier == SENSOR_ANGLE_IDENTIFIER){
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
        if(CAN.identifier == DIR_ANGLE_IDENTIFIER){
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
        if(CAN.identifier == GNSS_IDENTIFIER){
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
    float currentTime;
    float previousTime = 999999999.0;
    int count = 0;
    for(CANRecord CAN: secondAngleVector)
    {
        std::string millisec;

        //Отделение из локального времени миллисекунды
        millisec = csv.split_line(CAN.time, '.')[1];

        //удаление кавычек
        millisec.pop_back();
        currentTime = static_cast<float>(std::stod(millisec)/1000 + timeVector[count/10]);
        if(currentTime < previousTime){ currentTime += 1; } // закоментировать при необходимости
        //Присоединение миллисекунд к GNSS меткам
        timeVectorExtended.push_back(currentTime);
        previousTime = currentTime;
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
    fout.open( "sensorUgol.txt");
    fout << "Time\t\tUgol(grad)\n";
    for (int i=0; i<timeVectorExtended.size(); i++){
        fout << std::setprecision(5) << std::fixed << timeVectorExtended[i] << "\t" << sensorAngleVector[i] << std::endl;
    }
    fout.close();
    ui->textEdit->append("Данные угла датчика и GNSS меток успешно сохраненны в файл sensorUgol.txt");
    fout.open( "dirUgol.txt");
    fout << "Time\t\tdirUgol(grad)\n";
    for (int i=0; i<timeVectorExtended.size(); i++){
        fout << std::setprecision(5) << std::fixed << timeVectorExtended[i] << "\t" << dirAngleVectorExtended[i] << std::endl;
    }
    fout.close();
    ui->textEdit->append("Данные дирекционного угла и GNSS меток успешно сохраненны в файл dirUgol.txt");

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


