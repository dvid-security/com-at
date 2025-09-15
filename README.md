# com-at

> **Firmware AT universel pour ESP32-C6**
> Par Eun0us @ DVID
> Basé sur ESP-IDF v5.3.2

---

![firmware-esp-idf-v5.3.2](https://img.shields.io/badge/firmware-esp--idf%20v5.3.2-blue)
![Status](https://img.shields.io/badge/status-alpha-lightgrey)
![Licence](https://img.shields.io/badge/license-GNUv3-green)

---

## Présentation Firmware

**com-at** est un firmware AT pour l’**ESP32-C6** :

* Commandes AT compatibles modules industriels
* Stack BLE NimBLE dynamique (scan, pub, GATT custom)
* Gestion complète Wi-Fi (scan, AP, STA, état IP)
* Extensible : GPIO, MQTT, etc.

---

## ToDo 

* OTA sécurisé (update via URL HTTP/HTTPS)
* -> AT+RST

## Build & Flash

```sh
idf.py set-target esp32c6
idf.py menuconfig -> Components Config -> Bleutooth [x] -> Nimble (Only) 
idf.py build flash monitor
```

*ESP-IDF v5.1 >= requis*
(Cf. doc officielle Espressif pour l’installation)

---

## Table des matières

* [Commandes générales](#commandes-générales)
* [Commandes Wi-Fi](#commandes-wi-fi)
* [Commandes BLE](#commandes-ble)
* [Commandes OTA](#commandes-ota)
* [Tests automatiques](#tests-automatiques)
* [Licence](#licence)

---

## Commandes générales

| Commande   | Description                | Exemple    | Retour attendu          |
| ---------- | -------------------------- | ---------- | ----------------------- |
| `AT`       | Ping/test interface        | `AT`       | `OK`                    |
| `AT+HELP`  | Liste toutes les commandes | `AT+HELP`  | (liste) + `OK`          |
| `AT+GMR`   | Version firmware           | `AT+GMR`   | `com-at vX.Y.Z`<br>`OK` |
| `AT+RESET` | Redémarrage complet        | `AT+RESET` | (reboot)                |

---

## Commandes Wi-Fi

| Commande                        | Description                         | Exemple                                        | Retour attendu                                 |
| ------------------------------- | ----------------------------------- | ---------------------------------------------- | ---------------------------------------------- |
| `AT+CWLAP`                      | Scan réseaux Wi-Fi                  | `AT+CWLAP`                                     | `+CWLAP:(auth,"SSID",rssi,"MAC",chan)`<br>`OK` |
| `AT+CWJAP="SSID","PASS"`        | Connexion à un réseau               | `AT+CWJAP="Livebox-3EC0","LK7sHJ4RmpXdDySvKz"` | `OK`/`ERROR`                                   |
| `AT+CWJAP?`                     | SSID connecté                       | `AT+CWJAP?`                                    | `+CWJAP:"Livebox-3EC0"`<br>`OK`                |
| `AT+CWQAP`                      | Déconnexion Wi-Fi                   | `AT+CWQAP`                                     | `OK`                                           |
| `AT+CWSTATE?`                   | État connexion (0=non, 1=OK)        | `AT+CWSTATE?`                                  | `+CWSTATE:1`<br>`OK` ou `+CWSTATE:0`<br>`OK`   |
| `AT+CIFSR`                      | Affiche IP locale                   | `AT+CIFSR`                                     | `+CIFSR:"192.168.1.42"`<br>`OK`                |
| `AT+CWMODE?`                    | Mode actuel (1=STA, 2=AP, 3=STA+AP) | `AT+CWMODE?`                                   | `+CWMODE:1`<br>`OK`                            |
| `AT+CWMODE=<x>`                 | Change mode Wi-Fi                   | `AT+CWMODE=1`                                  | `OK`                                           |
| `AT+CWSAP="SSID","PASS",ch,ecn` | Crée un hotspot AP Wi-Fi            | `AT+CWSAP="ESP-AP","12345678",5,3`             | `OK`                                           |
| `AT+WIFISTOP`                   | Stoppe tout le Wi-Fi                | `AT+WIFISTOP`                                  | `OK`                                           |

---

## Commandes BLE

| Commande                                  | Description                                    | Exemple                                  | Retour attendu      |
| ----------------------------------------- | ---------------------------------------------- | ---------------------------------------- | ------------------- |
| `AT+BLECLEAR`                             | Reset complet BLE, efface la stack             | `AT+BLECLEAR`                            | `OK`                |
| `AT+BLEBEGIN="Nom";"ManuData"`            | Configure nom/manu data, init BLE stack et pub | `AT+BLEBEGIN="TestAT";"HELLO123"`        | `OK`                |
| `AT+BLEMFG="manudata"`                    | Change manufacturer data (pub)                 | `AT+BLEMFG="WORLD456"`                   | `OK`                |
| `AT+BLESETNAME="nom"`                     | Change nom BLE dynamique                       | `AT+BLESETNAME="NewName"`                | `OK`                |
| `AT+BLEADVSTART`                          | Démarre advertising                            | `AT+BLEADVSTART`                         | `OK`                |
| `AT+BLEADVSTOP`                           | Arrête advertising                             | `AT+BLEADVSTOP`                          | `OK`                |
| `AT+BLESTOP`                              | Stop NimBLE stack (BLE OFF)                    | `AT+BLESTOP`                             | `OK`                |
| `AT+BLESCAN=secs`                         | Scan des devices BLE pendant X secondes        | `AT+BLESCAN=5`                           | (résultats)<br>`OK` |
| `AT+GATTBUILD="svcUUID";"char:flags";...` | Rebuild GATT dynamique (service+characs)       | `AT+GATTBUILD="180F";"2A19:rw";"2A1B:r"` | `OK`                |
| `AT+BLEMADDR="AA:BB:CC:DD:EE:FF"`         | Change le MAC BLE                              | `AT+BLEMADDR="AA:BB:CC:DD:EE:FF"`        | `OK`                |

> **Flags char GATT** :  `r` = read, `w` = write, `n` = notify

---

## Commandes OTA (Mise-à-jour Firmware)

*Pack de commande a tester*

| Commande       | Description                          | Exemple                                    | Retour attendu          |
| -------------- | ------------------------------------ | ------------------------------------------ | ----------------------- |
| `AT+OTA=<url>` | Télécharge et flashe un firmware OTA | `AT+OTA=http://192.168.1.100/firmware.bin` | `OTA START`<br>`OTA OK` |

> Support HTTP et HTTPS. Firmware cible = partition OTA standard.

---

## Tests automatiques

Routine type (voir `at_ble_test_all()`) :

1. Init BLE, pub nom/payload
2. Changement dynamique du nom/payload/manufacturer
3. Start/stop pub, scan, etc.
4. Monitor via Python/nRF Connect

---

## Exemples de scripts de test BLE

Python + [bleak](https://github.com/hbldh/bleak) pour sniffer BLE :

```python
import asyncio
from bleak import BleakScanner

def detection_callback(device, adv_data):
    print(device, adv_data)

async def main():
    scanner = BleakScanner(detection_callback)
    await scanner.start()
    await asyncio.sleep(10)
    await scanner.stop()

asyncio.run(main())
```

---

## Licence

**GNU v3**

---

**Made by Eun0us for DVID — 2025**
Pour toute question : Discord ou Issue GitHub
