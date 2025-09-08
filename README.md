# com-at

**Firmware AT pour ESP-C6**  
Basé sur **ESP-IDF v5.3.2**  
Par [Eun0us - DVID]

---

![Firmware ESP-C6](https://img.shields.io/badge/firmware-esp--idf%20v5.3.2-blue)
![Status](https://img.shields.io/badge/status-alpha-lightgrey)

---

## Présentation

`com-at` est un firmware AT pour l’ESP-C6, conçu pour offrir une interface série simple et puissante :

* Commandes AT universelles
* Wi-Fi : scan, AP, STA
* Bluetooth Low Energy (BLE) : scan, pub, config stack NimBLE
* Idéal pour dev rapide, test hardware, domotique, IoT…
* **Mise à jour OTA du firmware directement via une commande AT**

Futures implémentations :
* MQTT natif, plus de commandes IO, persistance flash, etc.

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
* [Commandes BLE](#commandes-ble)
* [Commandes OTA (mise-à-jour)](#commandes-ota-mise-à-jour)
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

## Commandes BLE

| Commande                                 | Description                                                                | Exemple                                               | Retour attendu         |
| ---------------------------------------- | -------------------------------------------------------------------------- | ----------------------------------------------------- | ---------------------- |
| `AT+BLECLEAR`                            | Reset config BLE, efface tout                                              | `AT+BLECLEAR`                                         | `OK`                   |
| `AT+BLEBEGIN="Nom";"ManuData"`           | Configure le nom + manufacturer data, init NimBLE stack et advertising     | `AT+BLEBEGIN="TestAT";"HELLO123"`                     | `OK`                   |
| `AT+BLEMFG="manudata"`                   | Change manufacturer data pub (ASCII)                                       | `AT+BLEMFG="WORLD456"`                                | `OK`                   |
| `AT+BLESETNAME="nom"`                    | Change dynamiquement le nom BLE (adv)                                      | `AT+BLESETNAME="DynTest"`                             | `OK`                   |
| `AT+BLEADVSTART`                         | Démarre advertising BLE (avec config courante)                             | `AT+BLEADVSTART`                                      | `OK`                   |
| `AT+BLEADVSTOP`                          | Stop advertising BLE                                                       | `AT+BLEADVSTOP`                                       | `OK`                   |
| `AT+BLESTOP`                             | Stop NimBLE stack (BLE OFF)                                                | `AT+BLESTOP`                                          | `OK`                   |
| `AT+BLESCAN=secs`                        | Scan des devices BLE pendant X secondes                                    | `AT+BLESCAN=5`                                        | (résultats)+`OK`       |
| `AT+GATTBUILD="svcUUID";"char:flags";...`| Rebuild GATT dynamique (1 service, N caracs)                               | `AT+GATTBUILD="180F";"2A19:rw";"2A1B:r"`              | `OK`                   |

**Exemple de séquence BLE :**

```plaintext
AT+BLECLEAR
AT+BLEADDSVC=0x180F
AT+BLEADDCHAR=0,0x2A19,0x03
AT+BLELIST
AT+BLEINIT
```

---

## Commandes OTA (mise-à-jour)

**Nouveau !**  
Le firmware supporte la **mise à jour OTA** (Over-The-Air) directement via commande AT.  
Idéal pour déployer tes updates sans ouvrir le boîtier ni brancher de câble !

| Commande        | Description                                  | Exemple                                                         | Retour attendu                        |
| --------------- | -------------------------------------------- | --------------------------------------------------------------- | ------------------------------------- |
| `AT+OTA=<url>`  | Télécharge et flashe le firmware depuis URL  | `AT+OTA=http://monserveur.local/firmware.bin`                   | `OTA START`<br>`OTA OK`<br>(reboot)   |
|                 | (supporte aussi https si certs corrects)     |                                                                 | `OTA ERROR` en cas d’échec            |

> **Astuce :** le firmware téléchargé doit être compilé pour partition OTA, avec la même table de partitionnement.

### Exemple d’utilisation

1. Compiler et déposer le nouveau firmware `.bin` sur ton serveur HTTP ou HTTPS.
2. Connecter l’ESP-C6 à un réseau Wi-Fi (voir commandes Wi-Fi).
3. Depuis le terminal série :

    ```plaintext
   AT+OTA=http://192.168.1.100/firmware.bin
    ```

   ou

   ```plaintext
   AT+OTA=https://mon.serveur/firmware.bin
   ```

4. Attendre :  
   - Réponse `OTA START`,  
   - Si succès : `OTA OK` et redémarrage automatique.  
   - Si erreur : `OTA ERROR`

> **Remarque :** Pour https auto-signé, tu peux ajuster le code OTA pour ignorer le nom commun, ou ajouter ta CA (voir esp_https_ota docs).

---

## Tests automatiques

Routine type (voir `at_ble_test_all()`):

1. Init BLE, pub nom/payload
2. Changement dynamique du nom/payload/manufacturer
3. Start/stop pub, scan, etc.
4. Monitor avec Python sniffer ou via nRF Connect

---

## Exemples de scripts de sniffing BLE

Python + [bleak](https://github.com/hbldh/bleak) pour sniffer/filtrer les pubs :

```python
import asyncio
from bleak import BleakScanner

TARGET_MAC = "" #"A4:CF:12:FF:FF:FF" Target ESP BLE MAC addr

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

GNUv3

---

**Made by Eun0us for DVID**  
Pour toute question : Discord ou Issue git