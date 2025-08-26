import asyncio
from bleak import BleakScanner, BleakClient

ESP32_ADDR = "A4:CF:12:55:EF:E6"  # adapte ici si besoin (insensible à la casse)

async def explore_ble(addr):
    print(f"Recherche l'ESP32 ({addr})...")
    # Scan rapide pour être sûr que le device est là
    found = False
    devices = await BleakScanner.discover(timeout=10)
    for d in devices:
        if d.address.lower() == addr.lower():
            found = True
            print(f"ESP32 trouvé ! Nom: {d.name}, RSSI: {getattr(d, 'rssi', '-')}")
            break
    if not found:
        print("Device non trouvé. Relance advertising ou vérifie le code côté ESP32.")
        return

    print(f"Connexion à {addr} ...")
    async with BleakClient(addr) as client:
        print("Connecté ! Lecture des services et caractéristiques :")
        for service in client.services:
            print(f"- Service: {service.uuid} ({service.description})")
            for char in service.characteristics:
                props = ",".join(char.properties)
                value = None
                if "read" in char.properties:
                    try:
                        value = await client.read_gatt_char(char.uuid)
                        value = value.hex()
                    except Exception as e:
                        value = f"<read error: {e}>"
                print(f"    - Char: {char.uuid} ({char.description})  Props: {props}  Value: {value}")
    print("Fini.")

if __name__ == "__main__":
    asyncio.run(explore_ble(ESP32_ADDR))
