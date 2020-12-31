
#include "config.h"

#include <Wire.h>
#include <TinyGsmClient.h>
#include <SSLClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <algorithm>

#include "ip5306.h"
#include "storage.h"
#include "wall_time.h"
#include "certificates.h"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
bool modemOn = false;
bool modemSetup = false;

TinyGsmClient gsmClient(modem);
SSLClient sslClient(gsmClient, TAs, (size_t) TAs_NUM, pin::misc::sslRandom);
HttpClient httpClient(sslClient, server::host, server::port);
IP5306 pmu(Wire);
PermanentStorage storage;
//Clock wallClock;

void setupModemHardware() {
    // Keep reset high
    pinMode(pin::modem::rst, OUTPUT);
    digitalWrite(pin::modem::rst, HIGH);

    pinMode(pin::modem::powerKey, OUTPUT);
    pinMode(pin::modem::powerOn, OUTPUT);

    // Turn on the Modem power first
    digitalWrite(pin::modem::powerOn, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(pin::modem::powerKey, HIGH);
    delay(100);
    digitalWrite(pin::modem::powerKey, LOW);
    delay(1000);
    digitalWrite(pin::modem::powerKey, HIGH);

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, pin::modem::rx, pin::modem::tx);

    modemOn = true;
}

void serialMonPrintTimestamp() {
    SerialMon.print(wall_time::format(std::chrono::system_clock::now()));
    SerialMon.print(F(" "));
}

void turnOffNetlight() {
    serialMonPrintTimestamp();
    SerialMon.println(F("Turning off SIM800 Red LED..."));
    modem.sendAT("+CNETLIGHT=0");
}

void turnOnNetlight() {
    serialMonPrintTimestamp();
    SerialMon.println(F("Turning on SIM800 Red LED..."));
    modem.sendAT("+CNETLIGHT=1");
}

void logSignalQuality() {
    if (!modemSetup) {
        return;
    }

    int signal = modem.getSignalQuality();

    serialMonPrintTimestamp();
    SerialMon.print(F("Signal quality "));
    SerialMon.println(signal);
}

bool setupModem() {
    if (!modemOn) {
        setupModemHardware();
    }

    if (modemSetup) {
        if (modem.isNetworkConnected()) {
            serialMonPrintTimestamp();
            SerialMon.println(F("Network connected"));
            logSignalQuality();
            return true;
        }

        modemSetup = false;
    }

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    serialMonPrintTimestamp();
    SerialMon.print(F("Initializing modem..."));
    if (!modem.restart()) {
        SerialMon.println(F(" fail"));
        return false;
    }
    SerialMon.println(F(" OK"));

    // Turn off network status lights to reduce current consumption
    turnOffNetlight();

    // The status light cannot be turned off, only physically removed
    //turnOffStatuslight();

    // Or, use modem.init() if you don't need the complete restart
    String modemInfo = modem.getModemInfo();
    serialMonPrintTimestamp();
    SerialMon.print(F("Modem: "));
    SerialMon.println(modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (strlen(gprs::simPIN) && modem.getSimStatus() != 3) {
        modem.simUnlock(gprs::simPIN);
    }

    serialMonPrintTimestamp();
    SerialMon.print(F("Waiting for network..."));
    if (!modem.waitForNetwork(240000L)) {
        SerialMon.println(F(" fail"));
        return false;
    }
    SerialMon.println(F(" OK"));

    // When the network connection is successful, turn on the indicator
    digitalWrite(pin::misc::led, HIGH);

    if (modem.isNetworkConnected()) {
        serialMonPrintTimestamp();
        SerialMon.println(F("Network connected"));
    }

    logSignalQuality();

    modemSetup = true;

    return true;
}

void deepSleep [[ noreturn ]] (std::chrono::milliseconds time){
    serialMonPrintTimestamp();
    SerialMon.print(F("Deep sleep until "));
    SerialMon.println(wall_time::format(std::chrono::system_clock::now() + time));

    if (modemOn) {
        modem.poweroff();
    }

    storage.write();

    esp_sleep_enable_timer_wakeup((uint64_t) std::chrono::duration_cast<std::chrono::microseconds>(time).count());
    esp_deep_sleep_start();

    /*
    The sleep current using AXP192 power management is about 500uA,
    and the IP5306 consumes about 1mA
     */
}

void lightSleep(std::chrono::milliseconds time) {
    serialMonPrintTimestamp();
    SerialMon.print(F("Light sleep until "));
    SerialMon.println(wall_time::format(std::chrono::system_clock::now() + time));

    storage.write();

    esp_sleep_enable_timer_wakeup((uint64_t) std::chrono::duration_cast<std::chrono::microseconds>(time).count());
    esp_light_sleep_start();
}

void sleep(const std::chrono::milliseconds time) {
    if (time.count() <= 0) {
        return;
    } else if (time.count() <= 500) {
        delay((uint32_t) time.count());
    } else if (time.count() <= 10000) {
        lightSleep(time);
    } else {
        deepSleep(time);
    }
}

void sleep(const std::chrono::system_clock::time_point& until) {
    sleep(std::chrono::duration_cast<std::chrono::milliseconds>(until - std::chrono::system_clock::now()));
}

void logResetCause() {
    serialMonPrintTimestamp();
    SerialMon.print(F("Reset cause: "));

    esp_reset_reason_t cause = esp_reset_reason();

#define CASE(x) case x: SerialMon.println(F(#x)); break
    switch (cause) {
            CASE(ESP_RST_UNKNOWN);
            CASE(ESP_RST_POWERON);
            CASE(ESP_RST_EXT);
            CASE(ESP_RST_SW);
            CASE(ESP_RST_PANIC);
            CASE(ESP_RST_INT_WDT);
            CASE(ESP_RST_TASK_WDT);
            CASE(ESP_RST_WDT);
            CASE(ESP_RST_DEEPSLEEP);
            CASE(ESP_RST_BROWNOUT);
            CASE(ESP_RST_SDIO);
        default:
            SerialMon.print(F("Unknown value "));
            SerialMon.println(cause);
    }

#undef CASE
}

void logWakeUpCause() {
    serialMonPrintTimestamp();
    SerialMon.print(F("Wake up cause: "));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

#define CASE(x) case x: SerialMon.println(F(#x)); break
    switch (cause) {
            CASE(ESP_SLEEP_WAKEUP_UNDEFINED);
            CASE(ESP_SLEEP_WAKEUP_ALL);
            CASE(ESP_SLEEP_WAKEUP_EXT0);
            CASE(ESP_SLEEP_WAKEUP_EXT1);
            CASE(ESP_SLEEP_WAKEUP_TIMER);
            CASE(ESP_SLEEP_WAKEUP_TOUCHPAD);
            CASE(ESP_SLEEP_WAKEUP_ULP);
            CASE(ESP_SLEEP_WAKEUP_GPIO);
            CASE(ESP_SLEEP_WAKEUP_UART);
        default:
            SerialMon.print(F("Unknown value "));
            SerialMon.println(cause);
    }

#undef CASE
}

std::chrono::system_clock::time_point bootDelayTime;

void initBootDelay() {
    bootDelayTime = std::chrono::system_clock::now() + std::chrono::seconds(3);
}

bool bootDelay() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    if (now >= bootDelayTime) {
        return false;
    }

    serialMonPrintTimestamp();
    SerialMon.print(F("Booting in "));
    SerialMon.println((int32_t) std::chrono::duration_cast<std::chrono::seconds>(bootDelayTime - now).count());

    constexpr const int Steps = 50;
    for (int i = 0; i < Steps; i++) {
        digitalWrite(pin::misc::led, i < (Steps / 10));

        delay(1000 / Steps);

        if (digitalRead(pin::misc::reset) == 0) {
            SerialMon.println(F("Reset"));

            storage = PermanentStorage();
            storage.write();

            ESP.restart();
        }
    }

    return true;
}

/*
unsigned long getSSLTime() {
    return (unsigned long) std::chrono::duration_cast<std::chrono::seconds>(wallClock.now().time_since_epoch()).count();
}
 */
void setup() {
    std::chrono::system_clock::time_point bootTime = std::chrono::system_clock::now();

    // Set console baud rate
    SerialMon.begin(115200);

    delay(10);

    Wire.begin(pin::i2c::sda, pin::i2c::scl);

    // Start power management
    if (!pmu.setup()) {
        Serial.println(F("PMU setup error"));
    }

    storage.setup();

    /*if (wallClock.init(storage.nextWakeUpTime)) {
        SerialMonPrintTimestamp();
        Serial.println(F("Restored clock"));
    } else {

        Serial.println(F("Could not restore clock"));
    }*/

    // Initialize the indicator as an output
    pinMode(pin::misc::led, OUTPUT);
    digitalWrite(pin::misc::led, LOW);

    pinMode(pin::misc::batterySensor, ANALOG);

    // Some start operations
    // setupModemHardware();

    logResetCause();
    logWakeUpCause();

    std::chrono::system_clock::time_point compileTime = wall_time::getCompileTime();
    serialMonPrintTimestamp();
    SerialMon.print(F("Compile time "));
    SerialMon.println(wall_time::format(compileTime));

    if (esp_reset_reason() == esp_reset_reason_t::ESP_RST_POWERON) {
        serialMonPrintTimestamp();
        SerialMon.println(F("First boot, marking clock as uninitialized"));
        storage.clockInitialized = false;
    } else if (bootTime < compileTime - std::chrono::hours(24)) {
        serialMonPrintTimestamp();
        SerialMon.println(F("Boot time was more than a day before compile time, marking clock as uninitialized"));
        storage.clockInitialized = false;
    }

    initBootDelay();
}

void readVoltageData() {
    if (storage.voltageDataCount >= storage.voltageData.size()) {
        serialMonPrintTimestamp();
        SerialMon.println(F("Could not read voltage data: storage full"));
        return;
    }

    VoltageData& data = storage.voltageData[storage.voltageDataCount];
    memset(&data, 0, sizeof (VoltageData));

    data.timestamp = std::chrono::system_clock::now();
    data.internalPercent = pmu.getBatteryLevel();

    uint32_t sample = 0;
    constexpr const uint32_t samples = 5;
    for (uint32_t i = 0; i < samples; i++) {

        sample += analogRead(pin::misc::batterySensor);
        delay(10);
    }

    float raw = sample / (float) samples;
    data.externalRaw = (uint16_t) raw;

    constexpr const double vref = 3.3f;
    constexpr const double range = 4095;
    constexpr const double r1 = 10010;
    constexpr const double r2 = 2220;
    constexpr const double fudge = 12.58 / 11.86; // 5.18 / 4.48;
    constexpr const float conversion = (float) (fudge * (vref / range) * (r1 + r2) / r2);
    data.external = raw * conversion;

    SerialMon.print(wall_time::format(data.timestamp));
    SerialMon.print(F(" Voltage data "));
    SerialMon.print(storage.voltageDataCount);
    SerialMon.print(F(": "));
    SerialMon.print(data.internalPercent);
    SerialMon.print(F(", "));
    SerialMon.print(data.externalRaw);
    SerialMon.print(F(", "));
    SerialMon.print(data.external);
    SerialMon.println();

    storage.voltageDataCount++;
}

void serialMonLogHttpResponseBody() {
    int bodyLen = httpClient.contentLength();
    SerialMon.print(F("Content length is: "));
    SerialMon.println(bodyLen);
    SerialMon.println();
    SerialMon.println(F("Body returned follows:"));

    constexpr const uint32_t kNetworkTimeout = 30000;
    constexpr const uint32_t kNetworkDelay = 200;

    // Now we've got to the body, so we can print it out
    unsigned long timeoutStart = millis();
    char c;
    // Whilst we haven't timed out & haven't reached the end of the body
    while ((httpClient.connected() || httpClient.available()) &&
            (!httpClient.endOfBodyReached()) &&
            ((millis() - timeoutStart) < kNetworkTimeout)) {
        if (httpClient.available()) {
            c = httpClient.read();
            // Print out this character
            SerialMon.print(c);

            // We read something, reset the timeout counter
            timeoutStart = millis();
        } else {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
        }
    }

    SerialMon.println();
}

int sendSingleVoltageData(const VoltageData& data) {
    StaticJsonDocument<256> doc;

    doc["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(data.timestamp.time_since_epoch()).count();
    doc["internalPercent"] = data.internalPercent;
    doc["external"] = data.external;
    doc["externalRaw"] = data.externalRaw;

    String json;
    serializeJson(doc, json);

    serialMonPrintTimestamp();
    SerialMon.println(F("Posting data:"));
    SerialMon.println(json);
    SerialMon.println();

    httpClient.beginRequest();
    httpClient.post(server::voltageDataResource);
    httpClient.sendBasicAuth(server::secrets::user, server::secrets::password);
    httpClient.sendHeader("Content-Type", "application/json");
    httpClient.sendHeader("Content-Length", json.length());
    httpClient.beginBody();
    httpClient.println(json);
    httpClient.endRequest();

    int status = httpClient.responseStatusCode();
    if (status >= 0) {
        serialMonPrintTimestamp();
        SerialMon.print(F("Got status code: "));
        SerialMon.println(status);

        serialMonLogHttpResponseBody();
    } else {
        SerialMon.print(F("Getting response failed: "));
        SerialMon.println(status);
    }

    return status;
}

bool sendData() {
    if (storage.voltageDataCount == 0) {
        return true;
    }

    if (!modemSetup) {
        serialMonPrintTimestamp();
        SerialMon.println(F("Cannot send data, modem not setup"));
        return false;
    }

    serialMonPrintTimestamp();
    SerialMon.print(F("Connecting to APN: "));
    SerialMon.print(gprs::apn);
    if (!modem.gprsConnect(gprs::apn, gprs::user, gprs::password)) {
        SerialMon.println(F(" fail"));
        return false;
    }
    SerialMon.println(F(" OK"));

    httpClient.connectionKeepAlive();

    {
        size_t count = std::min<size_t>(storage.voltageDataCount, storage.voltageData.size());

        serialMonPrintTimestamp();
        SerialMon.print(F("Sending "));
        SerialMon.print(count);
        SerialMon.println(F(" voltage data"));

        for (size_t i = 0; i < count; i++) {
            sendSingleVoltageData(storage.voltageData[i]);
        }
        storage.voltageDataCount = 0;
        storage.write();
    }

    {
        float lat = 0;
        float lon = 0;
        if (modem.getGsmLocation(&lat, &lon)) {
            serialMonPrintTimestamp();
            SerialMon.print(F("Lat , lon: "));
            SerialMon.print(lat);
            SerialMon.print(F(", "));
            SerialMon.println(lon);
        } else {
            serialMonPrintTimestamp();
            SerialMon.println(F("No location..."));
        }
    }

    httpClient.stop();


    // Shutdown
    modem.gprsDisconnect();
    serialMonPrintTimestamp();
    SerialMon.println(F("GPRS disconnected"));

    return true;
}

void loop() {
    if (bootDelay()) {
        return;
    }

    std::chrono::system_clock::time_point loopStart = std::chrono::system_clock::now();

    {
        int8_t percent = pmu.getBatteryLevel();
        serialMonPrintTimestamp();
        SerialMon.print(F("PMU percent "));
        SerialMon.println(percent, DEC);
    }

    bool needModem = !storage.clockInitialized ||
            storage.voltageDataCount >= storage.voltageData.size() - 1 ||
            loopStart >= storage.nextPushTime;

    if (needModem) {
        if (!setupModem()) {
            lightSleep(std::chrono::milliseconds(10000));
            return;
        }
    }

    if (modemSetup) {
        if (wall_time::init(modem)) {
            bool clockWasInitialized = storage.clockInitialized;
            storage.clockInitialized = true;
            serialMonPrintTimestamp();
            SerialMon.println(F("Reinitialized clock"));

            if (!clockWasInitialized) {
                return;
            }
        } else {
            SerialMon.println(F("Could not reinitialize clock"));
            lightSleep(std::chrono::milliseconds(10000));
            return;
        }
    }

    if (loopStart >= storage.nextCollectTime) {
        readVoltageData();

        wall_time::addDelay(storage.nextCollectTime, data::collectInterval, loopStart);

        serialMonPrintTimestamp();
        SerialMon.print(F("Next collect "));
        SerialMon.println(wall_time::format(storage.nextCollectTime));
    }

    if (loopStart >= storage.nextPushTime || storage.voltageDataCount >= storage.voltageData.size()) {
        if (!sendData()) {
            lightSleep(std::chrono::milliseconds(10000));
            return;
        }

        wall_time::addDelay(storage.nextPushTime, data::pushInterval, loopStart);

        serialMonPrintTimestamp();
        SerialMon.print(F("Next push "));
        SerialMon.println(wall_time::format(storage.nextPushTime));
    }

    // Make the LED blink three times before going to sleep
    int i = 3;
    while (i--) {
        digitalWrite(pin::misc::led, HIGH);
        delay(500);
        digitalWrite(pin::misc::led, LOW);
        delay(500);
    }

    std::chrono::system_clock::time_point wakeUp = std::min(storage.nextPushTime, storage.nextCollectTime);
    sleep(wakeUp);
}
