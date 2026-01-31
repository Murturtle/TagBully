// COMMENT TO REMOVE SCREEN
#define HAS_SCREEN_42
#define LED_PIN 8
#define BOOT_BUTTON 9

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Wire.h>
#ifdef HAS_SCREEN_42
#include <SSD1315.h>
SSD1315 display;
#endif
#include <vector>
#include <algorithm>





int airtags = 0;
int apples = 0;
int totalDevices = 0;
bool scanStarted = false;
std::vector<std::string> seenDevices;

static BLEUUID AIR_TAG_SOUND_SERVICE("7DFC9000-7D1C-4951-86AA-8D9728F8D66C");
static BLEUUID AIR_TAG_SOUND_CHARACTERISTIC("7DFC9001-7D1C-4951-86AA-8D9728F8D66C");

BLEAdvertisedDevice* airtagDevice = nullptr;


void drawScreen(const char* status = nullptr) {
    #ifdef HAS_SCREEN_42
    display.fill(0);
    display.drawString(0, 0, String(airtags).c_str());
    display.drawString(16, 0, String(apples).c_str());
    //display.drawString(56, 0, scanStarted ? "SC" : "ST");
    display.drawString(40, 0, String(totalDevices).c_str());

    if (!seenDevices.empty()) {
        display.drawString(0, 13, seenDevices.back().c_str());
    }

    if (status) {
        display.drawString(0, 30, status);
    }

    display.drawRect(0, 11, 72, 11);
    display.display();
    #endif
}


void playAirTagSound(BLEClient* client) {
    BLERemoteService* pService = client->getService(AIR_TAG_SOUND_SERVICE);
    if (!pService) {
        Serial.println("AirTag sound service not found!");
        drawScreen("NO SVC");
        return;
    }

    BLERemoteCharacteristic* soundChar = pService->getCharacteristic(AIR_TAG_SOUND_CHARACTERISTIC);
    if (!soundChar) {
        Serial.println("AirTag sound characteristic not found!");
        drawScreen("NO CHAR");
        return;
    }

    uint8_t soundValue = 175;
    soundChar->writeValue(&soundValue, 1, true);
    Serial.println("AirTag sound triggered!");
    drawScreen("PLAY");
    delay(100);
}

void connectToAirTag(BLEAdvertisedDevice &airtagDevice) {
    BLEClient* client = BLEDevice::createClient();
    Serial.println("Connecting...");
    drawScreen("CONN");

    unsigned long start = millis();
    
    /*if (!client->connectTimeout(&airtagDevice, 10000)) {
        Serial.println("Failed to connect");
        drawScreen("CONN ERR");
        delete client;
        return;
    }*/

    while (!client->isConnected()) {
        if (millis() - start > 5000) {
            Serial.println("Connection timeout!");
            drawScreen("TIMEOUT");
            client->disconnect();
            delete client;
            return;
        }
        delay(50);
    }

    Serial.println("Connected to AirTag");
    drawScreen("CONN OK");
    delay(50);

    playAirTagSound(client);

    client->disconnect();
    delete client;

    Serial.println("Disconnected, moving on");
}

void runScan(uint32_t duration = 5) {
    apples = 0;
    totalDevices = 0;

    BLEScan* pScan = BLEDevice::getScan();
    pScan->clearResults();

    scanStarted = true;
    String msg = "SCAN " + String(duration);
    drawScreen(msg.c_str());
    Serial.println("Scanning...");

    BLEScanResults* results = pScan->start(duration, false);
    Serial.printf("Scan complete, found %d devices\n", results->getCount());
    scanStarted = false;

    for (int i = 0; i < results->getCount(); i++) {
        totalDevices++;
        BLEAdvertisedDevice d = results->getDevice(i);
        String mfgData = d.getManufacturerData();

        uint16_t companyID = (uint8_t)mfgData[1] << 8 | (uint8_t)mfgData[0];

        // manufacturer data
        if (mfgData.length() > 0) {
            Serial.print("MFG Data: ");
            for (size_t j = 0; j < mfgData.length(); j++) {
                Serial.printf("%02X ", (uint8_t)mfgData[j]);
            }

            if(companyID == 0x004C){
                Serial.print("APPLE DEVICE ");
            }

            Serial.println();
        }

        // Is it an AirTag?
        if (mfgData.length() >= 5) {
            if (true) { // Apple 
                apples++;

                uint8_t byte4 = (uint8_t)mfgData[4];
                uint8_t filterByte = 0x10;
                uint8_t maskByte = 0x18;

                bool lastByte = (byte4 & maskByte) == (filterByte & maskByte);


                // Check the AirTag payload 0x12 means find my device, 0x19 means offline finding, XXX10XXX means airtag ???
                if ((uint8_t)mfgData[2] == 0x12 &&
                    (uint8_t)mfgData[3] == 0x19 &&
                    lastByte) {
                    String macAddr = d.getAddress().toString();
                    std::string macStd = std::string(macAddr.c_str());

                    if (std::find(seenDevices.begin(), seenDevices.end(), macStd) == seenDevices.end()) {
                        Serial.println("AirTag found: " + macAddr);
                        airtags++;
                        seenDevices.push_back(macStd);
                        drawScreen("FOUND");

                        connectToAirTag(d);
                    }
                }
            }
        }
    }
    drawScreen("DONE");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(BOOT_BUTTON, INPUT_PULLUP);

    // screen
    #ifdef HAS_SCREEN_42
    Wire.begin(5, 6);
    display.begin();
    display.fill(0);
    display.drawString(0, 0, "Loading");
    display.display();
    #endif
    Serial.println("Starting Bluetooth...");
    BLEDevice::init("ESP32C3");
    

    drawScreen("READY");
}

void loop() {
    if (digitalRead(BOOT_BUTTON) == LOW) {
        digitalWrite(LED_PIN, LOW);
        Serial.println("BOOT button pressed!");
        //seenDevices.clear();
        //airtags = 0;
        //drawScreen("Cleared");
        delay(300);

        runScan(10); // blocking scan !!!
        digitalWrite(LED_PIN, HIGH);
    }
}
