#include "ipc_manager.h"
#include "files.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>

namespace {

void transformFile(const char* inputPath,
                   const char* outputPath,
                   const std::string& key) {
    if (key.empty() || key.size() > 256)
        throw std::runtime_error(
            "Ключ должен содержать 1–256 байтов"
        );

    std::ifstream input(inputPath, std::ios::binary);

    if (!input)
        throw std::runtime_error(
            "Не удалось открыть исходный файл"
        );

    IPCManager ipc(false);
    int index = ipc.findFreeSlot();

    if (index == -1)
        throw std::runtime_error("Все сессии заняты");

    Session& session = ipc.session(index);

    FILE* output = nullptr;
    bool created = false;
    bool pending = false;

    auto exchange = [&](Command command) {
        session.command = command;

        IPCManager::signal(ipc.request(index));
        pending = true;

        IPCManager::wait(ipc.response(index));
        pending = false;

        if (session.error)
            throw std::runtime_error(
                "Сервер отклонил запрос"
            );
    };

    try {
        // Не перезаписываем существующий файл.
        output = std::fopen(outputPath, "wbx");

        if (!output)
            throw std::runtime_error(
                "Не удалось создать результат. "
                "Возможно, такой файл уже существует"
            );

        created = true;

        session.keySize = key.size();
        std::copy(key.begin(), key.end(), session.key);

        exchange(Command::Begin);

        for (;;) {
            input.read(
                reinterpret_cast<char*>(session.buffer),
                BUFFER_SIZE
            );

            if (input.bad() || (input.fail() && !input.eof()))
                throw std::runtime_error(
                    "Ошибка чтения файла"
                );

            session.dataSize =
                static_cast<unsigned>(input.gcount());

            if (session.dataSize == 0) break;

            exchange(Command::Data);

            if (std::fwrite(
                    session.buffer,
                    1,
                    session.dataSize,
                    output
                ) != session.dataSize) {
                throw std::runtime_error(
                    "Ошибка записи файла"
                );
            }
        }

        exchange(Command::End);

        int result = std::fclose(output);
        output = nullptr;

        if (result != 0)
            throw std::runtime_error(
                "Ошибка сохранения файла"
            );

    } catch (...) {
        if (output)
            std::fclose(output);

        if (created)
            std::remove(outputPath);

        if (!pending)
            sem_post(&ipc.available(index));

        throw;
    }

    ipc.freeSlot(index);
}

}

int main(int argc, char* argv[]) {
    try {
        if (argc == 4 && std::string(argv[1]) == "compare") {
            bool equal = compareFiles(argv[2], argv[3]);

            std::cout << (
                equal
                    ? "Файлы совпадают\n"
                    : "Файлы различаются\n"
            );

            return equal ? 0 : 1;
        }

        if (argc != 5 || std::string(argv[1]) != "crypt") {
            std::cerr
                << "Использование:\n"
                << "client crypt исходный результат ключ\n"
                << "client compare первый второй\n";

            return 1;
        }

        transformFile(argv[2], argv[3], argv[4]);

        std::cout << "Файл обработан\n";
        return 0;

    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
    }

    return 1;
}