#pragma once
#include "../core/ATCommandBase.h"

class com_at_ble {
public:
    com_at_ble(ATCommandBase& at) : _at(at) {}
    bool clear();
    bool begin(const String& name, const String& manu);
    bool setName(const String& name);
    bool setMfg(const String& mfg);
    bool advStart();
    bool advStop();
    bool scan(int sec, String& out);
    bool stop();
    bool gattBuild(const String& svc, const String& chars);

private:
    ATCommandBase& _at;
};
