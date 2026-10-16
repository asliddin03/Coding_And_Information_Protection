#include "rc4.h"
#include "ipc_manager.h"
#include "files.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

void check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

void writeFile(const std::string& name,
               const std::string& text) {
    std::ofstream file(name, std::ios::binary);

    file.write(text.data(), text.size());

    check(bool(file), "Не удалось создать тестовый файл");
}

}

int main() {
    const std::string name =
        "rc4_test_" + std::to_string(getpid());

    const std::string first = name + "_1";
    const std::string second = name + "_2";

    int result = 1;

    try {
        // RC4: известный результат и обработка по частям.
        unsigned char data[] = {
            'P', 'l', 'a', 'i', 'n', 't', 'e', 'x', 't'
        };

        const unsigned char expected[] = {
            0xbb, 0xf3, 0x16, 0xe8, 0xd9,
            0x40, 0xaf, 0x0a, 0xd3
        };

        RC4 cipher;
        cipher.setKey("Key");

        cipher.process(data, 3);
        cipher.process(data + 3, 6);

        check(
            std::equal(data, data + 9, expected),
            "Ошибка RC4"
        );

        cipher.setKey("Key");
        cipher.process(data, 9);

        check(
            std::memcmp(data, "Plaintext", 9) == 0,
            "Ошибка расшифрования"
        );

        // Сравнение файлов.
        writeFile(first, "");
        writeFile(second, "");

        check(
            compareFiles(first, second),
            "Сравнение пустых файлов"
        );

        writeFile(first, std::string("a\0b", 3));
        writeFile(second, std::string("a\0b", 3));

        check(
            compareFiles(first, second),
            "Сравнение двоичных файлов"
        );

        writeFile(second, std::string("a\0c", 3));

        check(
            !compareFiles(first, second),
            "Файлы с разным содержимым"
        );

        writeFile(second, std::string("a\0bc", 4));

        check(
            !compareFiles(first, second),
            "Файлы разной длины"
        );

        writeFile(first, std::string(10000, 'x'));
        writeFile(second, std::string(10000, 'x'));

        check(
            compareFiles(first, second),
            "Сравнение больших файлов"
        );

        // Общая память, семафоры и 100 слотов.
        {
            IPCManager server(true);
            IPCManager client(false);

            std::vector<unsigned> slots;

            for (unsigned n = 0; n < MAX_SESSIONS; ++n) {
                int index = client.findFreeSlot();

                check(index >= 0, "Не удалось получить слот");
                slots.push_back(index);

                Session& sent = client.session(index);
                Session& received = server.session(index);

                sent.dataSize = n;

                IPCManager::signal(client.request(index));
                IPCManager::wait(server.request(index));

                check(
                    received.dataSize == n,
                    "Ошибка передачи через общую память"
                );

                IPCManager::signal(server.response(index));
                IPCManager::wait(client.response(index));
            }

            check(
                client.findFreeSlot() == -1,
                "Должны быть заняты все 100 слотов"
            );

            for (unsigned index : slots)
                client.freeSlot(index);

            int index = client.findFreeSlot();

            check(index >= 0, "Слот не освободился");
            client.freeSlot(index);
        }

        std::cout << "Все тесты пройдены\n";
        result = 0;

    } catch (const std::exception& error) {
        std::cerr << "Ошибка теста: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
    }

    std::remove(first.c_str());
    std::remove(second.c_str());

    return result;
}

// python3 <<'PY'
// import ctypes
// import errno
// import os
//
// libc = ctypes.CDLL(None, use_errno=True)
// name = f"/rc4_{os.getuid()}"
//
// def remove(function, resource):
//     function.argtypes = [ctypes.c_char_p]
//     function.restype = ctypes.c_int
//
//     if function(resource.encode()) != 0:
//         code = ctypes.get_errno()
//         if code != errno.ENOENT:
//             raise OSError(code, os.strerror(code), resource)
//
// for session in range(100):
//     for kind in range(3):
//         remove(libc.sem_unlink, f"{name}_{session}_{kind}")
//
// remove(libc.shm_unlink, name)
// print("Общая память и семафоры RC4 очищены")
// PY