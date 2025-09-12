#include "com_at.h"

bool com_at::testAT() {
    return sendCommand("AT");
}

bool com_at::getVersion(String& out) {
    return sendCommand("AT+GMR", "OK", 2000, &out);
}

bool com_at::reset() {
    return sendCommand("AT+RESET", "OK", 3000);
}
