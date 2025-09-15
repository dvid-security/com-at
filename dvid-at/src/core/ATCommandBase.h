#pragma once
#include <Arduino.h>

class ATCommandBase {
public:
    ATCommandBase(Stream& serial) : _serial(serial) {}
protected:
    Stream& _serial;

    bool sendCommand(const String& cmd, const String& wait = "OK", uint32_t timeout = 2000, String* response = nullptr) {
        _serial.flush();
        while (_serial.available()) _serial.read(); // Clear buffer

        _serial.println(cmd);
        unsigned long t0 = millis();
        String buf;
        while (millis() - t0 < timeout) {
            while (_serial.available()) {
                char c = _serial.read();
                if (c == '\r') continue;
                if (c == '\n') {
                    if (buf.length() > 0) {
                        if (response) *response += buf + "\n";
                        if (wait.length() && buf.indexOf(wait) != -1) return true;
                        if (buf.indexOf("ERROR") != -1) return false;
                        buf = "";
                    }
                } else {
                    buf += c;
                }
            }
            yield();
        }
        return false;
    }
};
