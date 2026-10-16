#include "crypto.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        if (argc > 2)
            throw runtime_error("Укажите только путь к файлу настроек");

        ifstream file(argc == 2 ? argv[1] : "config.txt");

        string modeName, padName, keyText, ivText;
        string input, output, restored, extra;
        int threads;

        if (!(file >> modeName >> padName >> threads
                   >> keyText >> ivText
                   >> input >> output >> restored)) {
            throw runtime_error("Не удалось прочитать настройки");
        }

        if (file >> extra)
            throw runtime_error("Лишние данные в файле настроек");

        if (input == output || input == restored || output == restored)
            throw runtime_error("Имена трёх файлов должны отличаться");

        ModeType mode;

        if (modeName == "ECB")
            mode = ModeType::ECB;
        else if (modeName == "CBC")
            mode = ModeType::CBC;
        else if (modeName == "PCBC")
            mode = ModeType::PCBC;
        else
            throw runtime_error("Режим должен быть ECB, CBC или PCBC");

        PadType pad;

        if (padName == "Zeros")
            pad = PadType::Zeros;
        else if (padName == "ANSIX923")
            pad = PadType::ANSIX923;
        else
            throw runtime_error("Набивка должна быть Zeros или ANSIX923");

        auto code = make_shared<Xtea>(fromHex(keyText));
        Bytes iv = ivText == "-" ? Bytes{} : fromHex(ivText);

        Context box(code, mode, pad, iv);

        box.encrypt(input, output, threads).get();
        cout << "Шифрование завершено: " << output << '\n';

        box.decrypt(output, restored, threads).get();
        cout << "Расшифрование завершено: " << restored << '\n';

        if (readFile(input) != readFile(restored))
            throw runtime_error("Файлы не совпадают");

        cout << "Исходный и восстановленный файлы совпадают.\n";
    }
    catch (const exception& error) {
        cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}