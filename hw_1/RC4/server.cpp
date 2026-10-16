#include "ipc_manager.h"
#include "rc4.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

namespace {

volatile std::sig_atomic_t interrupted = 0;
std::atomic<bool> running{true};

void stop(int) {
    interrupted = 1;
}

void worker(IPCManager& ipc, unsigned index) noexcept {
    Session& session = ipc.session(index);
    RC4 cipher;

    try {
        while (running) {
            IPCManager::wait(ipc.request(index));

            if (!running) break;

            session.error = false;

            try {
                switch (session.command) {
                case Command::Begin:
                    if (session.keySize == 0 ||
                        session.keySize > 256) {
                        throw std::runtime_error("Неверный ключ");
                    }

                    cipher.setKey(
                        std::string(session.key, session.keySize)
                    );
                    break;

                case Command::Data:
                    if (session.dataSize > BUFFER_SIZE)
                        throw std::runtime_error(
                            "Неверный размер блока"
                        );

                    cipher.process(
                        session.buffer,
                        session.dataSize
                    );
                    break;

                case Command::End:
                    cipher = RC4{};
                    break;

                default:
                    throw std::runtime_error(
                        "Неизвестная команда"
                    );
                }
            } catch (...) {
                cipher = RC4{};
                session.error = true;
            }

            IPCManager::signal(ipc.response(index));
        }
    } catch (...) {
        running = false;
    }
}

}

int main() {
    try {
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);

        IPCManager ipc(true);
        std::vector<std::thread> threads;

        auto finish = [&] {
            running = false;

            for (unsigned n = 0; n < MAX_SESSIONS; ++n)
                sem_post(&ipc.request(n));

            for (auto& thread : threads) {
                if (thread.joinable())
                    thread.join();
            }
        };

        try {
            for (unsigned n = 0; n < MAX_SESSIONS; ++n)
                threads.emplace_back(worker, std::ref(ipc), n);

            std::cout
                << "Сервер готов. Для выхода нажмите Ctrl+C.\n"
                << std::flush;

            while (running && !interrupted) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100)
                );
            }
        } catch (...) {
            finish();
            throw;
        }

        const bool failed = !running;
        finish();

        return failed ? 1 : 0;

    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "Непредвиденная ошибка\n";
    }

    return 1;
}