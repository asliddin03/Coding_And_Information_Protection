#include "bit_operations.h"

#include <stdexcept>

namespace {

// Получаем число, у которого младшие n битов равны единице
Value lowMask(unsigned n) {
    if (n == 64) {
        return ~Value{0};
    }

    return (Value{1} << n) - 1;
}

void checkWidth(unsigned n) {
    if (n < 1 || n > 64) {
        throw std::invalid_argument("Размер должен быть от 1 до 64 бит");
    }
}

void checkIndex(unsigned index, unsigned n) {
    if (index >= n) {
        throw std::out_of_range("Индекс бита выходит за границы");
    }
}

}

Value rotateLeft(Value value, unsigned n, unsigned k) {
    checkWidth(n);

    value &= lowMask(n);
    k %= n;

    if (k == 0) {
        return value;
    }

    return ((value << k) | (value >> (n - k))) & lowMask(n);
}

Value rotateRight(Value value, unsigned n, unsigned k) {
    checkWidth(n);

    value &= lowMask(n);
    k %= n;

    if (k == 0) {
        return value;
    }

    return ((value >> k) | (value << (n - k))) & lowMask(n);
}

Value applyMask(Value value, unsigned n, Value mask, unsigned k) {
    checkWidth(n);

    if (k < 1 || k > n) {
        throw std::invalid_argument("Размер маски должен быть от 1 до n");
    }

    if ((mask & ~lowMask(k)) != 0) {
        throw std::invalid_argument("Маска не помещается в k битов");
    }

    return value & mask;
}

Value extractBits(Value value, unsigned n, unsigned i, unsigned j) {
    checkWidth(n);
    checkIndex(i, n);
    checkIndex(j, n);

    if (i > j) {
        throw std::invalid_argument("Начало диапазона больше конца");
    }

    return (value >> i) & lowMask(j - i + 1);
}

Value swapBits(Value value, unsigned n, unsigned i, unsigned j) {
    checkWidth(n);
    checkIndex(i, n);
    checkIndex(j, n);

    value &= lowMask(n);

    const Value first = (value >> i) & 1;
    const Value second = (value >> j) & 1;

    if (first != second) {
        value ^= (Value{1} << i) | (Value{1} << j);
    }

    return value;
}

Value setBit(Value value, unsigned n, unsigned i, bool bit) {
    checkWidth(n);
    checkIndex(i, n);

    value &= lowMask(n);
    const Value mask = Value{1} << i;

    if (bit) {
        return value | mask;
    }

    return value & ~mask;
}