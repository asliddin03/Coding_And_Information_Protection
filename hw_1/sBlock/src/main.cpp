#include "substitution.h"

#include <bitset>
#include <exception>
#include <iostream>
#include <stdexcept>

void printBits(const Bytes& data) {
    for (auto byte : data) {
        std::cout << std::bitset<8>(byte) << ' ';
    }
    std::cout << '\n';
}

int main() {
    try {
        const Bytes input{0b10110010};

        const Table table{
                {0b1011, 0b0101},
                {0b0010, 0b1110}
        };

        const Rule rule = [](std::uint8_t value) -> std::uint8_t {
            switch (value) {
            case 0b1011: return 0b0101;
            case 0b0010: return 0b1110;
            default:
                throw std::out_of_range("Для группы не задана замена");
            }
        };

        std::cout << "Исходные данные: ";
        printBits(input);

        std::cout << "По таблице:      ";
        printBits(substitute(input, table, 4));

        std::cout << "По функции:      ";
        printBits(substitute(input, rule, 4));

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
        return 1;
    }
}