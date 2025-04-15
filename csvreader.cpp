#include <csvreader.h>

std::vector<std::string> CSVReader::split_line(const std::string &str, char delim) {

    std::vector<std::string> tokens;

    if (!str.empty()) {
        size_t start = 0, end;
        do {
            tokens.push_back(str.substr(start, (str.find(delim, start) - start)));
            end = str.find(delim, start);
            start = end + 1;
        } while (end != std::string::npos);
    }

    return tokens;
}

bool isBlankLine(char const* line)
{
    for ( char const* cp = line; *cp; ++cp )
    {
        if ( !isspace(*cp) ) return false;
    }
    return true;
}

bool isBlankLine(std::string const& line)
{
   return isBlankLine(line.c_str());
}

CSVReader::CSVReader(const std::string& filename, unsigned headerLength)
{
    fin.open(filename);
    this->headerLength = headerLength;
    count = 0;
}

CSVReader::~CSVReader()
{
    fin.close();
}

bool CSVReader::is_open()
{
    return fin.is_open();
}

CANRecord CSVReader::read()
{

    std::string line;
    std::getline(fin, line);
    CANRecord CAN;
    if(headerLength != 0){
        --headerLength;
        CAN.time = "";
        CAN.identifier = "";
        CAN.format = "";
        CAN.flags = "";
        CAN.data = "";
        return CAN;}
    if (fin.eof() or (line.empty() or (line.size()==0))) {
        //pass
        CAN.time = "";
        CAN.identifier = "";
        CAN.format = "";
        CAN.flags = "";
        CAN.data = "";
        return CAN;
    }
    else
    {
        auto tokens = split_line(line, ';');

        std::string CANTime = tokens[0];
        std::string CANIdentifier = tokens[1];
        std::string CANFormat = tokens[2];
        std::string CANFlags = tokens[3];
        std::string CANData = tokens[4];
        CAN.time = CANTime;
        CAN.identifier = CANIdentifier;
        CAN.format = CANFormat;
        CAN.flags = CANFlags;
        CAN.data = CANData;
        return CAN;
    }
}

std::vector<CANRecord> CSVReader::readAll()
{

    std::vector<CANRecord> CANRecords;
        while (!fin.eof())
        {
            CANRecord CAN = read();
            if (CAN.time==""){ }
            else  CANRecords.push_back(CAN);
        }
        fin.seekg(0);
        return CANRecords;
}

std::vector<std::vector<std::string>> CSVReader::readAllByKey(std::string key, int keyColumn)
{
    std::vector<std::vector<std::string>> dataArray;
    int headerLength = this->headerLength;
    while (!fin.eof())
    {
        std::string line;
        std::getline(fin, line);
        if(isBlankLine(line) or line.length() == 0 or line.empty() or fin.eof()){
            //pass
        }
        else
        {
            std::vector<std::string> data;
            if(headerLength != 0){ --headerLength;}
            else{
                std::vector<std::string> tokens = split_line(line, ';');
                if (tokens[keyColumn] == key){
                    for(std::string i: tokens){
                        data.push_back(i);
                    }
                    count++;
                }
            }
            dataArray.push_back(data);
        }
    }
    fin.seekg(0);
    std::cout << "reading data with key = " << key << " ended succesfuly." << " Number of elements is:" << count << std::endl;
    count = 0;
    return dataArray;
}


