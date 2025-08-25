import serial
import time

SERIAL_PORT = "/dev/ttyUSB0"  # À adapter si besoin
BAUDRATE = 115200

# Liste de commandes à tester (mets ici toutes tes commandes)
COMMANDS = [
    "AT",                   # Test basique
    "AT+HELP",              # Liste les commandes
    "AT+CWLAP",             # Scan Wi-Fi
    # "AT+CWJAP=\"SSID\",\"PASSWORD\"",  # Connexion Wi-Fi (à adapter)
    "AT+CWQAP",             # Déconnexion Wi-Fi
    "AT+CWSTATE?",          # État connexion
    "AT+CIFSR",             # Adresse IP
]

# Si tu veux tester la connexion Wi-Fi, décommente et adapte :
SSID = "MonWifi"
PASSWORD = "MonMotDePasse"
COMMANDS.insert(3, f'AT+CWJAP="{SSID}","{PASSWORD}"')

def wait_response(ser, timeout=10.0):
    start = time.time()
    lines = []
    while time.time() - start < timeout:
        if ser.in_waiting:
            line = ser.readline().decode(errors="ignore").strip()
            print(f"    [RECU] '{line}'")
            lines.append(line)
            # Arrête dès qu'on a "OK" ou "ERROR"
            if line.upper() in ["OK", "ERROR"]:
                break
        else:
            time.sleep(0.05)
    return lines

def main():
    with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.5) as ser:
        time.sleep(2)
        ser.reset_input_buffer()
        for cmd in COMMANDS:
            print(f"\n[ENVOI] {cmd}")
            ser.write((cmd + "\r\n").encode())
            ser.flush()
            wait_response(ser, timeout=15.0)  # Timeout généreux pour le Wi-Fi

if __name__ == "__main__":
    main()
