#include "com_at_wifi.h"

bool com_at_wifi::scan(String& result) {
    return _at.sendCommand("AT+CWLAP", "OK", 8000, &result);
}

bool com_at_wifi::connect(const String& ssid, const String& pass) {
    String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"";
    return _at.sendCommand(cmd, "OK", 10000);
}

bool com_at_wifi::status(String& out) {
    return _at.sendCommand("AT+CWSTATE?", "OK", 2000, &out);
}

bool com_at_wifi::disconnect() {
    return _at.sendCommand("AT+CWQAP");
}

bool com_at_wifi::getIP(String& out) {
    return _at.sendCommand("AT+CIFSR", "OK", 2000, &out);
}

bool com_at_wifi::getMode(int& mode) {
    String resp;
    if (_at.sendCommand("AT+CWMODE?", "OK", 2000, &resp)) {
        int idx = resp.indexOf(":");
        if (idx != -1) mode = resp.substring(idx + 1).toInt();
        return true;
    }
    return false;
}

bool com_at_wifi::setMode(int mode) {
    String cmd = "AT+CWMODE=" + String(mode);
    return _at.sendCommand(cmd);
}

bool com_at_wifi::startAP(const String& ssid, const String& pwd, int ch, int ecn) {
    String cmd = "AT+CWSAP=\"" + ssid + "\",\"" + pwd + "\"," + String(ch) + "," + String(ecn);
    return _at.sendCommand(cmd);
}
