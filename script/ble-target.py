import asyncio
import datetime
from bleak import BleakScanner

TARGET_MAC = ""  # Mets l'adresse MAC cible ici (en majuscules) Ex - "A4:CF:12:55:EF:E6"
DISPLAY_ALL = False  # <-- IMPORTANT

def format_bytes(b):
    if not b: return "-"
    return " ".join(f"{x:02X}" for x in b)

def detection_callback(device, adv_data):
    try:
        mac = device.address.upper()
        if TARGET_MAC is not None and mac != TARGET_MAC:
            return  # Ignore tous les autres
        name = device.name or "-"
        rssi = adv_data.rssi
        mfg_data = adv_data.manufacturer_data
        now = datetime.datetime.now().strftime("%H:%M:%S")
        print(f"\n[{now}] Addr: {mac}")
        print(f"  Name: {name}")
        print(f"  RSSI: {rssi}")
        if mfg_data:
            for k, v in mfg_data.items():
                try:
                    ascii_val = v.decode("ascii")
                except Exception:
                    ascii_val = v.decode("ascii", "replace")
                print(f"  Manufacturer: 0x{k:04X}: {format_bytes(v)}  (ASCII: {ascii_val})")
        else:
            print("  Manufacturer: -")
        print(f"  Service Data: {getattr(adv_data, 'service_data', {})}")
        print(f"  Service UUIDs: {getattr(adv_data, 'service_uuids', [])}")
    except Exception as e:
        print(f"  [ERROR] Exception in callback: {e}")

async def main():
    print("Start BLE Scan Sniffer...")
    scanner = BleakScanner(detection_callback)
    await scanner.start()
    print("[Scanner] Started.")
    try:
        await asyncio.sleep(60)
    finally:
        await scanner.stop()
        print("[Scanner] Stopped.")

if __name__ == "__main__":
    asyncio.run(main())
