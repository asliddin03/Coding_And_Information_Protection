#include "bit_operations.h"

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
        const Value value = 0b10110010;

        check(
            rotateLeft(value, 8, 2) == 0b11001010,
            "Циклический сдвиг влево"
        );

        check(
            rotateRight(value, 8, 2) == 0b10101100,
            "Циклический сдвиг вправо"
        );

        check(
            rotateLeft(value, 8, 0) == value &&
            rotateRight(value, 8, 8) == value,
            "Нулевой и полный сдвиг"
        );

        check(
            rotateLeft(value, 8, 10) == rotateLeft(value, 8, 2),
            "Сдвиг больше размера значения"
        );

        check(
            rotateRight(rotateLeft(value, 8, 3), 8, 3) == value,
            "Обратимость циклического сдвига"
        );

        check(
            applyMask(value, 8, 0b1111, 4) == 0b0010,
            "Применение маски"
        );

        check(
            applyMask(value, 8, 0, 4) == 0,
            "Нулевая маска"
        );

        check(
            extractBits(value, 8, 2, 5) == 0b1100,
            "Извлечение диапазона"
        );

        check(
            extractBits(value, 8, 7, 7) == 1,
            "Извлечение одного бита"
        );

        check(
            swapBits(value, 8, 0, 1) == 0b10110001,
            "Обмен разных битов"
        );

        check(
            swapBits(value, 8, 0, 2) == value &&
            swapBits(value, 8, 1, 1) == value,
            "Обмен одинаковых битов и одной позиции"
        );

        check(
            setBit(value, 8, 0, true) == 0b10110011,
            "Установка единицы"
        );

        check(
            setBit(value, 8, 1, false) == 0b10110000,
            "Установка нуля"
        );

        check(
            rotateLeft(0b10011, 5, 1) == 0b00111,
            "Пятибитовое значение"
        );

        check(
            rotateLeft(1, 1, 5) == 1 &&
            setBit(1, 1, 0, false) == 0,
            "Однобитовое значение"
        );

        const Value high = Value{1} << 63;
        const Value all = ~Value{0};

        check(
            rotateLeft(high, 64, 1) == 1 &&
            rotateRight(1, 64, 1) == high &&
            rotateLeft(high, 64, 64) == high,
            "Сдвиги 64-битового значения"
        );

        check(
            extractBits(all, 64, 0, 63) == all &&
            applyMask(all, 64, all, 64) == all,
            "Полный 64-битовый диапазон и маска"
        );

        check(
            swapBits(high, 64, 0, 63) == 1 &&
            setBit(0, 64, 63, true) == high,
            "Старший бит 64-битового значения"
        );

        std::cout << "Все тесты успешно пройдены\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Тест не пройден: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
        return 1;
    }
}