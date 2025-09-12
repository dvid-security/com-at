#pragma once

#include "core/ATCommandBase.h"
#include "wifi/com_at_wifi.h"
#include "ble/com_at_ble.h"
//#include "ota/com_at_ota.h"

class com_at : public ATCommandBase {
public:
    com_at(Stream& serial)
    : ATCommandBase(serial),
      wifi(*this),
      ble(*this) {}
      //ota(*this) {}

    // Commandes générales
    bool testAT();
    bool getVersion(String& out);
    bool reset();

    // Modules spécialisés
    com_at_wifi wifi;
    com_at_ble  ble;
    //com_at_ota  ota;
};
