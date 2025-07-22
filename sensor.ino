#include <ArduinoJson.h>
#include <base64.hpp>

#include <ESP8266WiFi.h>       // For WiFi connectivity
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>


#include <LittleFS.h>
#include <CRC.h>
#include <Wire.h>
#include <RTClib.h>


#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_ADS1X15.h>
//#include <INA228.h>
#include <INA226.h>
#include <vector>
#include <algorithm>
#define DEBUG_ESP_WIFI false
#define DEBUG_ESP_PORT Serial // Change to your preferred Serial port
#define DEBUG_ESP_HTTP_CLIENT true
#define DEBUG_ESP_HTTP_CLIENT true

#define RTC_ADDRESS 0x51


// WiFi and MQTT clients
WiFiClient espClient;
BearSSL::WiFiClientSecure *espClientSecure;
//BearSSL::CertStore certStore; // Unused object

const char* host = "raw.githubusercontent.com";
const int httpsPort = 443;
// Define firmware URLs
//#define sensor_firmware_version_url "https://google.co.uk"
#define sensor_firmware_version_url "https://raw.githubusercontent.com/thermalsystemsltd/sensorfirmware/refs/heads/main/version"
#define sensor_firmware_bin_url "https://github.com/thermalsystemsltd/sensorfirmware/blob/main/sensorfirmware.bin"
const String firmwareBinURL = "https://raw.githubusercontent.com/thermalsystemsltd/sensorfirmware/main/sensorfirmware.bin";

// Trust Root
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEyDCCA7CgAwIBAgIQDPW9BitWAvR6uFAsI8zwZjANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0yMTAzMzAwMDAwMDBaFw0zMTAzMjkyMzU5NTlaMFkxCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKkRpZ2lDZXJ0IEdsb2Jh
bCBHMiBUTFMgUlNBIFNIQTI1NiAyMDIwIENBMTCCASIwDQYJKoZIhvcNAQEBBQAD
ggEPADCCAQoCggEBAMz3EGJPprtjb+2QUlbFbSd7ehJWivH0+dbn4Y+9lavyYEEV
cNsSAPonCrVXOFt9slGTcZUOakGUWzUb+nv6u8W+JDD+Vu/E832X4xT1FE3LpxDy
FuqrIvAxIhFhaZAmunjZlx/jfWardUSVc8is/+9dCopZQ+GssjoP80j812s3wWPc
3kbW20X+fSP9kOhRBx5Ro1/tSUZUfyyIxfQTnJcVPAPooTncaQwywa8WV0yUR0J8
osicfebUTVSvQpmowQTCd5zWSOTOEeAqgJnwQ3DPP3Zr0UxJqyRewg2C/Uaoq2yT
zGJSQnWS+Jr6Xl6ysGHlHx+5fwmY6D36g39HaaECAwEAAaOCAYIwggF+MBIGA1Ud
EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFHSFgMBmx9833s+9KTeqAx2+7c0XMB8G
A1UdIwQYMBaAFE4iVCAYlebjbuYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAd
BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQG
CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKG
NGh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RH
Mi5jcnQwQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29t
L0RpZ2lDZXJ0R2xvYmFsUm9vdEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwC
ATAHBgVngQwBATAIBgZngQwBAgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG
9w0BAQsFAAOCAQEAkPFwyyiXaZd8dP3A+iZ7U6utzWX9upwGnIrXWkOH7U1MVl+t
wcW1BSAuWdH/SvWgKtiwla3JLko716f2b4gp/DA/JIS7w7d7kwcsr4drdjPtAFVS
slme5LnQ89/nD/7d+MS5EHKBCQRfz5eeLjJ1js+aWNJXMX43AYGyZm0pGrFmCW3R
bpD0ufovARTFXFZkAdl9h6g4U5+LXUZtXMYnhIHUfoyMo5tS58aI7Dd8KvvwVVo4
chDYABPPTHPbqjc1qCmBaZx2vN4Ye5DUys/vZwP9BFohFrH/6j/f3IL16/RZkiMN
JCqVJUzKoZHm1Lesh3Sz8W2jmdv51b2EQJ8HmA==
-----END CERTIFICATE-----
)EOF";

X509List *cert;

#define RTC_SDA_PIN 4
#define RTC_SCL_PIN 5
#define EEPROM_ADDR 0x50
#define MAX_EEPROM_ADDRESS 8100
#define FIRMWARE_VERSION "1.0.8"
#define BUFFER_SIZE 512
#define DATA_FILE "/data.txt"
#define LAST_TIME_SYNC_FILE "/lastTimeSync.txt"
#define MAX_SYNC_RETRIES 3       // Maximum number of time sync retries
#define TIME_SYNC_INTERVAL 3600  // 1 hour in seconds
#define LAST_TIME_SYNC_FILE "/lastTimeSync.txt"

// 7-day rolling buffer constants
#define MAX_DAYS_STORAGE 7
#define READINGS_PER_DAY 960  // 24*60*60/90 = 960 readings per day
#define MAX_READINGS (MAX_DAYS_STORAGE * READINGS_PER_DAY)  // 6720 readings
#define MAX_ROLLING_FILE_SIZE (MAX_READINGS * 50)  // Approximate max file size in bytes

// Rolling buffer state
struct RollingBufferState {
    size_t writePosition;  // Current write position in the file
    size_t totalEntries;   // Total number of entries written
    bool isWrapped;        // Whether we've wrapped around at least once
};

RollingBufferState bufferState = {0, 0, false};

////////POWER SWITCHING///
#define THERMISTOR_POWER_PIN 9  // GPIO09 to control thermistor power
#define BATTERY_POWER_PIN 14    // GPIO14 to control battery power

#define LORA_SS_PIN 15          // GPIO15 (D8)
#define LORA_DIO0_PIN 0         // GPIO0 (D3 or RX)

#define LBT_RSSI_THRESHOLD -90  // RSSI threshold for clear channel (adjust as needed)
#define LBT_MAX_RETRIES 5       // Maximum number of retries for channel checking
#define LBT_BACKOFF_TIME 200    // Base backoff time in milliseconds



int loopCounter = 0;  // Counter to track loop cycles

// Initialize the ADS1115
Adafruit_ADS1115 *ads;  
INA226 *ina226;  // Use the correct I2C address, often 0x40

////////////////////////////////////////////////////
unsigned long bootTime = 0; // Global variable to store the time of boot.
bool timeSyncRequired = false;   // Declare the timeSyncRequired flag here globally
unsigned long lastTimeSync = 0;  // Will be updated from LittleFS
const int thermistorPin = A0;
const int loopDelay = 500;
String ackStatus = "N";
RTC_PCF8563 *rtc;
unsigned long lastLoRaTransmission = 0;
unsigned long loRaTransmissionDelay = 500;

String serialNumber = "131214";

String LoRaReadBuffer = "";
String SerialReadBuffer = "";

bool wrapEnabled = true;
bool ackReceived = false;
bool receivingFirmware = false;
bool updateRequired = false;


#define CRC_POLYNOMIAL 0xEDB88320
#define CRC_INITIAL_VALUE 0xFFFFFFFF


bool ackReceivedRecently = false;  // Add this line

enum DeviceState {
  NORMAL_OPERATION,
  UPDATE_MODE,
  RECEIVE_CHUNK,
  SEND_ACK
};

DeviceState currentState = NORMAL_OPERATION;

// Forward declarations for all functions used before their definition
std::vector<int> parseVersionString(const String& version);
String cleanIncomingData(const String& rawData);  // Updated to pass by const reference
void clearFile();
void handleSerialCommands();
void setTimeFromSerial();
float getTemperature();
bool writeToFile(float temperature, unsigned long timestamp, const String& ackStatus);
void readFromFile();
void printDateTime(DateTime dt);
void printTwoDigits(int number);
void processLoRaMessage(const String& message);
bool isValidMessageFormat(const String& message);
void sendACK(int chunkNumber);
void resendLoggedData();
void initializeWritePosition();
unsigned long readLastTimeSync();
void testBinaryUrl();
String readSerialNumberFromLittleFS();
void handleFirmwareUpdate(const String& message);
bool isChannelClear();
void simulateTwoDaysOfData();
void windowedBackfillAllData();
void handleCalibrationCommand(const String& command);
void loadCalibrationTable();
float getInterpolatedOffset(float temperature);
void saveOrUpdateCalibrationPoint(float setpoint, float offset);
void clearCalibrationFile();
void initializeRollingBuffer();
void saveBufferState();
void loadBufferState();
size_t calculateWritePosition(size_t entryNumber);
void cleanupOldData();

// Calibration support
struct CalibrationPoint {
    float setpoint;
    float offset;
};
std::vector<CalibrationPoint> calibrationTable;
#define CALIBRATION_FILE "/calibration.txt"

void loadCalibrationTable() {
    calibrationTable.clear();
    File file = LittleFS.open(CALIBRATION_FILE, "r");
    if (!file) {
        Serial.println("No calibration file found. Using default (no offset).");
        return;
    }
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        int comma = line.indexOf(',');
        if (comma == -1) continue;
        float setpoint = line.substring(0, comma).toFloat();
        float offset = line.substring(comma + 1).toFloat();
        calibrationTable.push_back({setpoint, offset});
    }
    file.close();
}

void saveOrUpdateCalibrationPoint(float setpoint, float offset) {
    loadCalibrationTable();
    bool found = false;
    for (auto& cp : calibrationTable) {
        if (abs(cp.setpoint - setpoint) < 0.01) {
            cp.offset = offset;
            found = true;
            break;
        }
    }
    if (!found) calibrationTable.push_back({setpoint, offset});
    File file = LittleFS.open(CALIBRATION_FILE, "w");
    for (const auto& cp : calibrationTable) {
        file.printf("%.2f,%.2f\n", cp.setpoint, cp.offset);
    }
    file.close();
}

void clearCalibrationFile() {
    LittleFS.remove(CALIBRATION_FILE);
    calibrationTable.clear();
}

float getInterpolatedOffset(float measured) {
    if (calibrationTable.empty()) return 0.0;
    std::sort(calibrationTable.begin(), calibrationTable.end(),
              [](const CalibrationPoint& a, const CalibrationPoint& b) { return a.setpoint < b.setpoint; });
    if (measured <= calibrationTable.front().setpoint) return calibrationTable.front().offset;
    if (measured >= calibrationTable.back().setpoint) return calibrationTable.back().offset;
    for (size_t i = 1; i < calibrationTable.size(); ++i) {
        if (measured < calibrationTable[i].setpoint) {
            float x0 = calibrationTable[i-1].setpoint, y0 = calibrationTable[i-1].offset;
            float x1 = calibrationTable[i].setpoint, y1 = calibrationTable[i].offset;
            return y0 + (measured - x0) * (y1 - y0) / (x1 - x0);
        }
    }
    return 0.0;
}

void handleCalibrationCommand(const String& command) {
    if (!command.startsWith("CAL:")) return;
    int s1 = command.indexOf(':', 4);
    if (s1 == -1) return;
    String serial = command.substring(4, s1);
    if (serial != serialNumber) return;
    String payload = command.substring(s1 + 1);
    if (payload == "CLEAR") {
        clearCalibrationFile();
        Serial.println("Calibration file cleared.");
        loadCalibrationTable();
        return;
    }
    int comma = payload.indexOf(',');
    if (comma == -1) return;
    float setpoint = payload.substring(0, comma).toFloat();
    float offset = payload.substring(comma + 1).toFloat();
    saveOrUpdateCalibrationPoint(setpoint, offset);
    Serial.printf("Calibration updated: %.2f -> %.2f\n", setpoint, offset);
    loadCalibrationTable();
}

///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

void setup() {
  WiFi.forceSleepBegin();
  WiFi.mode(WIFI_OFF);  // Optional redundancy
  delay(1);             // Give time for shutdown
  system_update_cpu_freq(40); // Set CPU to 40 MHz
  
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());

  // Initialize the certificate
  cert = new X509List(trustRoot);
  if (cert == nullptr) {
    Serial.println("Failed to allocate memory for X509List!");
    while(1) delay(100);
  }
  yield();

  // GPIO12 setup
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW); // Ensure GPIO10 starts low at power-up


if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS. Cannot proceed.");
    while (1); // Halt if mounting fails
}
loadCalibrationTable();
yield();
  // Start serial communication for debugging
  Serial.begin(115200);
  bootTime = millis(); // Record the boot time
 // delay(20);  // Give time for the ESP to stabilize after waking
 // Serial.println("Booting");
  //delay(20);

  // Initialize rolling buffer
  initializeRollingBuffer();

  

  // Reinitialize I2C bus
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);  // SDA and SCL pin for RTC
  //Serial.println("I2C bus reinitialized.");

  // Initialize the RTC
  rtc = new RTC_PCF8563();
  if (rtc == nullptr) {
    Serial.println("Failed to allocate memory for RTC!");
    while(1) delay(100);
  }
  rtc->begin();
  yield();
  //Serial.println("RTC initialized.");

  // Test RTC communication
  if (!Wire.requestFrom(0x51, 7)) {  // 0x51 is the address of PCF8563
    Serial.println("Failed to communicate with the RTC over I2C.");
  } else {
    Serial.println("RTC communication successful.");
  }
  yield();






  // Initialize ADS1115
  ads = new Adafruit_ADS1115();
  if (ads == nullptr) {
    Serial.println("Failed to allocate memory for ADS1115!");
    while(1) delay(100);
  }
  if (!ads->begin()) {
    Serial.println("Failed to initialize ADS1115");
    while (1);
  }
  ads->setGain(GAIN_ONE); // Set gain for ADS1115
  yield();

  // Initialize the INA226
  ina226 = new INA226(0x41);
  if (ina226 == nullptr) {
    Serial.println("Failed to allocate memory for INA226!");
    while(1) delay(100);
  }
  if (!ina226->begin()) {
    Serial.println("Failed to initialize INA226!");
    while (1);  // Halt if INA226 fails
  }
  yield();


// Attempt to read serial number from LittleFS
    String storedSerialNumber = readSerialNumberFromLittleFS();
    yield();
    if (!storedSerialNumber.isEmpty()) {
        serialNumber = storedSerialNumber.c_str();  // Override the hardcoded serial number
        Serial.println("Overridden hardcoded serial number with: " + storedSerialNumber);
    } else {
        //Serial.println("Using hardcoded serial number: " + String(serialNumber));
    }





  // Initialize LoRa settings
  LoRa.setPins(LORA_SS_PIN, LORA_DIO0_PIN);  // SS and DIO0 pin setup
  LoRa.setSpreadingFactor(7);      // SF7
  LoRa.setSignalBandwidth(125E3);  // 125 kHz
  LoRa.setCodingRate4(5);          // Coding Rate 4/5
  LoRa.setTxPower(1, PA_OUTPUT_PA_BOOST_PIN);  // Set power to 10 dBm (10 mW) using PA_BOOST

  if (!LoRa.begin(433E6)) {  // Set frequency to 433 MHz
    Serial.println("Starting LoRa failed!");
    while (1);  // Stop further execution if LoRa fails
  }

  // Check if RTC lost power
  if (rtc->lostPower()) {
    Serial.println("RTC is not running! Setting time sync required.");
    timeSyncRequired = true;
  } else {
    // Log RTC time at boot
    DateTime bootTime = rtc->now();
    Serial.print("RTC From Boot: ");
    printDateTime(bootTime);

    // Initialize LittleFS and format if needed
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS, formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("Failed to mount LittleFS after formatting");
            return;
        }
    }

    // Check LittleFS for last valid time
    File file = LittleFS.open("/lastValidTime.txt", "r");
    yield();
    if (file) {
        String savedTimeStr = file.readStringUntil('\n');
        unsigned long savedTime = savedTimeStr.toInt();
        if (savedTime > 0) {
            DateTime littlefsTime(savedTime);
            Serial.println("adjust RTC via LittleFS: ");
            printDateTime(littlefsTime);

            // Calculate time difference between RTC boot time and LittleFS time
            unsigned long timeDifference = abs((long)(bootTime.unixtime() - littlefsTime.unixtime()));

            if (timeDifference > 300) {  // 5 minutes in seconds
                Serial.println("Significant time difference detected. Setting time sync required.");
                timeSyncRequired = true;
            } else {
                Serial.println(" No significant difference detected.");
            }
        } else {
            Serial.println("No valid time in LittleFS; time sync required.");
            timeSyncRequired = true;
        }
        file.close();
    } else {
        Serial.println("No saved time in LittleFS; time sync required.");
        timeSyncRequired = true;
    }
  }




////////////////////////////////////  /////////////////////////////////////////END RTC TEST ALRMS


  // Reserve memory for buffers
  LoRaReadBuffer.reserve(256);
  SerialReadBuffer.reserve(256);

  // Log firmware version
  Serial.print("Firmware Version: ");
  Serial.println(FIRMWARE_VERSION);
//////////////////////////////////////////////////////////////////////////

// Initialize BearSSL Secure Client
  espClientSecure = new BearSSL::WiFiClientSecure();
  if (espClientSecure == nullptr) {
    Serial.println("Failed to allocate memory for BearSSLClient!");
    while(1) delay(100);
  }

  espClientSecure->setInsecure(); // Temporarily bypass certificate validation for debugging
  espClientSecure->setTimeout(15000); // Set a timeout of 15 seconds


}


/////END SETUP ////

// Helper function to print DateTime
void printDateTime(DateTime dt) {
  Serial.print(dt.year());
  Serial.print("-");
  printTwoDigits(dt.month());
  Serial.print("-");
  printTwoDigits(dt.day());
  Serial.print(" ");
  printTwoDigits(dt.hour());
  Serial.print(":");
  printTwoDigits(dt.minute());
  Serial.print(":");
  printTwoDigits(dt.second());
}

// Helper function to print two digits
void printTwoDigits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

// Helper function to convert Unix time to DateTime
DateTime unixTimeToDateTime(unsigned long unixTime) {
  return DateTime(unixTime);  // DateTime class handles the conversion
}







void loop() {
    loadCalibrationTable();
    unsigned long loopStartTime = millis(); // Record the time at the start of the loop
    loopCounter++;  // Increment the loop counter (if needed for other purposes)
    //Serial.print("Loop counter: ");
    //Serial.println(loopCounter);
    //checkForFirmwareUpdate(); // This is now handled in the ACK/response processing section
    handleSerialCommands();

    int packetSize = LoRa.parsePacket();
    if (packetSize > 0) {
        String loraMsg = "";
        while (LoRa.available()) loraMsg += (char)LoRa.read();
        String cleanedMsg = cleanIncomingData(loraMsg);

        // 1. Handle calibration command first
        if (cleanedMsg.startsWith("CAL:")) {
            handleCalibrationCommand(cleanedMsg);
            // SKIP further LoRa message processing for this loop
            return; // This ensures the rest of the loop is not executed for CAL commands
        }

        // 2. Handle other message types (ACK, UPDATE, etc.)
        if (cleanedMsg.startsWith(String(serialNumber) + ",")) {
            Serial.println("Serial number matches.");

            // Extract the datetime and update flag
            int firstCommaIndex = cleanedMsg.indexOf(",");
            int secondCommaIndex = cleanedMsg.indexOf(",", firstCommaIndex + 1);

            // Ensure we have valid comma positions
            if (firstCommaIndex != -1 && secondCommaIndex != -1) {
              String datetimeStr = cleanedMsg.substring(firstCommaIndex + 1, secondCommaIndex);  // Extract datetime string
              String updateFlag = cleanedMsg.substring(secondCommaIndex + 1);                    // Extract the "UPDATE" flag

              Serial.println("Extracted datetime: " + datetimeStr);
              Serial.println("Extracted flag: " + updateFlag);

              // Check if the message ends with "UPDATE"
              if (updateFlag == "UPDATE") {
                Serial.println("UPDATE flag detected. Processing time update...");

                // Parse the datetime string and update the RTC
                DateTime newDateTime = parseDateTime(datetimeStr);

                if (newDateTime.year() > 2000) {  // Basic validity check
                  if (isBST(newDateTime)) {       // Adjust for BST after receiving time sync message
                    //newDateTime = newDateTime + TimeSpan(0, 1, 0, 0);  // Add one hour
                    Serial.println("BST is active. Adjusting time by +1 hour???");
                  }
                  rtc->adjust(newDateTime);  // Adjust RTC to new datetime
                  //Serial.println("RTC updated with new datetime: " + datetimeStr);
                } else {
                  Serial.println("Invalid datetime received.");
                }
              } else {
                Serial.println("No UPDATE flag detected.");
              }
            } else {
              Serial.println("Invalid message format (missing or misplaced commas).");
            }
        } else {
            Serial.println("Message does not match serial number.");
        }
        // After handling a non-CAL message, continue with the rest of the loop as normal
    }

    //////////////////////////// TIME SYNC ////////////////////////////
    if (timeSyncRequired) {
        Serial.println("Time sync required. Sending time sync...");
        sendTimeSyncCommand();  // Send time sync request to the base station

        // Wait for the time sync response
        unsigned long syncWaitStartTime = millis();
        bool syncReceived = false;

        while (millis() - syncWaitStartTime < 200) {  // Wait up to 5 seconds for the response
            int packetSize = LoRa.parsePacket();
            if (packetSize > 0) {
                String response = "";
                while (LoRa.available()) {
                    response += (char)LoRa.read();
                }
                Serial.println("Received time sync response: " + response);
                
                String cleanedResponse = cleanIncomingData(response);
                if (cleanedResponse.startsWith("UPDATE:START")) {
                    handleFirmwareUpdate(cleanedResponse);
                } else {
                    processTimeSyncResponse(cleanedResponse);  // Process the time sync response
                }
                
                syncReceived = true;
                break;
            }
        }

        if (!syncReceived) {
            Serial.println("No time sync response received.");
        } else {
            // Save the last time sync to LittleFS
            writeLastTimeSync(rtc->now().unixtime());  // Record the current time as the last sync time
        }

        // Reset the time sync flag
        timeSyncRequired = false;

        // Directly proceed with temperature transmission after time sync
        Serial.println("Time sync completed. Proceeding with temperature transmission.");
    }

    //////////////////////////// TEMPERATURE MEASUREMENT /////////////////////////
    unsigned long currentMillis = millis();

    // Check if it's time to transmit temperature based on the loop or the recent time sync
    if (currentMillis - lastLoRaTransmission >= loRaTransmissionDelay || timeSyncRequired == false) {
        lastLoRaTransmission = currentMillis;
        //Serial.println("Starting temperature and voltage measurement...");

        // Get the temperature reading
        float temperature = getTemperature();
        float offset = getInterpolatedOffset(temperature);
        float calibratedTemperature = temperature + offset;
        Serial.print("Raw temperature: ");
        Serial.print(temperature, 2);
        Serial.print(" | Applied offset: ");
        Serial.print(offset, 2);
        Serial.print(" | Calibrated temperature: ");
        Serial.println(calibratedTemperature, 2);

        // Get the battery voltage
        float batteryVoltage = readBatteryVoltageINA();
        Serial.print("Battery Voltage: ");
        Serial.print(batteryVoltage, 2);
        Serial.println(" V");
/////////////////////////////////////////////////////////////NEW LBT CODE////////////////////////////////////////

       // Prepare the LoRa data payload
    String loRaData = "T:" + String(calibratedTemperature, 2) + ",V:" + String(batteryVoltage, 2) + ",S:" + serialNumber + ",FW:" + FIRMWARE_VERSION;

    // Sanitize and calculate CRC for the LoRa message
    String sanitizedLoRaData = sanitizeData(loRaData);
    uint32_t crcValue = crc32((const uint8_t*)sanitizedLoRaData.c_str(), sanitizedLoRaData.length());
    String crcString = String(crcValue, HEX);

    // Final LoRa packet with CRC and data
    loRaData = crcString + "," + loRaData;
   // Serial.print("Prepared Data: " + loRaData);
   // Serial.println();
    // Implement Listen Before Talk (LBT)
    int retries = 0;
    bool channelClear = false;

    while (retries < LBT_MAX_RETRIES) {
        if (isChannelClear()) {
            channelClear = true;
            break;
        }
        // If the channel is busy, wait for a random backoff period
        int backoff = LBT_BACKOFF_TIME + random(0, LBT_BACKOFF_TIME);
        Serial.println("Channel busy. Waiting for ");
        Serial.print(backoff);
        Serial.println(" ms");
        delay(backoff);
        retries++;
    }

    if (channelClear) {
        //Serial.println("Channel is clear. Transmitting data...");
        LoRa.beginPacket();
        LoRa.print(loRaData);
        LoRa.endPacket();
        Serial.println("Data transmitted successfully.");
    } else {
        Serial.println("Failed to transmit data. Channel is busy after maximum retries.");
    }

///////////////////////////////////////////////////////////////NEW LBT CODE ///////////////////////////////////////////
        //Serial.println("Checking for incoming ACK...");

        // Non-blocking ACK waiting
        unsigned long ackWaitStartTime = millis();
        ackReceived = false;

        while (millis() - ackWaitStartTime < 300) {
            yield();  // Allow background tasks to run to avoid WDT reset

            // Check if ACK or other packet received
            int ackPacketSize = LoRa.parsePacket();
            if (ackPacketSize > 0) {
                String loRaResponse = "";
                while (LoRa.available()) {
                    loRaResponse += (char)LoRa.read();
                }
                //Serial.println("LoRa Response: " + loRaResponse);

                String trimmedLoRaResponse = cleanIncomingData(loRaResponse);

                // ADD THIS:
                if (trimmedLoRaResponse.startsWith("CAL:")) {
                    handleCalibrationCommand(trimmedLoRaResponse);
                    continue; // Don't treat as ACK, keep waiting
                }

                // Process ACK as before...
                if (trimmedLoRaResponse.endsWith("ACK") && isValidMessageFormat(trimmedLoRaResponse)) {
                    ackStatus = "y";
                    ackReceived = true;
                    ackReceivedRecently = true;
                    Serial.println("ACK received successfully.");
                    checkForBackfill();
                    break;
                } else if (trimmedLoRaResponse.endsWith("ACK:N")) {
                    ackStatus = "n";
                    ackReceived = false;
                    Serial.println("Negative ACK received.");
                    break;
                } else if (trimmedLoRaResponse.startsWith("UPDATE:START")) {
                    handleFirmwareUpdate(trimmedLoRaResponse);
                } else {
                    ackStatus = "?";
                    Serial.println("Unrecognized LoRa message format: " + trimmedLoRaResponse);
                }
            }
        }

        if (!ackReceived) {
            Serial.println("No ACK received within timeout.");
            ackStatus = "n";
        }

        if (!writeToFile(calibratedTemperature, rtc->now().unixtime(), ackStatus)) {
            Serial.println("Failed to write data to file!");
        }
    } else {
        Serial.println("Temperature transmission not triggered yet.");
    }

    //////////////////////////// POWER OFF & TIMER SIGNAL ////////////////////////////

    LoRa.sleep(); // Forces module to low-power mode


    pinMode(10, OUTPUT); // allow via AND GATE HIGH
    digitalWrite(10, HIGH); // allow via AND GATE HIGH
    Serial.println("TURN OFF");
    
    delay(200); 
    
    //digitalWrite(10, LOW); // Optionally, reset GPIO10 to low
}





/////////////////////////////////////////////END MAIN LOOP/////////////////////////////

uint32_t calculateCRC(const String& message) {
  String sanitizedMessage = sanitizeData(message);
  return crc32((const uint8_t*)sanitizedMessage.c_str(), sanitizedMessage.length(), CRC_INITIAL_VALUE, CRC_POLYNOMIAL);
}

std::vector<int> parseVersionString(const String& version) {
  std::vector<int> versionParts;
  int start = 0;
  for (int i = 0; i < version.length(); ++i) {
    if (version[i] == '.' || version[i] == 'v') {
      versionParts.push_back(version.substring(start, i).toInt());
      start = i + 1;
    }
  }
  versionParts.push_back(version.substring(start).toInt());
  return versionParts;
}

void processLoRaMessage(const String& message) {
  Serial.println("Processing LoRa message: " + message);

  // Check if the message starts with the serial number followed by a comma
  if (message.startsWith(String(serialNumber) + ",")) {
    Serial.println("Serial number matches.");

    // Extract the datetime and update flag
    int firstCommaIndex = message.indexOf(",");
    int secondCommaIndex = message.indexOf(",", firstCommaIndex + 1);

    // Ensure we have valid comma positions
    if (firstCommaIndex != -1 && secondCommaIndex != -1) {
      String datetimeStr = message.substring(firstCommaIndex + 1, secondCommaIndex);  // Extract datetime string
      String updateFlag = message.substring(secondCommaIndex + 1);                    // Extract the "UPDATE" flag

      Serial.println("Extracted datetime: " + datetimeStr);
      Serial.println("Extracted flag: " + updateFlag);

      // Check if the message ends with "UPDATE"
      if (updateFlag == "UPDATE") {
        Serial.println("UPDATE flag detected. Processing time update...");

        // Parse the datetime string and update the RTC
        DateTime newDateTime = parseDateTime(datetimeStr);

        if (newDateTime.year() > 2000) {  // Basic validity check
          if (isBST(newDateTime)) {       // Adjust for BST after receiving time sync message
            //newDateTime = newDateTime + TimeSpan(0, 1, 0, 0);  // Add one hour
            Serial.println("BST is active. Adjusting time by +1 hour???");
          }
          rtc->adjust(newDateTime);  // Adjust RTC to new datetime
          //Serial.println("RTC updated with new datetime: " + datetimeStr);
        } else {
          Serial.println("Invalid datetime received.");
        }
      } else {
        Serial.println("No UPDATE flag detected.");
      }
    } else {
      Serial.println("Invalid message format (missing or misplaced commas).");
    }
  } else {
    Serial.println("Message does not match serial number.");
  }
}

bool isValidMessageFormat(const String& message) {
  int comma1Index = message.indexOf(',');
  int comma2Index = message.indexOf(',', comma1Index + 1);

  if (comma1Index != -1 && comma2Index != -1) {
    String crcString = message.substring(0, comma1Index);
    String receivedSerialNumber = message.substring(comma1Index + 1, comma2Index);
    String ackString = message.substring(comma2Index + 1);

    if (crcString.length() > 0 && receivedSerialNumber == serialNumber && ackString.startsWith("ACK")) {
      return true;
    }
  }

  Serial.println("Received non-ACK message or format mismatch: " + message);
  return false;
}

void sendACK(int chunkNumber) {
  String ackMessage = "ACK:" + String(chunkNumber) + "," + serialNumber + "\n";
  LoRa.beginPacket();
  LoRa.print(ackMessage);
  LoRa.endPacket();
  Serial.println("Sent ACK for chunk: " + String(chunkNumber));
  Serial.println("ACK Message: " + ackMessage);  // Print the ACK message being sent
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    Serial.print("Received command: ");
    Serial.println(command);

    if (command == "AT" || command == "AT+RX") {
      LoRa.beginPacket();
      LoRa.print(command);
      LoRa.endPacket();
      Serial.print("Command Executed");
    } else if (command == "loopforever") {
      wrapEnabled = !wrapEnabled;
      Serial.print("Circular buffer wrapping ");
      Serial.println(wrapEnabled ? "enabled" : "disabled");
    } else if (command == "read") {
      readFromFile();
    } else if (command == "clear") {
      clearFile();
      Serial.println("File cleared.");
    } else if (command == "settime") {
      setTimeFromSerial();
    } else if (command == "getdate") {
      DateTime now = rtc->now();
      Serial.print("Current Date and Time: ");
      printDateTime(now);
      Serial.println();
    } else if (command == "updatetime") {
      Serial.println("Triggering time sync...");
      timeSyncRequired = true;  // Trigger time sync via serial command
    } else if (command == "simulate7days") {
      simulateTwoDaysOfData();
    } else {
      Serial.println("Invalid command. Available commands: AT, AT+RX, loopforever, read, clear, settime, getdate, updatetime, simulate7days");
    }

    SerialReadBuffer = "";
  }
}


void setTimeFromSerial() {
  Serial.println("Enter date and time in YYYY-MM-DD HH:MM:SS format:");
  String input = Serial.readStringUntil('\r');
  int year, month, day, hour, minute, second;
  if (sscanf(input.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    DateTime newTime(year, month, day, hour, minute, second);
    rtc->adjust(newTime);
    Serial.println("RTC time has been set successfully.");
  } else {
    Serial.println("Invalid date and time format!");
  }
}

float getTemperature() {
  const float referenceResistor = 20000.0;  // Reference resistor (20kΩ)
  const float nominalResistance = 5000.0;   // Thermistor resistance at 25°C (5kΩ)
  const float nominalTemperature = 25.0;    // Nominal temperature (25°C)
  const float betaCoefficient = 3950.0;     // Beta coefficient (consult thermistor datasheet)
  
  // Add a delay for voltage stabilization (allow thermistor circuit to stabilize)
  delay(20);  // 20 ms delay to ensure voltage stabilizes after turning on the circuit

  // Take multiple readings and average them for better accuracy
  const int numReadings = 5;
  float totalVoltage = 0.0;
  int validReadings = 0;
  
  for (int i = 0; i < numReadings; i++) {
    // Read the ADC value from the thermistor pin using ADS1115
    int16_t adcReading = ads->readADC_SingleEnded(0);  // Read from AIN0

    // Convert the 16-bit ADC value to voltage (assuming GAIN_ONE is set, range is +-4.096V)
    float measuredVoltage = adcReading * (4.096 / 32768.0);  // ADS1115 is 16-bit ADC

    // Ensure the measured voltage is within the valid range
    if (measuredVoltage > 0.0 && measuredVoltage < 3.3) {
      totalVoltage += measuredVoltage;
      validReadings++;
    }
    
    delay(5); // Small delay between readings
  }
  
  if (validReadings == 0) {
    Serial.println("No valid voltage readings! Returning default value.");
    return 0.0;
  }
  
  float measuredVoltage = totalVoltage / validReadings;

  // Calculate the thermistor resistance using the actual voltage
  float thermistorResistance = referenceResistor * (3.3 / measuredVoltage - 1);
  
  // Ensure the calculated resistance is positive and reasonable
  if (thermistorResistance <= 0.0) {
    Serial.println("Thermistor resistance out of range! Returning default value.");
    return 0.0;  // Return default value if resistance is invalid
  }

  // Use the Beta parameter equation to calculate the temperature in Kelvin
  float temperatureK = 1.0 / ((log(thermistorResistance / nominalResistance) / betaCoefficient) + (1.0 / (nominalTemperature + 273.15)));
  
  // Convert Kelvin to Celsius
  float temperatureC = temperatureK - 273.15;

  // Apply built-in compensation for known systematic errors
  // Based on your observations: -22°C reads as -12°C (error: +10°C), 25°C reads as 34°C (error: +9°C)
  // This suggests a non-linear error that decreases with temperature
  // But we need to be more conservative with the compensation
  float compensation = 0.0;
  if (temperatureC > -50.0 && temperatureC < 50.0) {
    // Use a more conservative compensation factor (25% of the observed error)
    // At -22°C: error = +10°C, At 25°C: error = +9°C
    float errorAtLow = 2.5;   // 25% of 10°C error at -22°C
    float errorAtHigh = 2.25; // 25% of 9°C error at 25°C
    float tempLow = -22.0;
    float tempHigh = 25.0;
    
    if (temperatureC <= tempLow) {
      compensation = errorAtLow;
    } else if (temperatureC >= tempHigh) {
      compensation = errorAtHigh;
    } else {
      // Linear interpolation
      float slope = (errorAtHigh - errorAtLow) / (tempHigh - tempLow);
      compensation = errorAtLow + slope * (temperatureC - tempLow);
    }
  }

  // Apply compensation (subtract the error)
  temperatureC -= compensation;

  // Ensure temperature is within a reasonable range
  if (temperatureC < -40.0 || temperatureC > 125.0) {
    Serial.println("Temperature out of range! Returning default value.");
    return 0.0;  // Return default value if out of range
  }

  return temperatureC;
}

// Old file size limit - now handled by rolling buffer
// const size_t MAX_FILE_SIZE = 2 * 1024 * 1024;  // 2MB

bool writeToFile(float temperature, unsigned long timestamp, const String& ackStatus) {
  // Prepare the data to write
  String data = String(temperature, 2) + "," + String(timestamp) + "," + ackStatus + "\n";
  
  // Check if we need to implement wrap-around (file is at max capacity)
  if (bufferState.totalEntries >= MAX_READINGS) {
    // Implement wrap-around by truncating the file and starting over
    // This is simpler than trying to overwrite specific positions
    File dataFile = LittleFS.open(DATA_FILE, "w");
    if (!dataFile) {
      Serial.println("Failed to create data file for wrap-around");
      return false;
    }
    dataFile.print(data);
    dataFile.close();
    
    // Reset buffer state for wrap-around
    bufferState.totalEntries = 1;
    bufferState.writePosition = data.length();
    bufferState.isWrapped = true;
    
    saveBufferState();
    return true;
  }
  
  // Normal append mode
  File dataFile = LittleFS.open(DATA_FILE, "a");
  if (!dataFile) {
    // If file doesn't exist, create it
    dataFile = LittleFS.open(DATA_FILE, "w");
    if (!dataFile) {
      Serial.println("Failed to open/create data file for writing");
      return false;
    }
  }
  
  // Write data to the file
  size_t bytesWritten = dataFile.print(data);
  dataFile.close();
  
  if (bytesWritten != data.length()) {
    Serial.println("Failed to write all data to the file.");
    return false;
  }
  
  // Update buffer state
  bufferState.totalEntries++;
  bufferState.writePosition += data.length();
  
  // Save buffer state
  saveBufferState();
  
  // Debug output - less frequent for better performance
  if (bufferState.totalEntries % 1000 == 0) {
    Serial.printf("[Buffer] Total entries: %d, Write pos: %d, Wrapped: %s\n", 
                  bufferState.totalEntries, bufferState.writePosition, 
                  bufferState.isWrapped ? "YES" : "NO");
  }
  
  return true;
}

void initializeWritePosition() {
  // This function is now handled by the rolling buffer system
  // The rolling buffer automatically manages write positions
  Serial.println("[Buffer] Write position initialization handled by rolling buffer");
}

void readFromFile() {
    File dataFile = LittleFS.open(DATA_FILE, "r");
    if (!dataFile) {
        Serial.println("Failed to open data file for reading");
        return;
    }

    Serial.println("Reading data from file (rolling buffer):");
    Serial.printf("Total entries: %d, Wrapped: %s\n", 
                  bufferState.totalEntries, bufferState.isWrapped ? "YES" : "NO");
    
    int lineCount = 0;
    while (dataFile.available() && lineCount < 50) { // Limit output to 50 lines
        String data = dataFile.readStringUntil('\n');
        if (data.length() == 0) continue;
        
        int firstCommaIndex = data.indexOf(',');
        int secondCommaIndex = data.indexOf(',', firstCommaIndex + 1);

        if (firstCommaIndex != -1 && secondCommaIndex != -1) {
            float temperature = data.substring(0, firstCommaIndex).toFloat();
            unsigned long timestamp = data.substring(firstCommaIndex + 1, secondCommaIndex).toInt();
            String ackStatus = data.substring(secondCommaIndex + 1);
            ackStatus.trim();

            DateTime dt = DateTime(timestamp);
            Serial.print("Line "); Serial.print(lineCount + 1); Serial.print(": ");
            Serial.print("Temperature: ");
            Serial.print(temperature, 2);
            Serial.print(" °C, Timestamp: ");
            printDateTime(dt);
            Serial.print(", ACK Status: ");
            Serial.println(ackStatus);
        } else {
            Serial.println("Invalid data format");
        }
        lineCount++;
    }
    
    if (lineCount >= 50) {
        Serial.println("... (showing first 50 lines only)");
    }
    
    dataFile.close();
}

void clearFile() {
  LittleFS.remove(DATA_FILE);
  File dataFile = LittleFS.open(DATA_FILE, "w");
  if (!dataFile) {
    Serial.println("Failed to create new data file");
  } else {
    dataFile.close();
  }
}

String sanitizeData(const String& data) {
  String sanitizedData;
  for (char c : data) {
    if (!isspace(c)) {
      sanitizedData += c;
    }
  }
  return sanitizedData;
}

const int MAX_PACKET_SIZE = 200; // LoRa payload limit (adjust as needed)
const int MAX_ACK_WAIT_MS = 1000; // How long to wait for ACK

void resendLoggedData() {
    File dataFile = LittleFS.open(DATA_FILE, "r");
    if (!dataFile) {
        Serial.println("Failed to open data file for reading");
        return;
    }

    std::vector<float> temps;
    std::vector<unsigned long> times;
    std::vector<size_t> lineNumbers; // To remember which lines are in this batch
    const int MAX_LORA_PAYLOAD = 200; // bytes
    const int MAX_ENTRIES = 7; // safety cap, fits LoRa payload and RAM
    int batchCount = 0;
    size_t lineIndex = 0;

    // Only collect enough for one batch
    while (dataFile.available() && batchCount < MAX_ENTRIES) {
        String line = dataFile.readStringUntil('\n');
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 == -1 || c2 == -1) { lineIndex++; continue; }
        String ack = line.substring(c2 + 1); ack.trim();
        if (ack == "n") {
            float t = line.substring(0, c1).toFloat();
            unsigned long ts = line.substring(c1 + 1, c2).toInt();
            temps.push_back(t);
            times.push_back(ts);
            lineNumbers.push_back(lineIndex);
            batchCount++;
        }
        lineIndex++;
    }
    dataFile.close();

    if (temps.empty()) {
        Serial.println("No unacknowledged data to backfill.");
        return;
    }

    // Now, trim batch to fit LoRa payload
    int entriesToSend = temps.size();
    String payload;
    while (entriesToSend > 0) {
        // Build compact JSON
        DynamicJsonDocument doc(256 + 32 * entriesToSend);
        doc["s"] = serialNumber;
        JsonArray entries = doc.createNestedArray("e");
        for (int i = 0; i < entriesToSend; ++i) {
            JsonArray entry = entries.createNestedArray();
            entry.add(temps[i]);
            entry.add(times[i]);
        }
        String json;
        serializeJson(doc, json);

        // Base64 encode
        char encoded[512];
        unsigned char jsonInput[512];
        strncpy((char*)jsonInput, json.c_str(), sizeof(jsonInput) - 1);
        jsonInput[sizeof(jsonInput) - 1] = '\0';
        encode_base64(jsonInput, json.length(), (unsigned char*)encoded);
        int encodedLen = strlen(encoded);
        Serial.print("JSON length: "); Serial.println(json.length());
        Serial.print("Base64 length: "); Serial.println(encodedLen);
        if (encodedLen <= MAX_LORA_PAYLOAD) {
            payload = String(encoded);
            break;
        }
        entriesToSend--;
    }

    if (entriesToSend == 0) {
        Serial.println("Single entry too large to send via LoRa!");
        return;
    }

    // Send via LoRa
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();
    Serial.print("Backfill batch sent (entries: ");
    Serial.print(entriesToSend);
    Serial.println(")");

    // Wait for ACK
    unsigned long start = millis();
    String ackPayload;
    bool gotAck = false;
    while (millis() - start < MAX_ACK_WAIT_MS) {
        int packetSize = LoRa.parsePacket();
        if (packetSize > 0) {
            while (LoRa.available()) {
                ackPayload += (char)LoRa.read();
            }
            gotAck = true;
            break;
        }
        delay(10);
    }

    if (!gotAck) {
        Serial.println("No ACK received for backfill batch.");
        return;
    }

    // Decode base64 ACK
    char decoded[512];
    unsigned char ackInput[512];
    strncpy((char*)ackInput, ackPayload.c_str(), sizeof(ackInput) - 1);
    ackInput[sizeof(ackInput) - 1] = '\0';
    int decodedLen = decode_base64(ackInput, (unsigned char*)decoded);
    decoded[decodedLen] = '\0';
    String ackJson = String(decoded);

    // Parse ACK and update file
    // --- Efficient file update: rewrite file, update only relevant lines ---
    DynamicJsonDocument ackDoc(256);
    DeserializationError err = deserializeJson(ackDoc, ackJson);
    if (err) {
        Serial.println("Failed to parse ACK JSON.");
        return;
    }
    if (!ackDoc.containsKey("a")) return;
    JsonArray acked = ackDoc["a"].as<JsonArray>();

    // Open original file for reading and a temp file for writing
    File inFile = LittleFS.open(DATA_FILE, "r");
    File outFile = LittleFS.open("/data_tmp.txt", "w");
    if (!inFile || !outFile) {
        Serial.println("Failed to open files for updating ACKs.");
        if (inFile) inFile.close();
        if (outFile) outFile.close();
        return;
    }
    size_t curLine = 0;
    while (inFile.available()) {
        String line = inFile.readStringUntil('\n');
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 == -1 || c2 == -1) {
            outFile.println(line);
            curLine++;
            continue;
        }
        float t = line.substring(0, c1).toFloat();
        unsigned long ts = line.substring(c1 + 1, c2).toInt();
        String ack = line.substring(c2 + 1); ack.trim();
        bool shouldAck = false;
        // Only update if this line is in our batch and timestamp is in ACK
        for (size_t i = 0; i < entriesToSend; ++i) {
            if (curLine == lineNumbers[i]) {
                for (JsonVariant v : acked) {
                    if (ts == v.as<unsigned long>()) {
                        shouldAck = true;
                        break;
                    }
                }
                break;
            }
        }
        if (shouldAck && ack == "n") {
            outFile.printf("%.2f,%lu,y\n", t, ts);
        } else {
            outFile.println(line);
        }
        curLine++;
    }
    inFile.close();
    outFile.close();
    LittleFS.remove(DATA_FILE);
    LittleFS.rename("/data_tmp.txt", DATA_FILE);
}

void handleBackfillAck(const String& ackJson) {
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, ackJson);
    if (err) {
        Serial.println("Failed to parse ACK JSON.");
        return;
    }
    if (!doc.containsKey("a")) return;
    JsonArray acked = doc["a"].as<JsonArray>();

    // Read all lines
    File dataFile = LittleFS.open(DATA_FILE, "r");
    if (!dataFile) return;
    std::vector<String> lines;
    while (dataFile.available()) lines.push_back(dataFile.readStringUntil('\n'));
    dataFile.close();

    // Write back, updating ACKs
    File outFile = LittleFS.open(DATA_FILE, "w");
    for (String& line : lines) {
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 == -1 || c2 == -1) {
            outFile.println(line);
            continue;
        }
        unsigned long ts = line.substring(c1 + 1, c2).toInt();
        String ack = line.substring(c2 + 1); ack.trim();
        bool found = false;
        for (JsonVariant v : acked) {
            if (ts == v.as<unsigned long>()) {
                found = true;
                break;
            }
        }
        if (found && ack == "n") {
            // Mark as acknowledged
            outFile.printf("%s,%lu,y\n", line.substring(0, c1).c_str(), ts);
        } else {
            outFile.println(line);
        }
    }
    outFile.close();
    Serial.println("Backfill ACK processed.");
}

String formatTimestamp(const DateTime& timestamp) {
  String timestampString = String(timestamp.year()) + "/" + 
                           (timestamp.month() < 10 ? "0" : "") + String(timestamp.month()) + "/" + 
                           (timestamp.day() < 10 ? "0" : "") + String(timestamp.day()) + " " + 
                           (timestamp.hour() < 10 ? "0" : "") + String(timestamp.hour()) + ":" + 
                           (timestamp.minute() < 10 ? "0" : "") + String(timestamp.minute()) + ":" + 
                           (timestamp.second() < 10 ? "0" : "") + String(timestamp.second());
  return timestampString;
}


bool validateData(const String& data) {
  for (char c : data) {
    if (!isPrintable(c)) {
      Serial.println("Data contains non-printable characters. Skipping entry.");
      return false;
    }
  }
  return true;
}

bool waitForACK() {
  unsigned long ackWaitStartTime = millis();
  while (millis() - ackWaitStartTime < 300) {
    yield();  // Prevent WDT reset

    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String loRaResponse = "";
      while (LoRa.available()) {
        loRaResponse += (char)LoRa.read();
      }
      Serial.println("LoRa Response: " + loRaResponse);

      String trimmedLoRaResponse = cleanIncomingData(loRaResponse);

      if (isValidMessageFormat(trimmedLoRaResponse) && trimmedLoRaResponse.endsWith("ACK")) {
        return true;
      }
    }
  }
  return false;
}

void updateACKStatusInFile(File& dataFile, float temperature, unsigned long timestampData) {
  String newData = String(temperature, 2) + "," + String(timestampData) + ",y\n";
  File newDataFile = LittleFS.open(DATA_FILE, "r+");
  if (newDataFile) {
    newDataFile.seek(dataFile.position() - newData.length() - 1, SeekSet);
    newDataFile.print(newData);
    newDataFile.flush();
    newDataFile.close();
  } else {
    Serial.println("Failed to open data file for updating ACK status.");
  }
}

String cleanIncomingData(const String& rawData) {
  int startIndex = 0;
  int endIndex = rawData.length() - 1;

  while (startIndex < rawData.length() && isspace(rawData[startIndex])) {
    startIndex++;
  }

  while (endIndex >= 0 && isspace(rawData[endIndex])) {
    endIndex--;
  }

  String cleanedData = rawData.substring(startIndex, endIndex + 1);
  return cleanedData;
}


void sendTimeSyncCommand() {
  DateTime now = rtc->now();  // Get current time from RTC
  Serial.println(formatTimestamp(now));
  if (now.year() < 2000) {  // Basic validity check to ensure we have a valid time
    Serial.println("RTC time is invalid, skipping time sync.");
    return;  // Skip sending time sync if the time is invalid
  }

  String timeSyncMessage = String(serialNumber) + "," + formatTimestamp(now);  // Prepare time sync message
  Serial.println("Sending Time Sync Command: " + timeSyncMessage);
  
  LoRa.beginPacket();
  LoRa.print(timeSyncMessage);
  LoRa.endPacket();
  Serial.println("Time sync command sent.");
}
// Read the last time sync from LittleFS
unsigned long readLastTimeSync() {
  File file = LittleFS.open(LAST_TIME_SYNC_FILE, "r");
  if (!file) {
    Serial.println("No last time sync file found.");
    return 0;  // No sync has been performed before
  }

  String lastTime = file.readStringUntil('\n');
  file.close();

  unsigned long lastSyncTime = lastTime.toInt();  // Convert the string to an unsigned long
  if (lastSyncTime == 0) {
    Serial.println("Invalid last time sync data.");
  }

  return lastSyncTime;  // Return the Unix timestamp
}

// Write the last time sync to LittleFS
void writeLastTimeSync(unsigned long currentTime) {
    // Ensure LittleFS is mounted
   

    File file = LittleFS.open(LAST_TIME_SYNC_FILE, "w");
    if (!file) {
        Serial.println("Failed to write last time sync file.");
        return;
    }

    file.println(String(currentTime));  // Save the current time as Unix timestamp
    file.close();
    Serial.println("Last time sync written to LittleFS: " + String(currentTime));
      // Unmount LittleFS after operation
}

// Process the response from the base unit and update RTC
void processTimeSyncResponse(const String& message) {
    Serial.println("Processing time sync response: " + message);

    if (message.startsWith(String(serialNumber) + ",")) {
        Serial.println("Serial number matches.");

        int firstCommaIndex = message.indexOf(",");
        int secondCommaIndex = message.indexOf(",", firstCommaIndex + 1);

        if (firstCommaIndex != -1 && secondCommaIndex != -1) {
            String datetimeStr = message.substring(firstCommaIndex + 1, secondCommaIndex);  // Extract datetime string
            String updateFlag = message.substring(secondCommaIndex + 1);                    // Extract the "UPDATE" flag

            Serial.println("Extracted datetime: " + datetimeStr);
            Serial.println("Extracted flag: " + updateFlag);

            if (updateFlag == "UPDATE") {
                Serial.println("UPDATE flag detected. Processing time update...");

                // Parse the datetime string and set the RTC in the same way as setTimeFromSerial
                DateTime newDateTime = parseDateTime(datetimeStr);

                if (newDateTime.year() > 2000) {  // Basic validity check
                    Serial.println("Valid datetime received. Adjusting RTC...");
                    rtc->adjust(newDateTime);  // Adjust RTC to the parsed datetime
                    Serial.println("RTC updated with new datetime: ");
                    printDateTime(newDateTime);  // Print the updated datetime

                    // Save the valid datetime to LittleFS once
                    unsigned long currentUnixTime = newDateTime.unixtime();
                    saveTimeToLittleFS(newDateTime);   // Save valid time
                    writeLastTimeSync(currentUnixTime);  // Save last time sync only once
                    Serial.println("Saved valid datetime to LittleFS: " + String(currentUnixTime));
                } else {
                    Serial.println("Invalid datetime received.");
                }
            } else {
                Serial.println("No UPDATE flag detected.");
            }
        } else {
            Serial.println("Invalid message format.");
        }
    } else {
        Serial.println("Serial number mismatch.");
    }
}

// Parse received datetime string
DateTime parseDateTime(const String& datetimeStr) {
  int year, month, day, hour, minute, second;

  // Parse the datetime string in the format "YYYY/MM/DD HH:MM:SS"
  int parsedItems = sscanf(datetimeStr.c_str(), "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

  if (parsedItems == 6) {
    Serial.println("Parsed datetime successfully.");
    return DateTime(year, month, day, hour, minute, second);
  } else {
    Serial.println("Failed to parse datetime. Using default value.");
    return DateTime();  // Return an invalid DateTime object if parsing fails
  }
}



bool isBST(const DateTime& dt) {
  // BST runs from the last Sunday of March to the last Sunday of October.
  int year = dt.year();

  // Get the last Sunday of March
  DateTime lastSundayOfMarch = DateTime(year, 3, (31 - (5 * year / 4 + 4) % 7));
  // Get the last Sunday of October
  DateTime lastSundayOfOctober = DateTime(year, 10, (31 - (5 * year / 4 + 1) % 7));

  return (dt >= lastSundayOfMarch && dt < lastSundayOfOctober);
}

// Function to read battery voltage from INA226
float readBatteryVoltageINA() {
  return ina226->getBusVoltage_mV() / 1000.0;  // Convert mV to V
}

void saveTimeToLittleFS(DateTime now) {
    // Ensure LittleFS is mounted
   

    File file = LittleFS.open("/lastValidTime.txt", "w");
    if (!file) {
        Serial.println("Failed to open file to save time.");
        return;
    }
    unsigned long currentTime = now.unixtime();
    file.println(String(currentTime));  // Save current time as Unix timestamp
    file.close();
    Serial.println("Current time saved to LittleFS: " + String(currentTime));
     // Unmount LittleFS after operation
}

unsigned long restoreTimeFromLittleFS() {
  File file = LittleFS.open("/lastValidTime.txt", "r");
  if (!file) {
    Serial.println("No saved time found in LittleFS.");
    return 0;  // No valid saved time
  }

  String savedTimeStr = file.readStringUntil('\n');
  file.close();
  unsigned long savedTime = savedTimeStr.toInt();  // Convert saved string to Unix time

  if (savedTime > 0 && savedTime < 2147483647) {  // Validate the range of Unix timestamp
    Serial.println("Restored time from LittleFS: " + String(savedTime));
  } else {
    Serial.println("Invalid time found in LittleFS.");
    savedTime = 0;
  }

  return savedTime;
}


void checkForTemperatureData() {
    // Ensure LittleFS is mounted
    

    // Open the file in read mode
    File dataFile = LittleFS.open(DATA_FILE, "r");
    if (!dataFile) {
        Serial.println("Temperature data file not found.");
        return;
    }

    // Check if the file contains any valid data
    if (dataFile.size() > 0) {
        Serial.println("Temperature data exists in LittleFS:");
        
        // Read the first line as a quick check
        String firstEntry = dataFile.readStringUntil('\n');
        Serial.println("First entry: " + firstEntry);

        // Optionally, parse and validate the data
        int firstCommaIndex = firstEntry.indexOf(',');
        int secondCommaIndex = firstEntry.indexOf(',', firstCommaIndex + 1);

        if (firstCommaIndex != -1 && secondCommaIndex != -1) {
            float temperature = firstEntry.substring(0, firstCommaIndex).toFloat();
            unsigned long timestamp = firstEntry.substring(firstCommaIndex + 1, secondCommaIndex).toInt();
            String ackStatus = firstEntry.substring(secondCommaIndex + 1);
            ackStatus.trim();

            Serial.print("Parsed Temperature: ");
            Serial.println(temperature);
            Serial.print("Parsed Timestamp: ");
            Serial.println(timestamp);
            Serial.print("ACK Status: ");
            Serial.println(ackStatus);
        } else {
            Serial.println("Invalid format in temperature data file.");
        }
    } else {
        Serial.println("No temperature data found in LittleFS.");
    }

    dataFile.close();
}

// Add persistent backfillId
unsigned long backfillId = 1;

// Windowed backfill: send 10 packets of 7 entries, wait for single ACK with backfillId
void windowedBackfillAllData() {
    const int PACKETS_PER_WINDOW = 10;
    const int ENTRIES_PER_PACKET = 7;
    const int ENTRIES_PER_WINDOW = PACKETS_PER_WINDOW * ENTRIES_PER_PACKET;
    const int MAX_RETRIES = 3;
    const int MAX_CONSECUTIVE_FAILED_WINDOWS = 3;
    static int consecutiveFailedWindows = 0;
    unsigned long totalStart = millis();
    int totalEntriesProcessed = 0;
    int windowCount = 0;
    while (true) {
        windowCount++;
        Serial.print("\n[Backfill] Starting window #"); Serial.println(windowCount);
        unsigned long openStart = millis();
        // 1. Collect up to ENTRIES_PER_WINDOW unacknowledged entries
        std::vector<float> temps;
        std::vector<unsigned long> times;
        File dataFile = LittleFS.open(DATA_FILE, "r");
        unsigned long openEnd = millis();
        Serial.print("[Backfill] File open time: "); Serial.print(openEnd - openStart); Serial.println(" ms");
        if (!dataFile) {
            Serial.println("[Backfill] Failed to open data file for reading");
            return;
        }
        int batchCount = 0;
        while (dataFile.available() && batchCount < ENTRIES_PER_WINDOW) {
            String line = dataFile.readStringUntil('\n');
            int c1 = line.indexOf(',');
            int c2 = line.indexOf(',', c1 + 1);
            if (c1 == -1 || c2 == -1) continue;
            String ack = line.substring(c2 + 1); ack.trim();
            if (ack == "n") {
                float t = line.substring(0, c1).toFloat();
                unsigned long ts = line.substring(c1 + 1, c2).toInt();
                temps.push_back(t);
                times.push_back(ts);
                batchCount++;
            }
        }
        dataFile.close();
        Serial.print("[Backfill] Entries in this window: "); Serial.println(batchCount);
        if (temps.empty()) {
            Serial.println("[Backfill] No unacknowledged data to backfill.");
            unsigned long totalEnd = millis();
            Serial.print("[Backfill] Total backfill time: "); Serial.print(totalEnd - totalStart); Serial.println(" ms");
            Serial.print("[Backfill] Total entries processed: "); Serial.println(totalEntriesProcessed);
            return;
        }
        int entriesToSend = temps.size();
        if (entriesToSend > ENTRIES_PER_WINDOW) entriesToSend = ENTRIES_PER_WINDOW;
        // 2. Send up to PACKETS_PER_WINDOW packets
        int sent = 0;
        for (int pkt = 0; pkt < PACKETS_PER_WINDOW && sent < entriesToSend; ++pkt) {
            int thisPacket = min(ENTRIES_PER_PACKET, entriesToSend - sent);
            DynamicJsonDocument doc(256 + 32 * thisPacket);
            doc["s"] = serialNumber;
            doc["b"] = backfillId;
            JsonArray entries = doc.createNestedArray("e");
            for (int i = 0; i < thisPacket; ++i) {
                JsonArray entry = entries.createNestedArray();
                entry.add(temps[sent + i]);
                entry.add(times[sent + i]);
            }
            String json;
            serializeJson(doc, json);
            char encoded[512];
            unsigned char jsonInput[512];
            strncpy((char*)jsonInput, json.c_str(), sizeof(jsonInput) - 1);
            jsonInput[sizeof(jsonInput) - 1] = '\0';
            encode_base64(jsonInput, json.length(), (unsigned char*)encoded);
            int encodedLen = strlen(encoded);
            Serial.print("[Backfill] Sending window packet "); Serial.print(pkt+1); Serial.print("/"); Serial.print(PACKETS_PER_WINDOW);
            Serial.print(" (Base64 length: "); Serial.print(encodedLen); Serial.println(")");
            LoRa.beginPacket();
            LoRa.print(encoded);
            LoRa.endPacket();
            delay(200); // 500ms between packets
            sent += thisPacket;
        }
        // 3. Wait for ACK with backfillId
        int retries = 0;
        bool acked = false;
        while (retries < MAX_RETRIES && !acked) {
            Serial.print("[Backfill] Waiting for window ACK (id: "); Serial.print(backfillId); Serial.println(")...");
            unsigned long ackWaitStart = millis();
            String ackPayload;
            bool gotAck = false;
            const int MAX_ACK_WAIT_MS = 2000;
            while (millis() - ackWaitStart < MAX_ACK_WAIT_MS) {
                int packetSize = LoRa.parsePacket();
                if (packetSize > 0) {
                    while (LoRa.available()) {
                        ackPayload += (char)LoRa.read();
                    }
                    gotAck = true;
                    break;
                }
                delay(10);
            }
            unsigned long ackWaitEnd = millis();
            Serial.print("[Backfill] ACK wait time: "); Serial.print(ackWaitEnd - ackWaitStart); Serial.println(" ms");
            if (!gotAck) {
                Serial.println("[Backfill] No ACK received for window.");
                retries++;
                continue;
            }
            // Decode base64 ACK
            char decoded[256];
            unsigned char ackInput[256];
            strncpy((char*)ackInput, ackPayload.c_str(), sizeof(ackInput) - 1);
            ackInput[sizeof(ackInput) - 1] = '\0';
            int decodedLen = decode_base64(ackInput, (unsigned char*)decoded);
            decoded[decodedLen] = '\0';
            String ackJson = String(decoded);
            DynamicJsonDocument ackDoc(128);
            DeserializationError err = deserializeJson(ackDoc, ackJson);
            if (err) {
                Serial.println("[Backfill] Failed to parse window ACK JSON.");
                retries++;
                continue;
            }
            if (!ackDoc.containsKey("b")) {
                Serial.println("[Backfill] Window ACK missing 'b' (backfillId) key.");
                retries++;
                continue;
            }
            unsigned long ackId = ackDoc["b"].as<unsigned long>();
            if (ackId != backfillId) {
                Serial.print("[Backfill] Window ACK id mismatch (expected "); Serial.print(backfillId); Serial.print(", got "); Serial.print(ackId); Serial.println(")");
                retries++;
                continue;
            }
            Serial.print("[Backfill] Window ACK received for id: "); Serial.println(backfillId);
            acked = true;
        }
        if (!acked) {
            consecutiveFailedWindows++;
            Serial.print("[Backfill] Consecutive failed windows: ");
            Serial.println(consecutiveFailedWindows);
            if (consecutiveFailedWindows >= MAX_CONSECUTIVE_FAILED_WINDOWS) {
                Serial.println("[Backfill] Too many failed windows. Pausing backfill until next ACK.");
                break; // Exit the backfill loop
            }
        } else {
            consecutiveFailedWindows = 0; // Reset on success
        }
        if (!acked) {
            Serial.print("[Backfill] Window failed after "); Serial.print(MAX_RETRIES); Serial.println(" retries, skipping to next window.");
            // Optionally, break or continue; here we continue to next window
        } else {
            // Remove the first ENTRIES_PER_WINDOW unacknowledged entries from the file
            unsigned long fileUpdateStart = millis();
            File inFile = LittleFS.open(DATA_FILE, "r");
            File outFile = LittleFS.open("/data_tmp.txt", "w");
            if (!inFile || !outFile) {
                Serial.println("[Backfill] Failed to open files for window file update.");
                if (inFile) inFile.close();
                if (outFile) outFile.close();
                return;
            }
            int unackedSeen = 0;
            while (inFile.available()) {
                String line = inFile.readStringUntil('\n');
                int c1 = line.indexOf(',');
                int c2 = line.indexOf(',', c1 + 1);
                if (c1 == -1 || c2 == -1) {
                    outFile.println(line);
                    continue;
                }
                String ack = line.substring(c2 + 1); ack.trim();
                if (ack == "n" && unackedSeen < ENTRIES_PER_WINDOW) {
                    unackedSeen++;
                    continue; // skip (delete)
                }
                outFile.println(line);
            }
            inFile.close();
            outFile.close();
            unsigned long fileDeleteStart = millis();
            LittleFS.remove(DATA_FILE);
            unsigned long fileDeleteEnd = millis();
            Serial.print("[Backfill] File delete time: "); Serial.print(fileDeleteEnd - fileDeleteStart); Serial.println(" ms");
            unsigned long fileRenameStart = millis();
            LittleFS.rename("/data_tmp.txt", DATA_FILE);
            unsigned long fileRenameEnd = millis();
            Serial.print("[Backfill] File rename (write) time: "); Serial.print(fileRenameEnd - fileRenameStart); Serial.println(" ms");
            unsigned long fileUpdateEnd = millis();
            Serial.print("[Backfill] File update (open+write+delete+rename) time: "); Serial.print(fileUpdateEnd - fileUpdateStart); Serial.println(" ms");
            Serial.print("[Backfill] Deleted "); Serial.print(unackedSeen); Serial.println(" entries after window ACK.");
            totalEntriesProcessed += unackedSeen;
        }
        backfillId++;
        // Check if any unacknowledged data remains
        File checkFile = LittleFS.open(DATA_FILE, "r");
        bool hasUnacked = false;
        while (checkFile.available()) {
            String line = checkFile.readStringUntil('\n');
            int c1 = line.indexOf(',');
            int c2 = line.indexOf(',', c1 + 1);
            if (c1 == -1 || c2 == -1) continue;
            String ack = line.substring(c2 + 1); ack.trim();
            if (ack == "n") { hasUnacked = true; break; }
        }
        checkFile.close();
        Serial.print("[Backfill] Window #"); Serial.print(windowCount); Serial.print(" complete. Remaining unacknowledged: "); Serial.println(hasUnacked ? "YES" : "NO");
        if (!hasUnacked) break;
    }
    unsigned long totalEnd = millis();
    Serial.print("[Backfill] Total backfill time: "); Serial.print(totalEnd - totalStart); Serial.println(" ms");
    Serial.print("[Backfill] Total entries processed: "); Serial.println(totalEntriesProcessed);
}

void checkForBackfill() {
    File dataFile = LittleFS.open(DATA_FILE, "r");
    if (!dataFile) {
        Serial.println("Failed to open data file for reading");
        return;
    }

    bool hasUnacknowledgedData = false;

    while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        line.trim(); // Remove whitespace or newline characters

        // Parse the line to check for unacknowledged data
        int firstCommaIndex = line.indexOf(',');
        int secondCommaIndex = line.indexOf(',', firstCommaIndex + 1);

        if (firstCommaIndex != -1 && secondCommaIndex != -1) {
            String ackStatus = line.substring(secondCommaIndex + 1); // Extract the ACK status
            ackStatus.trim(); // Trim the ACK status separately
            if (ackStatus == "n") {
                hasUnacknowledgedData = true;
                break; // No need to check further if unacknowledged data is found
            }
        }
    }

    dataFile.close();

    if (hasUnacknowledgedData) {
        Serial.println("Pending unacknowledged backfill data found. Preparing to resend...");
        windowedBackfillAllData();
    } else {
       // Serial.println("No unacknowledged backfill data found.");
    }
}

// Helper function to extract values from messages
String extractValue(const String &data, const String &key) {
  int startIndex = data.indexOf(key + ":");
  if (startIndex == -1) return "";
  startIndex += key.length() + 1;
  int endIndex = data.indexOf(",", startIndex);
  if (endIndex == -1) endIndex = data.length();
  return data.substring(startIndex, endIndex);
}

// Helper function to connect to Wi-Fi
bool connectToWiFi(const String &ssid, const String &password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to Wi-Fi...");
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis() - startTime > 30000) { // Timeout after 30 seconds
            Serial.println("\nFailed to connect to Wi-Fi.");
            return false;
        }
    }
    Serial.println("\nConnected to Wi-Fi.");
    return true;
}


bool isNewFirmwareAvailable() {
    // Clean up sensor data in LittleFS
    Serial.println("Deleting all sensor data files from LittleFS...");
   
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        if (dir.fileName().startsWith("/sensor")) { // Delete files starting with 'sensor'
            if (LittleFS.remove(dir.fileName())) {
                Serial.println("Deleted file: " + dir.fileName());
            } else {
                Serial.println("Failed to delete file: " + dir.fileName());
            }
        }
    }
     // Unmount LittleFS

    // Log heap status before HTTP request
    Serial.printf("Heap Fragmentation: %d%%, Free Heap: %u bytes\n", ESP.getHeapFragmentation(), ESP.getFreeHeap());

    // Initialize retry parameters
    const int maxRetries = 3;
    const int retryDelay = 2000;
    int retryCount = 0;

    while (retryCount < maxRetries) {
        HTTPClient http;

        // Ensure WiFi is still connected before making a request
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected. Aborting firmware check.");
            return false;
        }

        Serial.println("Attempting to fetch firmware version...");
        String url = "https://raw.githubusercontent.com/thermalsystemsltd/sensorfirmware/refs/heads/main/version";
        url += "?nocache=" + String(millis()); // Avoid cached responses

        if (!http.begin(*espClientSecure, url)) {
            Serial.println("Failed to initialize HTTP client.");
            http.end(); // Clean up
            retryCount++;
            delay(retryDelay);
            continue;
        }

        int httpCode = http.GET();
        if (httpCode > 0) {
            Serial.printf("HTTP GET Request sent. Response code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK) {
                String newVersion = http.getString();
                newVersion.trim(); // Remove extra whitespace or newlines
                Serial.println("Raw HTTP Response: " + newVersion);

                Serial.println("Available Firmware Version: " + newVersion);
                Serial.println("Current Firmware Version: " + String(FIRMWARE_VERSION));

                http.end(); // Clean up resources

                if (newVersion != String(FIRMWARE_VERSION)) {
                    Serial.println("New firmware version detected. Update required.");
                    return true;
                } else {
                    Serial.println("Firmware is up to date. No update required.");
                    return false;
                }
            } else {
                Serial.printf("HTTP response not OK. Code: %d\n", httpCode);
            }
        } else {
            Serial.printf("HTTP GET request failed. Error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end(); // Clean up resources
        retryCount++;
        delay(retryDelay);
    }

    Serial.println("Failed to fetch firmware version after maximum retries.");
    return false;
}






// Helper function to perform OTA update
bool performOTAUpdate(const String& firmwareURL, const String& firmwareBinURL) {
    Serial.println("Starting OTA update...");
    
    // Save the current serial number to LittleFS
    saveSerialNumberToLittleFS(serialNumber);
    
    Serial.printf("Fetching firmware from: %s\n", firmwareBinURL.c_str());

    ESPhttpUpdate.rebootOnUpdate(false); // Manually handle reboot

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); // Indicate update progress using LED

    // Monitor progress
    ESPhttpUpdate.onProgress([](int current, int total) {
        Serial.printf("OTA Progress: %d/%d bytes (%d%%)\r", current, total, (current * 100) / total);
    });

    t_httpUpdate_return ret = ESPhttpUpdate.update(*espClientSecure, firmwareBinURL);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("OTA Update Failed. Error: (%d) %s\n",
                          ESPhttpUpdate.getLastError(),
                          ESPhttpUpdate.getLastErrorString().c_str());
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No OTA updates available.");
            return false;
        case HTTP_UPDATE_OK:
            Serial.println("\nOTA Update Successful. Ready to reboot.");
            return true;
    }
    return false; // Should not be reached
}



/*
void checkForFirmwareUpdate() {
    if (LoRa.parsePacket()) {
        String message = "";
        while (LoRa.available()) {
            message += (char)LoRa.read();
        }
        Serial.println("LoRa Message Received: " + message);

        if (message.startsWith("UPDATE:START")) {
            String sensorID = extractValue(message, "SENSOR");
            String ssid = extractValue(message, "SSID");
            String password = extractValue(message, "PASS");
            String firmwareBinURL = extractValue(message, "URL");

            if (sensorID == serialNumber) {
                Serial.println("Firmware update command received for this sensor.");

                if (connectToWiFi(ssid, password)) {
                    if (!firmwareBinURL.isEmpty()) {
                        performOTAUpdate(sensor_firmware_version_url, firmwareBinURL);
                    } else {
                        Serial.println("Invalid firmware URL. Update aborted.");
                    }
                    WiFi.disconnect();
                } else {
                    Serial.println("Failed to connect to Wi-Fi for OTA.");
                }
            } else {
                Serial.println("Firmware update ignored: Sensor ID mismatch.");
            }
        }
    }
}
*/


void testBinaryUrl() {
    HTTPClient http;

    Serial.printf("Testing connection to %s\n", firmwareBinURL);
    if (!http.begin(*espClientSecure, firmwareBinURL)) {
        Serial.println("Failed to initialize HTTP client.");
        return;
    }

    int httpCode = http.GET();
    if (httpCode > 0) {
        Serial.printf("HTTP GET request sent. Response code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
            Serial.println("Binary URL is accessible and ready for download.");
            String payload = http.getString();
            Serial.println("Payload:");
            Serial.println(payload); // Print the response payload
        }
    } else {
        Serial.printf("HTTP GET request failed. Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}
////////////////////////
// Function to decode Base64-encoded SSID and password
bool decodeUpdate(const String& encodedSSID, const String& encodedPassword, String& decodedSSID, String& decodedPassword) {
    // Buffers to hold the input data for decoding
    unsigned char ssidInput[128] = {0};
    unsigned char passwordInput[128] = {0};

    // Copy encoded data into non-const buffers
    strncpy((char*)ssidInput, encodedSSID.c_str(), sizeof(ssidInput) - 1);
    strncpy((char*)passwordInput, encodedPassword.c_str(), sizeof(passwordInput) - 1);

    // Buffers to store the decoded data
    unsigned char ssidBuffer[128] = {0};
    unsigned char passwordBuffer[128] = {0};

    // Decode SSID
    unsigned int ssidLength = decode_base64(ssidInput, ssidBuffer);
    ssidBuffer[ssidLength] = '\0'; // Null-terminate the decoded SSID

    // Decode Password
    unsigned int passwordLength = decode_base64(passwordInput, passwordBuffer);
    passwordBuffer[passwordLength] = '\0'; // Null-terminate the decoded Password

    // Assign the decoded values to output Strings
    decodedSSID = String((char*)ssidBuffer);
    decodedPassword = String((char*)passwordBuffer);

    // Debugging output
    Serial.print("Decoded SSID: ");
    Serial.println(decodedSSID);
    Serial.print("Decoded Password: ");
    Serial.println(decodedPassword);

    // Return true if decoding was successful
    return (ssidLength > 0 && passwordLength > 0);
}

void saveSerialNumberToLittleFS(const String &serial) {
    

    File file = LittleFS.open("/serialNumber.txt", "w");
    if (!file) {
        Serial.println("Failed to create file for serial number.");
        return;
    }

    file.println(serial);  // Write the serial number
    file.close();
    Serial.println("Serial number saved to LittleFS: " + serial);
}

String readSerialNumberFromLittleFS() {
    

    File file = LittleFS.open("/serialNumber.txt", "r");
    if (!file) {
        //Serial.println("Serial number file not found.");
        return "";
    }

    String serial = file.readStringUntil('\n');  // Read the serial number
    serial.trim();  // Remove any extra whitespace
    file.close();

    if (serial.isEmpty()) {
        Serial.println("No serial number found in file.");
        return "";
    }

    Serial.println("Serial number read from LittleFS: " + serial);
    return serial;
}

bool isChannelClear() {
  LoRa.idle();  // Ensure LoRa module is not actively transmitting
  int rssi = LoRa.rssi();  // Get the current RSSI value
  //Serial.print("Current RSSI: ");
  //Serial.println(rssi);
  return rssi < LBT_RSSI_THRESHOLD;
}

///////////////////////RTC ALARM CODE ////////////////

void handleFirmwareUpdate(const String& message) {
    Serial.println("Firmware update command received. Processing...");

    // Extract values from the LoRa response
    String sensorID = extractValue(message, "SENSOR");
    String encodedSSID = extractValue(message, "SSID");
    String encodedPassword = extractValue(message, "PASS");

    // Validate that this update is intended for the current sensor
    if (sensorID == serialNumber) {
        Serial.println("Firmware update is for this sensor. Initiating update...");

        // Variables to store the decoded SSID and password
        String decodedSSID;
        String decodedPassword;

        // Decode the SSID and password
        if (decodeUpdate(encodedSSID, encodedPassword, decodedSSID, decodedPassword)) {
            // Attempt to connect to Wi-Fi
            if (connectToWiFi(decodedSSID, decodedPassword)) {
                Serial.println("Connected to Wi-Fi. Checking for firmware update...");
                if (isNewFirmwareAvailable()) {
                    // Before starting OTA update
                    pinMode(10, OUTPUT);
                    digitalWrite(10, LOW); // Hold TPL5110 "done" pin LOW to prevent power cut

                    if (performOTAUpdate(sensor_firmware_version_url, firmwareBinURL)) {
                        Serial.println("Update successful. Waiting for flash to finalize...");
                        Serial.flush();
                        WiFi.disconnect(true); // Disconnect and turn off radio

                        // Wait at least 8 seconds to ensure flash write is complete
                        delay(8000);
                        
                        Serial.println("ESP Restart");
                       
                        delay(1000);
                       
                        ESP.restart();  // Clean restart

                      
                    }
                }
                WiFi.disconnect(true); // Disconnect if no update or if it failed
            } else {
                Serial.println("Failed to connect to Wi-Fi for firmware update.");
            }
        } else {
            Serial.println("Failed to decode SSID or Password.");
        }
    } else {
        Serial.println("Firmware update command ignored: Sensor ID mismatch.");
    }
}

void simulateTwoDaysOfData() {
    // Clear existing data and buffer state
    LittleFS.remove(DATA_FILE);
    LittleFS.remove("/buffer_state.txt");
    
    // Reset buffer state
    bufferState.writePosition = 0;
    bufferState.totalEntries = 0;
    bufferState.isWrapped = false;
    
    File dataFile = LittleFS.open(DATA_FILE, "w");
    if (!dataFile) {
        Serial.println("Failed to open data file for simulation");
        return;
    }
    
    unsigned long now = 1710000000; // Arbitrary start timestamp
    float temp = 25.0;
    
    Serial.printf("Simulating %d days of data (%d readings)...\n", MAX_DAYS_STORAGE, MAX_READINGS);
    
    // Write data in chunks for much better performance
    String chunk = "";
    const int CHUNK_SIZE = 500; // Write 500 entries at a time
    
    for (int i = 0; i < MAX_READINGS; ++i) {
        float t = temp + (rand() % 100) / 100.0 - 0.5; // random temp +/-0.5
        unsigned long ts = now + i * 90; // 90s interval
        
        chunk += String(t, 2) + "," + String(ts) + ",n\n";
        
        // Write chunk when it reaches the target size or at the end
        if ((i + 1) % CHUNK_SIZE == 0 || i == MAX_READINGS - 1) {
            dataFile.print(chunk);
            chunk = "";
            
            // Progress indicator every 1000 entries
            if ((i + 1) % 1000 == 0) {
                Serial.printf("Progress: %d/%d entries\n", i + 1, MAX_READINGS);
            }
        }
    }
    
    dataFile.close();
    
    // Update buffer state to reflect all written entries
    bufferState.totalEntries = MAX_READINGS;
    bufferState.writePosition = dataFile.size();
    bufferState.isWrapped = true; // Mark as wrapped since we've filled the buffer
    saveBufferState();
    
    Serial.printf("Simulated %d days of data (%d points) written to file.\n", MAX_DAYS_STORAGE, MAX_READINGS);
    Serial.printf("Buffer state: %d entries, wrapped: %s\n", 
                  bufferState.totalEntries, bufferState.isWrapped ? "YES" : "NO");
}

// Add a resendLoggedDataWithAck() helper that returns true if ACK received, false otherwise
bool resendLoggedDataWithAck() {
    Serial.print("Backfill batch start: ");
    Serial.println(millis());
    Serial.print("Free Heap before batch: ");
    Serial.println(ESP.getFreeHeap());
    // Copy the body of resendLoggedData, but return true if ACK received, false otherwise
    File dataFile = LittleFS.open(DATA_FILE, "r");
    if (!dataFile) {
        Serial.println("Failed to open data file for reading");
        return false;
    }

    std::vector<float> temps;
    std::vector<unsigned long> times;
    std::vector<size_t> lineNumbers; // To remember which lines are in this batch
    const int MAX_LORA_PAYLOAD = 200; // bytes
    const int MAX_ENTRIES = 7; // safety cap, fits LoRa payload and RAM
    int batchCount = 0;
    size_t lineIndex = 0;

    // Only collect enough for one batch
    while (dataFile.available() && batchCount < MAX_ENTRIES) {
        String line = dataFile.readStringUntil('\n');
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 == -1 || c2 == -1) { lineIndex++; continue; }
        String ack = line.substring(c2 + 1); ack.trim();
        if (ack == "n") {
            float t = line.substring(0, c1).toFloat();
            unsigned long ts = line.substring(c1 + 1, c2).toInt();
            temps.push_back(t);
            times.push_back(ts);
            lineNumbers.push_back(lineIndex);
            batchCount++;
        }
        lineIndex++;
    }
    dataFile.close();

    if (temps.empty()) {
        Serial.println("No unacknowledged data to backfill.");
        return true; // treat as done
    }

    // Now, trim batch to fit LoRa payload
    int entriesToSend = temps.size();
    String payload;
    while (entriesToSend > 0) {
        // Build compact JSON
        DynamicJsonDocument doc(256 + 32 * entriesToSend);
        doc["s"] = serialNumber;
        JsonArray entries = doc.createNestedArray("e");
        for (int i = 0; i < entriesToSend; ++i) {
            JsonArray entry = entries.createNestedArray();
            entry.add(temps[i]);
            entry.add(times[i]);
        }
        String json;
        serializeJson(doc, json);

        // Base64 encode
        char encoded[512];
        unsigned char jsonInput[512];
        strncpy((char*)jsonInput, json.c_str(), sizeof(jsonInput) - 1);
        jsonInput[sizeof(jsonInput) - 1] = '\0';
        encode_base64(jsonInput, json.length(), (unsigned char*)encoded);
        int encodedLen = strlen(encoded);
        Serial.print("JSON length: "); Serial.println(json.length());
        Serial.print("Base64 length: "); Serial.println(encodedLen);
        if (encodedLen <= MAX_LORA_PAYLOAD) {
            payload = String(encoded);
            break;
        }
        entriesToSend--;
    }

    if (entriesToSend == 0) {
        Serial.println("Single entry too large to send via LoRa!");
        return true; // treat as done
    }

    // Send via LoRa
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();
    Serial.print("Packet sent at: ");
    Serial.println(millis());

    // Wait for ACK
    unsigned long start = millis();
    Serial.print("Waiting for ACK at: ");
    Serial.println(start);
    String ackPayload;
    bool gotAck = false;
    const int MAX_ACK_WAIT_MS = 2000;
    while (millis() - start < MAX_ACK_WAIT_MS) {
        int packetSize = LoRa.parsePacket();
        if (packetSize > 0) {
            while (LoRa.available()) {
                ackPayload += (char)LoRa.read();
            }
            gotAck = true;
            break;
        }
        delay(10);
    }
    Serial.print("ACK wait ended at: ");
    Serial.println(millis());

    if (!gotAck) {
        Serial.println("No ACK received for backfill batch.");
        Serial.print("Backfill batch end (timeout): ");
        Serial.println(millis());
        return false;
    }

    Serial.print("Raw ACK payload: ");
    Serial.println(ackPayload);

    // Decode base64 ACK
    char decoded[512];
    unsigned char ackInput[512];
    strncpy((char*)ackInput, ackPayload.c_str(), sizeof(ackInput) - 1);
    ackInput[sizeof(ackInput) - 1] = '\0';
    int decodedLen = decode_base64(ackInput, (unsigned char*)decoded);
    decoded[decodedLen] = '\0';
    String ackJson = String(decoded);

    // Parse ACK and update file
    DynamicJsonDocument ackDoc(256);
    DeserializationError err = deserializeJson(ackDoc, ackJson);
    if (err) {
        Serial.println("Failed to parse ACK JSON.");
        Serial.print("Backfill batch end (parse error): ");
        Serial.println(millis());
        return false;
    }
    if (!ackDoc.containsKey("a")) {
        Serial.print("Backfill batch end (no 'a' key): ");
        Serial.println(millis());
        Serial.print("Free Heap after batch: ");
        Serial.println(ESP.getFreeHeap());
        return false;
    }
    JsonArray acked = ackDoc["a"].as<JsonArray>();

    // Use RAM-buffered file update if enough free heap and file is small, else fallback to line-by-line
    File inFileCheck = LittleFS.open(DATA_FILE, "r");
    size_t fileSize = inFileCheck ? inFileCheck.size() : 0;
    if (inFileCheck) inFileCheck.close();
    if (ESP.getFreeHeap() > 32000 && fileSize < 4096) {
        Serial.print("Using RAM-buffered file update for backfill batch (heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.print(", file size: ");
        Serial.print(fileSize);
        Serial.println(")");
        removeAcknowledgedEntriesRAM(acked);
    } else {
        Serial.print("Using line-by-line file update for backfill batch (heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.print(", file size: ");
        Serial.print(fileSize);
        Serial.println(")");
        removeAcknowledgedEntries(acked);
    }
    Serial.print("Backfill batch end (ACK received): ");
    Serial.println(millis());
    Serial.print("Free Heap after batch: ");
    Serial.println(ESP.getFreeHeap());
    return true;
}

// RAM-buffered version
void removeAcknowledgedEntriesRAM(const JsonArray& ackedTimestamps) {
    std::vector<String> lines;
    File inFile = LittleFS.open(DATA_FILE, "r");
    if (!inFile) return;
    while (inFile.available()) {
        lines.push_back(inFile.readStringUntil('\n'));
    }
    inFile.close();

    std::vector<String> toKeep;
    for (const String& line : lines) {
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 == -1 || c2 == -1) {
            toKeep.push_back(line);
            continue;
        }
        unsigned long ts = line.substring(c1 + 1, c2).toInt();
        bool isAcked = false;
        for (JsonVariant v : ackedTimestamps) {
            if (ts == v.as<unsigned long>()) {
                isAcked = true;
                break;
            }
        }
        if (!isAcked) {
            toKeep.push_back(line);
        }
    }
    File outFile = LittleFS.open(DATA_FILE, "w");
    for (const String& line : toKeep) {
        outFile.println(line);
    }
    outFile.close();
}

// Add a helper to remove acknowledged entries from the data file
void removeAcknowledgedEntries(const JsonArray& ackedTimestamps) {
    File inFile = LittleFS.open(DATA_FILE, "r");
    File outFile = LittleFS.open("/data_tmp.txt", "w");
    if (!inFile || !outFile) {
        Serial.println("Failed to open files for removing ACKed entries.");
        if (inFile) inFile.close();
        if (outFile) outFile.close();
        return;
    }
    while (inFile.available()) {
        String line = inFile.readStringUntil('\n');
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        if (c1 == -1 || c2 == -1) {
            outFile.println(line);
            continue;
        }
        unsigned long ts = line.substring(c1 + 1, c2).toInt();
        bool isAcked = false;
        for (JsonVariant v : ackedTimestamps) {
            if (ts == v.as<unsigned long>()) {
                isAcked = true;
                break;
            }
        }
        if (!isAcked) {
            outFile.println(line);
        }
    }
    inFile.close();
    outFile.close();
    LittleFS.remove(DATA_FILE);
    LittleFS.rename("/data_tmp.txt", DATA_FILE);
}

// Rolling buffer management functions
void initializeRollingBuffer() {
    loadBufferState();
    
    // If this is the first time, initialize the file
    if (bufferState.totalEntries == 0) {
        File dataFile = LittleFS.open(DATA_FILE, "w");
        if (dataFile) {
            dataFile.close();
            Serial.println("[Buffer] Initialized new data file");
        }
    } else {
        Serial.printf("[Buffer] Loaded state: %d entries, pos: %d, wrapped: %s\n", 
                      bufferState.totalEntries, bufferState.writePosition, 
                      bufferState.isWrapped ? "YES" : "NO");
    }
}

void saveBufferState() {
    File stateFile = LittleFS.open("/buffer_state.txt", "w");
    if (stateFile) {
        stateFile.printf("%d,%d,%d\n", bufferState.writePosition, bufferState.totalEntries, bufferState.isWrapped ? 1 : 0);
        stateFile.close();
    }
}

void loadBufferState() {
    File stateFile = LittleFS.open("/buffer_state.txt", "r");
    if (stateFile) {
        String line = stateFile.readStringUntil('\n');
        stateFile.close();
        
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        
        if (comma1 != -1 && comma2 != -1) {
            bufferState.writePosition = line.substring(0, comma1).toInt();
            bufferState.totalEntries = line.substring(comma1 + 1, comma2).toInt();
            bufferState.isWrapped = line.substring(comma2 + 1).toInt() == 1;
        }
    } else {
        // Initialize with default values
        bufferState.writePosition = 0;
        bufferState.totalEntries = 0;
        bufferState.isWrapped = false;
    }
}

size_t calculateWritePosition(size_t entryNumber) {
    // For now, use a simpler approach - always append to the end
    // This is more reliable than trying to calculate exact positions
    // The rolling buffer logic will handle wrap-around by truncating the file
    return 0; // We'll seek to end of file instead
}

void cleanupOldData() {
    // This function can be used to clean up old data if needed
    // For now, the wrap-around logic handles this automatically
    Serial.println("[Buffer] Cleanup not needed - wrap-around handles old data");
}