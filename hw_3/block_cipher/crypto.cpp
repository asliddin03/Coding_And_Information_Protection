#include "crypto.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {

int digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    throw runtime_error("Недопустимый символ в HEX-строке");
}

uint32_t readWord(const Bytes& data, size_t pos) {
    uint32_t value = 0;

    for (size_t i = 0; i < 4; ++i)
        value = (value << 8) | data[pos + i];

    return value;
}

void putWord(Bytes& data, size_t pos, uint32_t value) {
    for (int i = 3; i >= 0; --i) {
        data[pos + i] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

Bytes getBlock(const Bytes& data, size_t pos, size_t size) {
    return Bytes(data.begin() + pos, data.begin() + pos + size);
}

void putBlock(Bytes& data, size_t pos,
              const Bytes& block, size_t size) {
    if (block.size() != size)
        throw runtime_error("Алгоритм вернул неверный блок");

    copy(block.begin(), block.end(), data.begin() + pos);
}

size_t check(const Bytes& data, const Cipher& code,
             const Bytes& iv, int threads, bool needIv) {
    size_t size = code.size();

    if (threads < 1)
        throw runtime_error("Количество потоков должно быть положительным");

    if (size == 0 || data.size() % size != 0)
        throw runtime_error("Неверная длина данных для режима");

    if (needIv && iv.size() != size)
        throw runtime_error("Неверная длина IV");

    return size;
}

// Каждый поток обрабатывает свою последовательность блоков.
void parallel(size_t total, int threads,
              const function<void(size_t)>& work) {
    if (threads < 1)
        throw runtime_error("Количество потоков должно быть положительным");

    size_t count = min(total, static_cast<size_t>(threads));

    if (count <= 1) {
        for (size_t i = 0; i < total; ++i)
            work(i);

        return;
    }

    vector<future<void>> jobs;

    for (size_t t = 0; t < count; ++t) {
        jobs.push_back(async(launch::async, [&, t] {
            for (size_t i = t; i < total; i += count)
                work(i);
        }));
    }

    for (auto& job : jobs)
        job.get();
}

} // namespace

Bytes fromHex(const string& text) {
    if (text.size() % 2 != 0)
        throw runtime_error("HEX-строка должна содержать пары цифр");

    Bytes data(text.size() / 2);

    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(
            digit(text[2 * i]) * 16 + digit(text[2 * i + 1])
        );
    }

    return data;
}

Bytes readFile(const string& path) {
    ifstream file(path, ios::binary | ios::ate);

    if (!file)
        throw runtime_error("Не удалось открыть файл: " + path);

    auto length = file.tellg();

    if (length < 0)
        throw runtime_error("Не удалось определить размер: " + path);

    Bytes data(static_cast<size_t>(length));
    file.seekg(0);

    if (!file || (!data.empty() &&
        !file.read(reinterpret_cast<char*>(data.data()),
                   static_cast<streamsize>(data.size())))) {
        throw runtime_error("Не удалось прочитать файл: " + path);
    }

    return data;
}

void writeFile(const string& path, const Bytes& data) {
    ofstream file(path, ios::binary);

    if (!file)
        throw runtime_error("Не удалось создать файл: " + path);

    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<streamsize>(data.size()));
    }

    file.close();

    if (!file)
        throw runtime_error("Не удалось записать файл: " + path);
}

Xtea::Xtea(const Bytes& data) {
    if (data.size() != 16)
        throw runtime_error("Ключ XTEA должен содержать 16 байтов");

    for (size_t i = 0; i < 4; ++i)
        key[i] = readWord(data, i * 4);
}

Bytes Xtea::run(const Bytes& data, bool back) const {
    if (data.size() != 8)
        throw runtime_error("Блок XTEA должен содержать 8 байтов");

    uint32_t left = readWord(data, 0);
    uint32_t right = readWord(data, 4);

    const uint32_t step = 0x9E3779B9u;
    uint32_t sum = back ? step * 32u : 0u;

    for (int i = 0; i < 32; ++i) {
        if (back) {
            right -= (((left << 4) ^ (left >> 5)) + left)
                     ^ (sum + key[(sum >> 11) & 3]);

            sum -= step;

            left -= (((right << 4) ^ (right >> 5)) + right)
                    ^ (sum + key[sum & 3]);
        } else {
            left += (((right << 4) ^ (right >> 5)) + right)
                    ^ (sum + key[sum & 3]);

            sum += step;

            right += (((left << 4) ^ (left >> 5)) + left)
                     ^ (sum + key[(sum >> 11) & 3]);
        }
    }

    Bytes out(8);
    putWord(out, 0, left);
    putWord(out, 4, right);

    return out;
}

Bytes Ecb::run(const Bytes& data, const Cipher& code,
               const Bytes& iv, int threads, bool back) const {
    size_t size = check(data, code, iv, threads, false);
    Bytes out(data.size());

    parallel(data.size() / size, threads, [&](size_t i) {
        size_t pos = i * size;
        Bytes block = getBlock(data, pos, size);

        block = back ? code.decrypt(block) : code.encrypt(block);

        putBlock(out, pos, block, size);
    });

    return out;
}

Bytes Cbc::run(const Bytes& data, const Cipher& code,
               const Bytes& iv, int threads, bool back) const {
    size_t size = check(data, code, iv, threads, true);

    if (back) {
        Bytes out = Ecb().run(data, code, {}, threads, true);

        parallel(data.size() / size, threads, [&](size_t i) {
            size_t pos = i * size;

            for (size_t j = 0; j < size; ++j) {
                out[pos + j] ^=
                    (i == 0 ? iv[j] : data[pos - size + j]);
            }
        });

        return out;
    }

    Bytes out(data.size());
    Bytes prev = iv;

    for (size_t pos = 0; pos < data.size(); pos += size) {
        Bytes block = getBlock(data, pos, size);

        for (size_t j = 0; j < size; ++j)
            block[j] ^= prev[j];

        prev = code.encrypt(block);
        putBlock(out, pos, prev, size);
    }

    return out;
}

Bytes Pcbc::run(const Bytes& data, const Cipher& code,
                const Bytes& iv, int threads, bool back) const {
    size_t size = check(data, code, iv, threads, true);

    Bytes out(data.size());
    Bytes prev = iv;

    if (back)
        out = Ecb().run(data, code, {}, threads, true);

    for (size_t pos = 0; pos < data.size(); pos += size) {
        if (back) {
            for (size_t j = 0; j < size; ++j) {
                out[pos + j] ^= prev[j];
                prev[j] = out[pos + j] ^ data[pos + j];
            }
        } else {
            Bytes block = getBlock(data, pos, size);

            for (size_t j = 0; j < size; ++j)
                block[j] ^= prev[j];

            block = code.encrypt(block);
            putBlock(out, pos, block, size);

            for (size_t j = 0; j < size; ++j)
                prev[j] = data[pos + j] ^ block[j];
        }
    }

    return out;
}

void Zeros::add(Bytes& data, size_t size) const {
    if (size == 0)
        throw runtime_error("Размер блока должен быть положительным");

    data.resize(data.size() + (size - data.size() % size) % size, 0);
}

void Zeros::remove(Bytes& data, size_t size, size_t old) const {
    if (size == 0 || data.size() % size != 0 ||
        old > data.size() || data.size() - old >= size) {
        throw runtime_error("Неверная длина набивки Zeros");
    }

    for (size_t i = old; i < data.size(); ++i) {
        if (data[i] != 0)
            throw runtime_error("Неверная набивка Zeros");
    }

    data.resize(old);
}

void Ansi::add(Bytes& data, size_t size) const {
    if (size == 0 || size > 255)
        throw runtime_error("Размер блока ANSI должен быть от 1 до 255");

    size_t count = size - data.size() % size;

    data.resize(data.size() + count, 0);
    data.back() = static_cast<uint8_t>(count);
}

void Ansi::remove(Bytes& data, size_t size, size_t old) const {
    if (size == 0 || size > 255 ||
        data.empty() || data.size() % size != 0) {
        throw runtime_error("Неверная длина набивки ANSI");
    }

    size_t count = data.back();

    if (count == 0 || count > size ||
        count > data.size() || data.size() - count != old) {
        throw runtime_error("Неверная длина набивки ANSI");
    }

    for (size_t i = old; i + 1 < data.size(); ++i) {
        if (data[i] != 0)
            throw runtime_error("Неверная набивка ANSI");
    }

    data.resize(old);
}

Context::Context(shared_ptr<const Cipher> code,
                 ModeType type, PadType padding,
                 Bytes iv, vector<Bytes> extra)
    : code(move(code)), iv(move(iv)) {

    if (!this->code || this->code->size() == 0)
        throw runtime_error("Не задан корректный алгоритм");

    if (!extra.empty())
        throw runtime_error("Для этих режимов дополнительные параметры не нужны");

    switch (type) {
        case ModeType::ECB:
            mode = make_shared<Ecb>();
            break;

        case ModeType::CBC:
            mode = make_shared<Cbc>();
            break;

        case ModeType::PCBC:
            mode = make_shared<Pcbc>();
            break;

        default:
            throw runtime_error("Неизвестный режим шифрования");
    }

    switch (padding) {
        case PadType::Zeros:
            pad = make_shared<Zeros>();
            break;

        case PadType::ANSIX923:
            pad = make_shared<Ansi>();
            break;

        default:
            throw runtime_error("Неизвестный режим набивки");
    }

    if (type != ModeType::ECB &&
        this->iv.size() != this->code->size()) {
        throw runtime_error("Длина IV должна совпадать с размером блока");
    }
}

void Context::encrypt(const Bytes& data,
                      Bytes& out, int threads) const {
    if (threads < 1)
        throw runtime_error("Количество потоков должно быть положительным");

    Bytes work = data;

    pad->add(work, code->size());
    work = mode->run(work, *code, iv, threads, false);

    // Первые 8 байтов содержат исходную длину данных.
    Bytes result(8);
    uint64_t old = data.size();

    for (int i = 7; i >= 0; --i) {
        result[i] = static_cast<uint8_t>(old);
        old >>= 8;
    }

    result.insert(result.end(), work.begin(), work.end());
    out = move(result);
}

void Context::decrypt(const Bytes& data,
                      Bytes& out, int threads) const {
    if (threads < 1)
        throw runtime_error("Количество потоков должно быть положительным");

    if (data.size() < 8)
        throw runtime_error("Отсутствует заголовок с длиной данных");

    uint64_t old = 0;

    for (size_t i = 0; i < 8; ++i)
        old = (old << 8) | data[i];

    if (old > data.size() - 8)
        throw runtime_error("Неверная исходная длина");

    Bytes work(data.begin() + 8, data.end());

    work = mode->run(work, *code, iv, threads, true);
    pad->remove(work, code->size(), static_cast<size_t>(old));

    out = move(work);
}

future<void> Context::encrypt(const string& from,
                              const string& to,
                              int threads) const {
    return async(launch::async, [self = *this, from, to, threads] {
        if (from == to)
            throw runtime_error("Входной и выходной файлы должны отличаться");

        Bytes data = readFile(from);
        Bytes out;

        self.encrypt(data, out, threads);
        writeFile(to, out);
    });
}

future<void> Context::decrypt(const string& from,
                              const string& to,
                              int threads) const {
    return async(launch::async, [self = *this, from, to, threads] {
        if (from == to)
            throw runtime_error("Входной и выходной файлы должны отличаться");

        Bytes data = readFile(from);
        Bytes out;

        self.decrypt(data, out, threads);
        writeFile(to, out);
    });
}