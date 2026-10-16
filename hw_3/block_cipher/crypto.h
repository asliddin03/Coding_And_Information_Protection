#pragma once

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

enum class ModeType { ECB, CBC, PCBC };
enum class PadType { Zeros, ANSIX923 };

Bytes fromHex(const std::string& text);
Bytes readFile(const std::string& path);
void writeFile(const std::string& path, const Bytes& data);

// Методы алгоритма не должны менять общее состояние при работе потоков.
class Cipher {
public:
    virtual ~Cipher() = default;
    virtual std::size_t size() const = 0;
    virtual Bytes encrypt(const Bytes& data) const = 0;
    virtual Bytes decrypt(const Bytes& data) const = 0;
};

class Xtea : public Cipher {
    std::uint32_t key[4];
    Bytes run(const Bytes& data, bool back) const;

public:
    explicit Xtea(const Bytes& key);

    std::size_t size() const override {
        return 8;
    }

    Bytes encrypt(const Bytes& data) const override {
        return run(data, false);
    }

    Bytes decrypt(const Bytes& data) const override {
        return run(data, true);
    }
};

class Mode {
public:
    virtual ~Mode() = default;

    virtual Bytes run(const Bytes& data, const Cipher& code,
                      const Bytes& iv, int threads, bool back) const = 0;
};

class Ecb : public Mode {
public:
    Bytes run(const Bytes&, const Cipher&, const Bytes&,
              int, bool) const override;
};

class Cbc : public Mode {
public:
    Bytes run(const Bytes&, const Cipher&, const Bytes&,
              int, bool) const override;
};

class Pcbc : public Mode {
public:
    Bytes run(const Bytes&, const Cipher&, const Bytes&,
              int, bool) const override;
};

class Padding {
public:
    virtual ~Padding() = default;

    virtual void add(Bytes& data, std::size_t size) const = 0;

    virtual void remove(Bytes& data, std::size_t size,
                        std::size_t old) const = 0;
};

class Zeros : public Padding {
public:
    void add(Bytes& data, std::size_t size) const override;

    void remove(Bytes& data, std::size_t size,
                std::size_t old) const override;
};

class Ansi : public Padding {
public:
    void add(Bytes& data, std::size_t size) const override;

    void remove(Bytes& data, std::size_t size,
                std::size_t old) const override;
};

class Context {
    std::shared_ptr<const Cipher> code;
    std::shared_ptr<const Mode> mode;
    std::shared_ptr<const Padding> pad;
    Bytes iv;

public:
    Context(std::shared_ptr<const Cipher> code,
            ModeType mode, PadType pad,
            Bytes iv = {}, std::vector<Bytes> extra = {});

    void encrypt(const Bytes& data, Bytes& out, int threads) const;
    void decrypt(const Bytes& data, Bytes& out, int threads) const;

    std::future<void> encrypt(const std::string& from,
                              const std::string& to,
                              int threads) const;

    std::future<void> decrypt(const std::string& from,
                              const std::string& to,
                              int threads) const;
};