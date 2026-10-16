#include "crypto.h"

#include <chrono>
#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace std;

void check(bool ok, const string& text) {
    if (!ok)
        throw runtime_error("Не пройден тест: " + text);
}

void mustFail(const function<void()>& work) {
    bool failed = false;

    try {
        work();
    }
    catch (const exception&) {
        failed = true;
    }

    check(failed, "ожидалась ошибка");
}

// Тестовый алгоритм для ручной проверки формул режимов.
class TestCipher : public Cipher {
public:
    size_t size() const override {
        return 4;
    }

    Bytes encrypt(const Bytes& data) const override {
        Bytes out = data;

        for (auto& value : out)
            ++value;

        return out;
    }

    Bytes decrypt(const Bytes& data) const override {
        Bytes out = data;

        for (auto& value : out)
            --value;

        return out;
    }
};

void testCipher() {
    check(fromHex("01aBFF") == Bytes({1, 171, 255}), "HEX");
    check(fromHex("").empty(), "пустая HEX-строка");

    mustFail([] { fromHex("123"); });
    mustFail([] { fromHex("GG"); });
    mustFail([] { Xtea code(Bytes(15)); });

    Xtea code(Bytes(16, 0));
    check(code.size() == 8, "размер блока");

    Bytes plain = fromHex("4142434445464748");
    Bytes expected = fromHex("A0390589F8B8EFA5");

    check(code.encrypt(plain) == expected, "контрольный пример XTEA");
    check(code.decrypt(expected) == plain, "обратный пример XTEA");

    Xtea other(fromHex("000102030405060708090A0B0C0D0E0F"));
    expected = fromHex("497DF3D072612CB5");

    check(other.encrypt(plain) == expected, "XTEA с ненулевым ключом");
    check(other.decrypt(expected) == plain, "обратный XTEA с ненулевым ключом");

    mustFail([&] { code.encrypt(Bytes(7)); });
    mustFail([&] { code.decrypt(Bytes(9)); });
}

void testPadding() {
    Zeros zeros;
    Ansi ansi;

    Bytes plain{1, 2, 0};
    Bytes data = plain;

    zeros.add(data, 8);
    check(data == Bytes({1, 2, 0, 0, 0, 0, 0, 0}), "байты Zeros");

    zeros.remove(data, 8, 3);
    check(data == plain, "исходный нулевой байт сохранён");

    ansi.add(data, 8);
    check(data == Bytes({1, 2, 0, 0, 0, 0, 0, 5}), "байты ANSI");

    ansi.remove(data, 8, 3);
    check(data == plain, "удаление ANSI");

    data = Bytes(8, 1);

    zeros.add(data, 8);
    check(data.size() == 8, "Zeros для полного блока");

    ansi.add(data, 8);
    check(data.size() == 16 && data.back() == 8, "ANSI для полного блока");

    ansi.remove(data, 8, 8);
    check(data == Bytes(8, 1), "удаление полного блока ANSI");

    mustFail([&] { zeros.add(data, 0); });
    mustFail([&] { ansi.add(data, 0); });
    mustFail([&] { ansi.add(data, 256); });
    mustFail([&] { zeros.remove(data, 8, 7); });
    mustFail([&] { zeros.remove(data, 8, 9); });

    data.clear();
    mustFail([&] { ansi.remove(data, 8, 0); });

    data = Bytes(8, 0);
    mustFail([&] { ansi.remove(data, 8, 0); });

    data.back() = 9;
    mustFail([&] { ansi.remove(data, 8, 0); });

    data = {1, 2, 3, 4, 0, 0, 9, 4};
    mustFail([&] { ansi.remove(data, 8, 4); });
}

void testModes() {
    TestCipher code;

    Bytes plain{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0};
    Bytes iv{10, 20, 30, 40};

    vector<shared_ptr<Mode>> modes{
        make_shared<Ecb>(),
        make_shared<Cbc>(),
        make_shared<Pcbc>()
    };

    vector<Bytes> expected{
        {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 1, 1},
        {12, 23, 30, 45, 10, 18, 26, 38, 4, 25, 27, 39},
        {12, 23, 30, 45, 9, 20, 27, 34, 6, 25, 29, 43}
    };

    for (size_t i = 0; i < modes.size(); ++i) {
        for (int threads : {1, 4}) {
            check(
                modes[i]->run(plain, code, iv, threads, false) == expected[i],
                "формула режима"
            );

            check(
                modes[i]->run(expected[i], code, iv, threads, true) == plain,
                "обратная формула"
            );
        }

        mustFail([&] {
            modes[i]->run(Bytes(1), code, iv, 1, false);
        });

        mustFail([&] {
            modes[i]->run(plain, code, iv, 0, true);
        });

        if (i != 0) {
            mustFail([&] {
                modes[i]->run(plain, code, {}, 1, false);
            });
        }
    }
}

void testContext() {
    auto code = make_shared<Xtea>(Bytes(16, 0));
    Bytes iv(8, 1), out;

    mustFail([&] {
        Context box(nullptr, ModeType::ECB, PadType::Zeros);
    });

    mustFail([&] {
        Context box(code, ModeType::CBC, PadType::Zeros);
    });

    mustFail([&] {
        Context box(code, static_cast<ModeType>(99), PadType::Zeros);
    });

    mustFail([&] {
        Context box(code, ModeType::ECB, static_cast<PadType>(99));
    });

    mustFail([&] {
        Context box(code, ModeType::ECB, PadType::Zeros, {}, {{1}});
    });

    for (auto mode : {ModeType::ECB, ModeType::CBC, ModeType::PCBC}) {
        for (auto pad : {PadType::Zeros, PadType::ANSIX923}) {
            Context box(code, mode, pad, iv);

            for (size_t size : {0, 1, 7, 8, 9, 16, 17, 31, 64, 65}) {
                Bytes plain(size), one, many, back;

                for (size_t i = 0; i < size; ++i)
                    plain[i] = static_cast<uint8_t>(i * 19);

                if (size != 0)
                    plain.back() = 0;

                box.encrypt(plain, one, 1);
                box.encrypt(plain, many, 4);

                check(one == many, "результат не зависит от числа потоков");

                box.decrypt(many, back, 4);
                check(back == plain, "восстановление данных");
            }

            Bytes data{1, 2, 0, 0};

            box.encrypt(data, data, 2);
            box.decrypt(data, data, 2);

            check(
                data == Bytes({1, 2, 0, 0}),
                "один массив для входа и выхода"
            );

            mustFail([&] { box.encrypt(data, out, 0); });
            mustFail([&] { box.decrypt(data, out, -1); });
            mustFail([&] { box.decrypt(Bytes(7), out, 1); });
            mustFail([&] { box.decrypt(Bytes(9), out, 1); });

            box.encrypt(data, out, 1);
            out[0] = 255;

            mustFail([&] { box.decrypt(out, out, 1); });
        }
    }

    class BadCipher : public TestCipher {
        Bytes encrypt(const Bytes&) const override {
            throw runtime_error("Ошибка алгоритма");
        }
    };

    Context bad(make_shared<BadCipher>(), ModeType::ECB, PadType::Zeros);

    mustFail([&] {
        bad.encrypt(Bytes(32), out, 4);
    });
}

void testFiles() {
    string from = "test_" + to_string(
        chrono::steady_clock::now().time_since_epoch().count()
    );

    string to = from + ".enc";
    string back = from + ".out";

    Bytes plain{0, 255, 10, 13, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0};
    auto code = make_shared<Xtea>(Bytes(16, 0));

    writeFile(from, {});
    check(readFile(from).empty(), "пустой файл");

    writeFile(from, plain);

    for (auto mode : {ModeType::ECB, ModeType::CBC, ModeType::PCBC}) {
        for (auto pad : {PadType::Zeros, PadType::ANSIX923}) {
            Context box(code, mode, pad, Bytes(8, 0));

            auto job = box.encrypt(from, to, 4);

            check(
                job.wait_for(chrono::seconds(0)) != future_status::deferred,
                "асинхронный запуск"
            );

            job.get();
            box.decrypt(to, back, 4).get();

            check(readFile(back) == plain, "восстановление файла");
        }
    }

    Context box(code, ModeType::ECB, PadType::Zeros);

    auto job = Context(code, ModeType::ECB, PadType::Zeros)
                   .encrypt(from, to, 2);

    job.get();
    box.decrypt(to, back, 2).get();

    check(
        readFile(back) == plain,
        "задача переживает временный контекст"
    );

    mustFail([&] { readFile(from + ".missing"); });
    mustFail([&] { writeFile(from + ".dir/out", {}); });

    mustFail([&] {
        box.encrypt(from + ".missing", to, 1).get();
    });

    mustFail([&] {
        box.decrypt(from + ".missing", back, 1).get();
    });

    mustFail([&] { box.encrypt(from, from, 1).get(); });
    mustFail([&] { box.decrypt(to, to, 1).get(); });
    mustFail([&] { box.encrypt(from, to, 0).get(); });
    mustFail([&] { box.decrypt(to, back, 0).get(); });

    writeFile(to, Bytes(7));

    mustFail([&] {
        box.decrypt(to, back, 1).get();
    });

    std::remove(from.c_str());
    std::remove(to.c_str());
    std::remove(back.c_str());
}

int main() {
    try {
        testCipher();
        testPadding();
        testModes();
        testContext();
        testFiles();

        cout << "Все тесты пройдены.\n";
    }
    catch (const exception& error) {
        cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}