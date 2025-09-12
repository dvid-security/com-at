import serial
import time

# ---- CONFIG ----
SERIAL_PORT = "/dev/ttyUSB0"   # <-- MODIFIE selon ton setup
BAUD = 115200
TIMEOUT = 2

WIFI_SSID = "Livebox-3EC0"
WIFI_PWD = "LK7sHJ4RmpXdDySvKz"

OTA_URL = "http://192.168.1.100/firmware.bin" # Pour la démo, change si besoin

def send_cmd(ser, cmd, wait="OK", delay=1.0, timeout=8):
    print(f"\n>>> {cmd.strip()}")
    ser.reset_input_buffer()
    ser.write((cmd.strip() + '\r\n').encode())
    ser.flush()
    t0 = time.time()
    lines = []
    found = False
    while time.time() - t0 < timeout:
        line = ser.readline().decode(errors="ignore").strip()
        if line:
            print(f"<<< {line}")
            lines.append(line)
            if wait and wait in line:
                found = True
                break
        else:
            time.sleep(0.02)
    if not found and wait:
        print("(!) Réponse attendue non reçue.")
    time.sleep(delay)
    return lines

def test_cmds_general(ser):
    print("\n===== TEST COMMANDES GÉNÉRALES =====")
    send_cmd(ser, "AT")
    send_cmd(ser, "AT+HELP")
    send_cmd(ser, "AT+GMR")
    # On évite AT+RESET qui reboot le module, sauf si vraiment besoin
    # send_cmd(ser, "AT+RESET", wait=None)

def test_wifi(ser):
    print("\n===== TEST WI-FI =====")
    send_cmd(ser, "AT+CWLAP", timeout=12)
    send_cmd(ser, f'AT+CWJAP="{WIFI_SSID}","{WIFI_PWD}"', timeout=10)
    send_cmd(ser, "AT+CWJAP?")
    send_cmd(ser, "AT+CWSTATE?")
    send_cmd(ser, "AT+CIFSR")
    send_cmd(ser, "AT+CWMODE?")
    send_cmd(ser, "AT+CWMODE=3")
    send_cmd(ser, 'AT+CWSAP="TestAP","12345678",6,3')
    send_cmd(ser, "AT+CWQAP")    # Déconnexion AP
    send_cmd(ser, "AT+CWSTATE?")
    send_cmd(ser, "AT+CWJAP?", delay=2)

def test_ble(ser):
    print("\n===== TEST BLE (multi profils) =====")
    # Reset stack BLE
    send_cmd(ser, "AT+BLECLEAR")

    # Séquence 1 : PUB classique Battery Service
    send_cmd(ser, 'AT+GATTBUILD="180F";"2A19:rw";"2A1B:r"')
    send_cmd(ser, 'AT+BLEBEGIN="TestAT";"HELLO123"')
    send_cmd(ser, "AT+BLEADVSTART", delay=5)
    send_cmd(ser, "AT+BLEADVSTOP")

    # Séquence 2 : Nom/manu dynamique
    send_cmd(ser, 'AT+BLESETNAME="DynTest"')
    send_cmd(ser, 'AT+BLEMFG="WORLD456"')
    send_cmd(ser, "AT+BLEADVSTART", delay=4)
    send_cmd(ser, "AT+BLEADVSTOP")

    # Séquence 3 : GATT custom, nom/devices différents
    profils = [
        {
            "gatt": 'AT+GATTBUILD="ABCD";"2A19:rw";"2A1B:r"',
            "name": "DevABCD",
            "manu": "CUSTOM1"
        },
        {
            "gatt": 'AT+GATTBUILD="180D";"2A37:rw";"2A38:r"',
            "name": "HeartESP",
            "manu": "HRT123"
        }
    ]
    for p in profils:
        send_cmd(ser, "AT+BLECLEAR")
        send_cmd(ser, p["gatt"])
        send_cmd(ser, f'AT+BLEBEGIN="{p["name"]}";"{p["manu"]}"')
        send_cmd(ser, "AT+BLEADVSTART", delay=5)
        send_cmd(ser, "AT+BLEADVSTOP")

    # Scan BLE (si tu veux vérifier le scan aussi)
    send_cmd(ser, "AT+BLESCAN=4", wait="OK", delay=2)
    send_cmd(ser, "AT+BLESTOP")

def test_ota(ser):
    print("\n===== TEST OTA (simulation) =====")
    # Lance une vraie MAJ OTA (pour tester seulement si tu as un .bin prêt !)
    send_cmd(ser, f'AT+OTA={OTA_URL}', wait="OTA", delay=2, timeout=20)

def main():
    ser = serial.Serial(SERIAL_PORT, BAUD, timeout=TIMEOUT)
    try:
        test_cmds_general(ser)
        test_wifi(ser)
        test_ble(ser)
        # Décommente pour tester l’OTA pour de vrai
        # test_ota(ser)
        send_cmd(ser, 'AT+CWSAP="TestAP","12345678",6,3')
        time.sleep(20)  # Garde l'AP active un moment
    finally:
        ser.close()
        print("\nPort série fermé.")

if __name__ == "__main__":
    main()



