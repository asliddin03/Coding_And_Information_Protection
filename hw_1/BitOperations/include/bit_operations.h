#pragma once

#include <cstdint>

using Value = std::uint64_t;

// Циклические сдвиги в пределах n битов.
Value rotateLeft(Value value, unsigned n, unsigned k);
Value rotateRight(Value value, unsigned n, unsigned k);

// Применение k-битовой маски к младшим разрядам.
Value applyMask(Value value, unsigned n, Value mask, unsigned k);

// Извлечение битов от i до j включительно.
Value extractBits(Value value, unsigned n, unsigned i, unsigned j);

// Обмен двух битов.
Value swapBits(Value value, unsigned n, unsigned i, unsigned j);

// Установка бита в 0 или 1.
Value setBit(Value value, unsigned n, unsigned i, bool bit);
