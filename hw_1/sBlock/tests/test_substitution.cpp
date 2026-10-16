#include "substitution.h"

#include <exception>
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    try {
        const Table table{
            {0b1011, 0b0101},
            {0b0010, 0b1110}
        };

        const Rule rule = [](std::uint8_t value) -> std::uint8_t {
            switch (value) {
                case 0b1011: return 0b0101;
                case 0b0010: return 0b1110;
                default:
                    throw std::out_of_range(
                        "Для группы не задана замена"
                    );
            }
        };

        const Bytes input{0b10110010};
        const Bytes expected{0b01011110};

        check(
            substitute(input, table, 4) == expected,
            "Замена по таблице"
        );

        check(
            substitute(input, rule, 4) == expected,
            "Замена по функции"
        );

        const Bytes decimalInput{178};
        const Table decimalTable{{11, 5}, {2, 14}};

        check(
            substitute(decimalInput, decimalTable, 4) == Bytes{94},
            "Замена по таблице с десятичными числами"
        );

        check(
            substitute(decimalInput, rule, 4) == Bytes{94},
            "Замена по функции с десятичными числами"
        );

        check(
            substitute(Bytes{0b10111011}, table, 4)
                == Bytes{0b01010101},
            "Повторяющиеся группы"
        );

        check(
            substitute(Bytes{}, table, 4).empty() &&
            substitute(Bytes{}, rule, 4).empty(),
            "Пустой массив"
        );

        const Rule identity = [](std::uint8_t value) {
            return value;
        };

        const Bytes data{
            0b10110010, 0b01101001, 0b11100010
        };

        check(
            substitute(data, identity, 3) == data,
            "Группы на границе байтов"
        );

        check(
            substitute(Bytes{0, 255}, identity, 8)
                == Bytes({0, 255}),
            "Восьмибитовые группы"
        );

        check(
            input == Bytes{0b10110010},
            "Исходные данные изменились"
        );

        std::cout << "Все тесты успешно пройдены\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Тест не пройден: "
                  << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
        return 1;
    }
}