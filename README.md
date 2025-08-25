# com-at

Firmware AT pour ESP-C6, basé sur ESP-IDF.

## Version ESP-IDF

Version utilisée : `v5.3.2`

## Table des matières

* [Commandes générales](#commandes-générales)
* [Commandes Wi-Fi](#commandes-wi-fi)
* [Commandes MQTT](#commandes-mqtt)
* [Commandes BLE (en cours)](#commandes-ble-en-cours)

---

## Commandes générales

| Commande   | Description         |
| ---------- | ------------------- |
| `AT`       | Test de présence    |
| `AT+HELP`  | Liste les commandes |
| `AT+GMR`   | Version firmware    |
| `AT+RESET` | Redémarre l’ESP     |

---

## Commandes Wi-Fi

### `AT+CWLAP`

**Scanne les réseaux Wi-Fi disponibles**

```
AT+CWLAP
```

**Réponse :**

```
+CWLAP:(<ecn>,"<ssid>",<rssi>,"<mac>",<channel>)
...
OK
```

---

### `AT+CWJAP="SSID","PASSWORD"`

**Connecte l’ESP à un réseau Wi-Fi**

**Réponse :** `OK` ou `ERROR`

---

### `AT+CWJAP?`

**Renvoie le SSID connecté actuellement**

```
AT+CWJAP?
```

**Réponse :**

```
+CWJAP:"NomDuSSID"
OK
```

---

### `AT+CWQAP`

**Déconnecte du Wi-Fi courant**

---

### `AT+CWSTATE?`

**Donne l’état de la connexion Wi-Fi**

```
+CWSTATE:<0|1>
OK
```

* `0` : déconnecté
* `1` : connecté

---

### `AT+CIFSR`

**Affiche l’IP locale de l’ESP**

```
+CIFSR:"192.168.1.42"
OK
```

---

### `AT+CWMODE?`

**Renvoie le mode Wi-Fi actuel :**

* `1` = Station (STA)
* `2` = Access Point (AP)
* `3` = Les deux

---

### `AT+CWMODE=<x>`

**Change le mode Wi-Fi** (`x` étant 1, 2 ou 3)

Exemple :

```
AT+CWMODE=3
```

---

### `AT+CWSAP="SSID","PASS",ch,ecn`

**Crée un point d'accès Wi-Fi (hotspot)**

Exemple :

```
AT+CWSAP="MonAP","12345678",5,3
```

* `ch` : canal (1-13)
* `ecn` : type de sécurité (0=OPEN, 3=WPA2)

---

## Commandes MQTT

### `AT+MQTTCONN="host",port,"user","pass"`

**Connexion à un broker MQTT**

---

### `AT+MQTTPUB="topic","payload"`

**Publie un message MQTT**

---

### `AT+MQTTSUB="topic"`

**S'abonne à un topic MQTT**

---

### `AT+MQTTDISC`

**Déconnecte du broker MQTT**

---

## Commandes BLE (en cours)

Fonctionnalités NimBLE à venir :

* `AT+BLESCAN`
* `AT+BLEADV="payload"`
* `AT+BLERD=<handle>`

---

Eun0us - DVID
