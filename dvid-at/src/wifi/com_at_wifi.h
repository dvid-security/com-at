#pragma once
#include "../core/ATCommandBase.h"

class com_at_wifi {
public:
    com_at_wifi(ATCommandBase& at) : _at(at) {}
    bool scan(String& result);
    bool connect(const String& ssid, const String& pass);
    bool status(String& out);
    bool disconnect();
    bool getIP(String& out);
    bool getMode(int& mode);
    bool setMode(int mode);
    bool startAP(const String& ssid, const String& pwd, int ch = 6, int ecn = 3);
private:
    ATCommandBase& _at;
};
