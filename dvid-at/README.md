# DVID-AT

**Status:DEV**

Libraire arduino pour piloter notre firmware AT facilement depuis le core **esp32** ou **smt32**

## Developpement

```text
Afin de developper efficacement nos lib et notre firmware nous devons respecter une logique dans nos fichiers 

```

Class **com_at**

```cpp
// Systeme d'heritage
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
```

OTA n'ai pas encore valide côté *esp32-c6* alors on le commente

## Utilisation

Incluer la lib dans /lib global dans le repos training