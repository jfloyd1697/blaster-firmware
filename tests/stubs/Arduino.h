#pragma once

#include <cstdint>
#include <sstream>
#include <string>

class DiagnosticTestSerial {
public:
    std::string input;
    std::string output;

    template<class T> void print(const T& item) {
        std::ostringstream stream;
        stream << item;
        output += stream.str();
    }
    template<class T> void println(const T& item) {
        print(item);
        output += '\n';
    }
    void println() { output += '\n'; }
    int available() { return static_cast<int>(input.size()); }
    int read() {
        if (input.empty()) return -1;
        const unsigned char value = static_cast<unsigned char>(input.front());
        input.erase(0, 1);
        return value;
    }
};

extern DiagnosticTestSerial Serial;
unsigned long millis();
void delay(unsigned long milliseconds);
inline void yield() {}
