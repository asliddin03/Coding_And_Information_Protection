#include "bit_operations.h"

#include <bitset>
#include <exception>
#include <iostream>

void print(const char* label, Value value) {
    std::cout << label << std::bitset<8>(value) << '\n';
}

int main() {
    try {
        const Value value = 0b10110010;
        const unsigned n = 8;

        print("Исходное значение:    ", value);
        print("Сдвиг влево на 2:     ", rotateLeft(value, n, 2));
        print("Сдвиг вправо на 2:    ", rotateRight(value, n, 2));
        print("Маска 00001111:       ", applyMask(value, n, 0b1111, 4));
        print("Биты с 2 по 5:        ", extractBits(value, n, 2, 5));
        print("Обмен битов 0 и 1:    ", swapBits(value, n, 0, 1));
        print("Установить бит 0 в 1: ", setBit(value, n, 0, true));
        print("Установить бит 1 в 0: ", setBit(value, n, 1, false));

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
        return 1;
    }
}