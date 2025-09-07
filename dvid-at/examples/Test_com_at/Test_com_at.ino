#include <SoftwareSerial.h>
#include "com_at.h"

SoftwareSerial espSerial(2, 3); // RX, TX (adapter selon branchement)
com_at esp(espSerial);

void setup() {
    Serial.begin(115200);
    espSerial.begin(115200);

    if (!esp.testAT()) {
        Serial.println("com-at non détecté !");
        while (1);
    }

    String fw;
    if (esp.getVersion(fw)) Serial.println("Firmware: " + fw);

    // WiFi scan
    String wifiScan;
    if (esp.wifi.scan(wifiScan)) {
        Serial.println("WiFi found:");
        Serial.println(wifiScan);
    }

    // Connexion WiFi
    if (esp.wifi.connect("Livebox-3EC0", "LK7sHJ4RmpXdDySvKz")) {
        Serial.println("WiFi connecté");
        String ip;
        if (esp.wifi.getIP(ip)) Serial.println("IP: " + ip);
    }

    // BLE simple
    esp.ble.clear();
    esp.ble.begin("TestAT", "HELLO123");
    esp.ble.advStart();
    delay(5000);
    esp.ble.advStop();

    // OTA (à tester quand prêt)
    // String otaResp;
    // esp.ota.update("http://192.168.1.100/firmware.bin", otaResp);
    // Serial.println(otaResp);
}

void loop() {}
