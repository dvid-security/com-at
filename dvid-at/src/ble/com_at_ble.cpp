#include "com_at_ble.h"

bool com_at_ble::clear() {
    return _at.sendCommand("AT+BLECLEAR");
}

bool com_at_ble::begin(const String& name, const String& manu) {
    String cmd = "AT+BLEBEGIN=\"" + name + "\";\"" + manu + "\"";
    return _at.sendCommand(cmd);
}

bool com_at_ble::setName(const String& name) {
    String cmd = "AT+BLESETNAME=\"" + name + "\"";
    return _at.sendCommand(cmd);
}

bool com_at_ble::setMfg(const String& mfg) {
    String cmd = "AT+BLEMFG=\"" + mfg + "\"";
    return _at.sendCommand(cmd);
}

bool com_at_ble::advStart() {
    return _at.sendCommand("AT+BLEADVSTART");
}

bool com_at_ble::advStop() {
    return _at.sendCommand("AT+BLEADVSTOP");
}

bool com_at_ble::scan(int sec, String& out) {
    String cmd = "AT+BLESCAN=" + String(sec);
    return _at.sendCommand(cmd, "OK", 2000 + 1000 * sec, &out);
}

bool com_at_ble::stop() {
    return _at.sendCommand("AT+BLESTOP");
}

bool com_at_ble::gattBuild(const String& svc, const String& chars) {
    String cmd = "AT+GATTBUILD=\"" + svc + "\";" + chars;
    return _at.sendCommand(cmd);
}
