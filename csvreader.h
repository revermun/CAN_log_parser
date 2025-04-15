#ifndef CSVREADER_H
#define CSVREADER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

struct CANRecord{
    std::string time;
    std::string identifier;
    std::string format;
    std::string flags;
    std::string data;
};

class CSVReader
{
public:
    CSVReader(const std::string& filename, unsigned headerLenght);
    ~CSVReader();

    bool is_open();
    void close();
    CANRecord read();
    std::vector<CANRecord> readAll();
    std::vector<std::string> split_line(const std::string &str, char delim);
    //Функция, записывающая в вектор все записи, в которых элемент из определенного столбца соответствует ключу
    std::vector<std::vector<std::string>> readAllByKey(std::string key, int keyColumn);
    operator bool(){
        return is_open();
    }
    unsigned count;
private:
    std::ifstream fin;
    unsigned headerLength;

};

#endif // CSVREADER_H
