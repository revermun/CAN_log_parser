#include <string>
#include <iostream>
#include <cmath>
#include <cstdint>

//Функция, преобразующая строку с 16-ным числом в строку с 2-ным числом
std::string hexToBin(const std::string& hexString) {
  std::string binaryString = "";

  for (char c : hexString) {
    unsigned char hexValue;
    if (isdigit(c)) {
      hexValue = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      hexValue = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      hexValue = c - 'A' + 10;
    } else {
      // Handle invalid characters (optional: throw an exception, return an error message, etc.)
      std::cerr << "Invalid hex character: " << c << std::endl;
      return ""; // Or throw an exception
    }

    // Convert hex value to 4-bit binary
    for (int i = 3; i >= 0; --i) {
      binaryString += ((hexValue >> i) & 1) ? '1' : '0';
    }
  }

  return binaryString;
}
// Функция, преобразующая строку с двоичным числом в соответствующий ему float
float binToFloat(const std::string& binaryString) {
    if (binaryString.length() != 32) {
        std::cerr << "Error: Binary string must be 32 bits long." << std::endl;
        return std::numeric_limits<float>::quiet_NaN(); // Return NaN on error
    }

    // Check if the string contains only '0' and '1'
    if (binaryString.find_first_not_of("01") != std::string::npos) {
        std::cerr << "Error: Binary string contains invalid characters." << std::endl;
        return std::numeric_limits<float>::quiet_NaN(); // Return NaN on error
    }

    // Convert binary string to unsigned integer
    unsigned int intValue = 0;
    for (int i = 0; i < 32; ++i) {
        if (binaryString[i] == '1') {
            intValue |= (1UL << (31 - i)); // Use 1UL to ensure unsigned long
        }
    }

    // Interpret the integer as a float using reinterpret_cast
    return *reinterpret_cast<float*>(&intValue);
}
// Функция, преобразующая строку с двоичным числом в соответствующий ему uint32_t
uint32_t binToUint32(const std::string& binaryString) {
    if (binaryString.length() != 32) {
        std::cerr << "Error: Binary string must be 32 bits long." << std::endl;
        return 0; // Or throw an exception
    }

    // Check if the string contains only '0' and '1'
    if (binaryString.find_first_not_of("01") != std::string::npos) {
        std::cerr << "Error: Binary string contains invalid characters." << std::endl;
        return 0; // Or throw an exception
    }

    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        if (binaryString[i] == '1') {
            result |= (1UL << (31 - i)); // Use 1UL to ensure unsigned long literal
        }
    }

    return result;
}
// Функция, преобразующая строку с двоичным числом в соответствующий ему int16_t
int16_t binToInt16(const std::string& binaryString) {
    if (binaryString.length() != 16) {
        std::cerr << "Error: Binary string must be 16 bits long." << std::endl;
        return 0; // Or throw an exception
    }

    // Check if the string contains only '0' and '1'
    if (binaryString.find_first_not_of("01") != std::string::npos) {
        std::cerr << "Error: Binary string contains invalid characters." << std::endl;
        return 0; // Or throw an exception
    }

    int16_t result = 0;
    for (int i = 0; i < 16; ++i) {
        if (binaryString[i] == '1') {
            //  Use 1 << (15 - i) to set the bit
            result |= (1 << (15 - i));
        }
    }

    // Handle the sign bit (most significant bit)
    if ((result & 0x8000) != 0) {  // Check if the MSB is set (0x8000 is 1000 0000 0000 0000 in hex)
        result |= 0xFFFF0000; // Extend the sign bit to fill the higher bytes for correct negative representation.
    }

    return result;
}
