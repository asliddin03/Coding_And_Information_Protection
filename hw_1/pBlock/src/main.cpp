#include "permutation.h"

#include <bitset>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {

// Выводим каждый байт в виде восьми двоичных цифр.
void printBits(const bits::Bytes& data) {
    for (auto byte : data) {
        std::cout << std::bitset<8>(byte) << ' ';
    }

    std::cout << '\n';
}

}

int main() {
    try {
        const bits::Bytes input{0xB2}; // 10110010

        const std::vector<std::size_t> p{
            1, 0, 2, 3, 4, 5, 6, 7
        };

        std::cout << "Исходные данные:  ";
        printBits(input);

        std::cout << "P-блок: [1, 0, 2, 3, 4, 5, 6, 7]\n";
        std::cout << "Начало отсчёта: 0\n\n";

        // По умолчанию нумерация идёт от старшего бита с нуля.
        std::cout << "От старшего бита: ";
        printBits(bits::permute(input, p));

        std::cout << "От младшего бита: ";
        printBits(bits::permute(
            input,
            p,
            bits::BitOrder::LeastSignificantFirst
        ));


        try {
            bits::permute(
                input,
                {0, 0, 2, 3, 4, 5, 6, 7}
            );
        } catch (const std::invalid_argument& error) {
            std::cout << "\nОшибка P-блока обработана: "
                      << error.what() << '\n';
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
        return 1;
    }
}