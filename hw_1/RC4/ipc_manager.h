#pragma once

#include "protocol.h"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <semaphore.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class IPCManager {
public:
    explicit IPCManager(bool server)
        : server(server),
          name("/rc4_" + std::to_string(getuid())) {
        int fd = -1;

        try {
            const int flags = server
                ? O_CREAT | O_EXCL | O_RDWR
                : O_RDWR;

            fd = shm_open(name.c_str(), flags, 0600);

            if (fd == -1) {
                const int code = errno;

                fail(
                    code,
                    std::string(
                        server
                            ? "Не удалось создать общую память "
                            : "Не удалось открыть общую память "
                    ) + name
                );
            }

            created = server;

            if (server &&
                ftruncate(
                    fd,
                    static_cast<off_t>(sizeof(SharedMemory))
                ) != 0) {
                const int code = errno;
                fail(code, "Не удалось задать размер общей памяти");
            }

            struct stat info{};

            if (fstat(fd, &info) != 0) {
                const int code = errno;
                fail(code, "Не удалось получить размер общей памяти");
            }

            if (info.st_size <
                static_cast<off_t>(sizeof(SharedMemory))) {
                throw std::runtime_error(
                    "Размер общей памяти меньше необходимого"
                );
            }

            void* address = mmap(
                nullptr,
                sizeof(SharedMemory),
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                0
            );

            if (address == MAP_FAILED) {
                const int code = errno;
                fail(code, "Не удалось подключить общую память");
            }

            memory = static_cast<SharedMemory*>(address);

            close(fd);
            fd = -1;

            for (unsigned n = 0; n < MAX_SESSIONS; ++n) {
                for (unsigned type = 0; type < 3; ++type) {
                    const unsigned index = n * 3 + type;

                    names[index] =
                        name + "_" + std::to_string(n) +
                        "_" + std::to_string(type);

                    sem_t* semaphore;

                    if (server) {
                        semaphore = sem_open(
                            names[index].c_str(),
                            O_CREAT | O_EXCL,
                            0600,
                            type == 0 ? 1u : 0u
                        );
                    } else {
                        semaphore = sem_open(
                            names[index].c_str(),
                            0
                        );
                    }

                    if (semaphore == SEM_FAILED) {
                        const int code = errno;

                        fail(
                            code,
                            std::string(
                                server
                                    ? "Не удалось создать семафор "
                                    : "Не удалось открыть семафор "
                            ) + names[index]
                        );
                    }

                    semaphores[index] = semaphore;
                }
            }
        } catch (...) {
            if (fd != -1)
                close(fd);

            clear();
            throw;
        }
    }

    ~IPCManager() {
        clear();
    }

    IPCManager(const IPCManager&) = delete;
    IPCManager& operator=(const IPCManager&) = delete;

    Session& session(unsigned index) {
        checkIndex(index);
        return memory->sessions[index];
    }

    sem_t& available(unsigned index) {
        checkIndex(index);
        return *semaphores[index * 3];
    }

    sem_t& request(unsigned index) {
        checkIndex(index);
        return *semaphores[index * 3 + 1];
    }

    sem_t& response(unsigned index) {
        checkIndex(index);
        return *semaphores[index * 3 + 2];
    }

    int findFreeSlot() {
        for (unsigned n = 0; n < MAX_SESSIONS; ++n) {
            int result;

            do {
                result = sem_trywait(&available(n));
            } while (result != 0 && errno == EINTR);

            if (result == 0)
                return static_cast<int>(n);

            const int code = errno;

            if (code != EAGAIN)
                fail(code, "Ошибка захвата сессии");
        }

        return -1;
    }

    void freeSlot(unsigned index) {
        signal(available(index));
    }

    static void wait(sem_t& semaphore) {
        while (sem_wait(&semaphore) != 0) {
            const int code = errno;

            if (code != EINTR)
                fail(code, "Ошибка ожидания семафора");
        }
    }

    static void signal(sem_t& semaphore) {
        if (sem_post(&semaphore) != 0) {
            const int code = errno;
            fail(code, "Ошибка отправки сигнала");
        }
    }

private:
    bool server;
    bool created = false;
    std::string name;

    SharedMemory* memory = nullptr;

    std::array<sem_t*, MAX_SESSIONS * 3> semaphores{};
    std::array<std::string, MAX_SESSIONS * 3> names{};

    [[noreturn]] static void fail(
        int code,
        const std::string& message
    ) {
        throw std::system_error(
            code,
            std::generic_category(),
            message
        );
    }

    static void checkIndex(unsigned index) {
        if (index >= MAX_SESSIONS) {
            throw std::runtime_error(
                "Неверный индекс сессии"
            );
        }
    }

    void clear() noexcept {
        for (unsigned n = 0; n < semaphores.size(); ++n) {
            if (semaphores[n]) {
                sem_close(semaphores[n]);

                // Удаляем только семафоры, созданные этим объектом.
                if (server)
                    sem_unlink(names[n].c_str());

                semaphores[n] = nullptr;
            }
        }

        if (memory) {
            munmap(memory, sizeof(SharedMemory));
            memory = nullptr;
        }

        if (created) {
            shm_unlink(name.c_str());
            created = false;
        }
    }
};