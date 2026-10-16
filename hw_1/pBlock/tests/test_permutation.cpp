#include "../src/permutation.h"

#include <exception>
#include <iostream>
#include <stdexcept>

namespace {

void check(bool condition, const char* name) {
    if (!condition) {
        throw std::runtime_error(name);
    }
}

template<class Exception, class Function>
void expectException(Function function, const char* name) {
    bool caught = false;

    try {
        function();
    } catch (const Exception&) {
        caught = true;
    }

    check(caught, name);
}

}

int main() {
    try {
        using namespace bits;

        const std::vector<std::size_t> identity{
            0, 1, 2, 3, 4, 5, 6, 7
        };

        const std::vector<std::size_t> swap{
            1, 0, 2, 3, 4, 5, 6, 7
        };

        const std::vector<std::size_t> reverse{
            7, 6, 5, 4, 3, 2, 1, 0
        };

        check(
            permute({0xB2}, identity) == Bytes{0xB2},
            "Тождественная перестановка"
        );

        check(
            permute({0xB2}, swap) == Bytes{0x72},
            "Перестановка от старшего бита"
        );

        check(
            permute(
                {0xB2},
                swap,
                BitOrder::LeastSignificantFirst
            ) == Bytes{0xB1},
            "Перестановка от младшего бита"
        );

        check(
            permute({0xB2}, reverse) == Bytes{0x4D},
            "Обратный порядок битов"
        );

        check(
            permute(
                {0xB2},
                {2, 1, 3, 4, 5, 6, 7, 8},
                BitOrder::MostSignificantFirst,
                IndexBase::One
            ) == Bytes{0x72},
            "Отсчёт с единицы от старшего бита"
        );

        check(
            permute(
                {0xB2},
                {2, 1, 3, 4, 5, 6, 7, 8},
                BitOrder::LeastSignificantFirst,
                IndexBase::One
            ) == Bytes{0xB1},
            "Отсчёт с единицы от младшего бита"
        );

        check(
            permute({}, {}).empty(),
            "Обработка пустого массива"
        );

        const Bytes input{0xB2};
        const Bytes original = input;

        permute(input, swap);

        check(
            input == original,
            "Сохранение исходных данных"
        );

        expectException<std::invalid_argument>(
            [] {
                permute({0xB2}, {0, 1});
            },
            "Обработка неверной длины P-блока"
        );

        expectException<std::invalid_argument>(
            [] {
                permute(
                    {0xB2},
                    {0, 0, 2, 3, 4, 5, 6, 7}
                );
            },
            "Обработка повторяющегося индекса"
        );

        expectException<std::out_of_range>(
            [] {
                permute(
                    {0xB2},
                    {0, 1, 2, 3, 4, 5, 6, 8}
                );
            },
            "Обработка выхода индекса за границы"
        );

        expectException<std::out_of_range>(
            [] {
                permute(
                    {0xB2},
                    {0, 2, 3, 4, 5, 6, 7, 8},
                    BitOrder::MostSignificantFirst,
                    IndexBase::One
                );
            },
            "Обработка нулевого индекса при отсчёте с единицы"
        );

        expectException<std::invalid_argument>(
            [] {
                permute(
                    {},
                    {},
                    static_cast<BitOrder>(99)
                );
            },
            "Обработка неверного направления нумерации"
        );

        expectException<std::invalid_argument>(
            [] {
                permute(
                    {},
                    {},
                    BitOrder::MostSignificantFirst,
                    static_cast<IndexBase>(99)
                );
            },
            "Обработка неверного начала отсчёта"
        );

        std::cout << "Все тесты успешно пройдены\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Тест не пройден: "
                  << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Непредвиденная ошибка при выполнении тестов\n";
        return 1;
    }
}