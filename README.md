# com-at

**Firmware AT pour ESP-C6**
Basé sur **ESP-IDF v5.3.2**
Par \[Eun0us - DVID]

---

![Firmware ESP-C6](https://img.shields.io/badge/firmware-esp--idf%20v5.3.2-blue)
![Status](https://img.shields.io/badge/status-alpha-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Présentation

`com-at` est un firmware AT pour l’ESP-C6, conçu pour offrir une interface série simple et puissante :

* Commandes AT universelles
* Wi-Fi : scan, AP, STA
* MQTT : connect, publish, subscribe
* Bluetooth Low Energy (BLE) : scan, pub, config stack NimBLE
* Idéal pour dev rapide, test hardware, domotique, IoT…

---

## Build & Flash

**Pré-requis :**

* [ESP-IDF v5.3.2](https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32/get-started/)
* Python 3.x, outils ESP

```sh
idf.py build flash monitor
```

---

## Table des matières

* [Commandes générales](#commandes-générales)
* [Commandes Wi-Fi](#commandes-wi-fi)
* [Commandes MQTT](#commandes-mqtt)
* [Commandes BLE](#commandes-ble)
* [Tests automatiques](#tests-automatiques)

---

## Commandes générales

| Commande   | Description         | Exemple    | Retour attendu               |
| ---------- | ------------------- | ---------- | ---------------------------- |
| `AT`       | Test de présence    | `AT`       | `OK`                         |
| `AT+HELP`  | Liste les commandes | `AT+HELP`  | (liste des commandes) + `OK` |
| `AT+GMR`   | Version firmware    | `AT+GMR`   | `com-at vX.Y.Z`<br>`OK`      |
| `AT+RESET` | Redémarre l’ESP     | `AT+RESET` | (ESP redémarre)              |

---

## Commandes Wi-Fi

| Commande                        | Description                         | Exemple                                | Retour attendu                                           |
| ------------------------------- | ----------------------------------- | -------------------------------------- | -------------------------------------------------------- |
| `AT+CWLAP`                      | Scan réseaux Wi-Fi                  | `AT+CWLAP`                             | `+CWLAP:(3,"Livebox",-42,"A0:20:A6:7C:42:13",1)`<br>`OK` |
| `AT+CWJAP="SSID","PASS"`        | Connexion à un réseau               | `AT+CWJAP="Livebox-1234","motdepasse"` | `OK` ou `ERROR`                                          |
| `AT+CWJAP?`                     | SSID connecté                       | `AT+CWJAP?`                            | `+CWJAP:"Livebox-1234"`<br>`OK`                          |
| `AT+CWQAP`                      | Déconnexion Wi-Fi                   | `AT+CWQAP`                             | `OK`                                                     |
| `AT+CWSTATE?`                   | État connexion (0/1)                | `AT+CWSTATE?`                          | `+CWSTATE:1`<br>`OK` ou `+CWSTATE:0`<br>`OK`             |
| `AT+CIFSR`                      | Affiche IP locale                   | `AT+CIFSR`                             | `+CIFSR:"192.168.1.42"`<br>`OK`                          |
| `AT+CWMODE?`                    | Mode actuel (1=STA, 2=AP, 3=STA+AP) | `AT+CWMODE?`                           | `+CWMODE:3`<br>`OK`                                      |
| `AT+CWMODE=<x>`                 | Change le mode                      | `AT+CWMODE=3`                          | `OK`                                                     |
| `AT+CWSAP="SSID","PASS",ch,ecn` | Crée un AP Wi-Fi (hotspot)          | `AT+CWSAP="MonAP","12345678",5,3`      | `OK`                                                     |

---

## Commandes MQTT

| Commande                                | Description        | Exemple                                       | Retour attendu  |
| --------------------------------------- | ------------------ | --------------------------------------------- | --------------- |
| `AT+MQTTCONN="host",port,"user","pass"` | Connexion MQTT     | `AT+MQTTCONN="192.168.1.10",1883,"user","pw"` | `OK` ou `ERROR` |
| `AT+MQTTPUB="topic","payload"`          | Publier            | `AT+MQTTPUB="test/esp","hello"`               | `OK` ou `ERROR` |
| `AT+MQTTSUB="topic"`                    | S’abonner          | `AT+MQTTSUB="test/esp"`                       | `OK` ou `ERROR` |
| `AT+MQTTDISC`                           | Déconnexion broker | `AT+MQTTDISC`                                 | `OK`            |

---

## Commandes BLE

| Commande                        | Description                                                            | Exemple                       | Retour attendu |
| ------------------------------- | ---------------------------------------------------------------------- | ----------------------------- | -------------- |
| `AT+BLECLEAR`                   | Reset config BLE                                                       | `AT+BLECLEAR`                 | `OK`           |
| `AT+BLEADDSVC=uuid`             | Ajoute un service (uuid 16 bits)                                       | `AT+BLEADDSVC=0x180F`         | `OK`           |
| `AT+BLEADDCHAR=sidx,uuid,flags` | Ajoute une caractéristique (flags: 0x01=read, 0x02=write, 0x04=notify) | `AT+BLEADDCHAR=0,0x2A19,0x03` | `OK`           |
| `AT+BLELIST`                    | Liste la config BLE                                                    | `AT+BLELIST`                  | (liste) + `OK` |
| `AT+BLEINIT`                    | Lance la stack BLE et publie                                           | `AT+BLEINIT`                  | `OK`           |
| `AT+BLEDEINIT`                  | Stoppe la stack BLE                                                    | `AT+BLEDEINIT`                | `OK`           |
| `AT+BLESCAN`                    | Scan BLE                                                               | `AT+BLESCAN`                  | (résultats)    |
| `AT+BLEADV="payload"`           | Diffuse un custom payload                                              | `AT+BLEADV="data"`            | `OK`           |
| `AT+BLERD=<handle>`             | Read sur une caractéristique                                           | `AT+BLERD=0x14`               | (donnée lue)   |

**Exemple de séquence BLE :**

```
AT+BLECLEAR
AT+BLEADDSVC=0x180F
AT+BLEADDCHAR=0,0x2A19,0x03
AT+BLELIST
AT+BLEINIT
```

---

## Tests automatiques

Routine type (voir `at_ble_test_all()`):

1. Init BLE, pub nom/payload
2. Changement dynamique du nom/payload/manufacturer
3. Start/stop pub, scan, etc.
4. Monitor avec Python sniffer ou via nRF Connect

---

## Exemples de scripts de sniffing BLE

Python + [bleak](https://github.com/hbldh/bleak) pour sniffer/filtrer les pubs :

```python
import asyncio
from bleak import BleakScanner

TARGET_MAC = "A4:CF:12:55:EF:E6"

def detection_callback(device, adv_data):
    if device.address.upper() == TARGET_MAC:
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

MIT – do what you want, enjoy, collab!

---

**Made by Eun0us for DVID**
Pour toute question : Discord or Issue git
