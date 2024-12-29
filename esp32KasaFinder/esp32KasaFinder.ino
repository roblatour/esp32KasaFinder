// esp32 Kasa Finder
//
// Copyright Rob Latour, 2024
// License: MIT
//
// https://github.com/roblatour/esp32KasaFinder
//
// Compile and upload using Arduino IDE (2.3.4 or greater)
//
// Physical board:                        ESP32 with built in 2.8 inch TFT display ( ESP32-2432S028R ) - https://www.aliexpress.com/item/1005004502250619.html
// Board Manager:                         ESP32 by Espressif Systems board library v2.0.13 (Note: the latest version of the esp32 board library may not work with this program / the above board)
// Board in Arduino board manager:        ESP32 Dev Module
//
// Arduino Tools settings:
//
// USB CDC On Boot:                       "Enabled"
// USB DFU On Boot:                       "Disabled"

// CPU Frequency:                         "240MHz (WiFi)"
// Core Debug Level:                      "None"
// Erase All Flash Before Sketch Upload:  "Disabled"
// Events Run On:                         "Core 1"
// Flash Frequency                        "80MHz"
// Flash Mode:                            "QI0"
// Flash Size                             "4MB (32Mb)"
// JTAG Adapter                           "Disabled"
// Arduino Runs On                        "Core 1"
// USB Firmware MSCOn Boot:               "Disabled"
// Partition Scheme:                      "Default 4BB with spiffs (1.285MB APP/1.5MB SPIFFS)"
// PSRAM:                                 "Disabled"
// Upload Speed:                          "921600"
//
// ----------------------------------------------------------------
// Programmer                             ESPTool
// ----------------------------------------------------------------

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h> // download from: http://pan.jczn1688.com/directlink/1/ESP32%20module/2.8inch_ESP32-2432S028R.rar

#include "pin_config.h"
#include "virtual_window.h"

#include <bb_spi_lcd.h> // download from: https://github.com/bitbank2/bb_spi_lcd
BB_SPI_LCD touchPanel;

#include "KasaSmartPlug.h" // download from: https://github.com/roblatour/KasaSmartPlug
                           // the above library also requires https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
KASAUtil kasaUtil;
std::vector<KASASmartPlug> allDevices;

#include "user_secrets.h"
#include "user_settings.h"

const char *WiFi_SSID = SECRET_WIFI_SSID;
const char *WiFi_Password = SECRET_WIFI_PASSWORD;

TFT_eSPI showPanel = TFT_eSPI(SETTINGS_TFT_WIDTH, SETTINGS_TFT_HEIGHT);

#define LCD_BACKLIGHT PIN_POWER_ON

VirtualWindow *virtualWindow;

void holdHereForeverAndEver()
{
  while (true)
    delay(60000);
}

enum addNewLine
{
  addNewLineYes,
  addNewLineNo,
};

enum sendToSerial
{
  sendToSerialYes,
  sendToSerialNo,
};

enum sendToTFTDisplay
{
  sendToTFTDisplayYes,
  sendToTFTDisplayNo,
};

void sendOutput(String msg, addNewLine addNL = addNewLineYes, sendToSerial toSerial = sendToSerialYes, sendToTFTDisplay toTFTDisplay = sendToTFTDisplayYes)
{

  if (addNL == addNewLineYes)
    msg.concat("\n");

  if (toSerial == sendToSerialYes)
    Serial.print(msg);

  if (toTFTDisplay == sendToTFTDisplayYes)
    virtualWindow->print(msg);
}

void connectToWiFi()
{

  sendOutput("Connecting to " + String(WiFi_SSID) + " ", addNewLineNo);

  WiFi.begin(WiFi_SSID, WiFi_Password);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 30000; // 30 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout)
  {
    delay(500);
    sendOutput(".", addNewLineNo);
  }

  if (WiFi.status() == WL_CONNECTED)
  {

    sendOutput("");
    sendOutput("Connected");
    sendOutput("IP address: " + WiFi.localIP().toString());
    sendOutput("");
  }
  else
  {

    sendOutput("\nFailed to connect to WiFi. Retrying...");
    delay(5000); // Wait for 5 seconds before retrying
    virtualWindow->clear();
    connectToWiFi(); // Retry connection
  }
}

void setupDisplay()
{

  // touch must be initialized before display

  touchPanel.begin(DISPLAY_CYD); // initialize the LCD in landscape mode
  touchPanel.rtInit();           // initialize the resistive touch controller

  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_POWER_ON, OUTPUT);

  showPanel.init();
  showPanel.fillScreen(TFT_BLACK);
  showPanel.setRotation(1);
  showPanel.setTextSize(2);

  virtualWindow = new VirtualWindow(&touchPanel, &showPanel, SETTINGS_MAX_BUFFER_ROWS, SETTINGS_MAX_BUFFER_COLUMNS);

  virtualWindow->setColors(TFT_GREEN, TFT_BLACK);
}

String padString(String str, int width)
{

  str.trim();
  str.substring(0, width);
  while (str.length() < width)
    str += ' ';

  return str;
}

enum includeEverything
{
  includeEverythingYes,
  includeEverythingNo,
};

String createFormattedOutputLine(KASASmartPlug *p, includeEverything includeEveryThing = includeEverything::includeEverythingNo)
{

  const int aliasWidth = 30;
  const int ipWidth = 17;
  const int macWidth = 18;
  const int modelWidth = 10;
  const int stateWidth = 8;

  String formattedOutput = "";

  if ((SETTINGS_SHOW_ALIAS) || (includeEveryThing == includeEverything::includeEverythingYes))
    formattedOutput += padString(String(p->alias), aliasWidth);

  if ((SETTINGS_SHOW_IP_ADDRESS) || (includeEveryThing == includeEverything::includeEverythingYes))
    formattedOutput += padString(String(p->ip_address), ipWidth);

  if ((SETTINGS_SHOW_MAC) || (includeEveryThing == includeEverything::includeEverythingYes))
    formattedOutput += padString(String(p->mac), macWidth);

  if ((SETTINGS_SHOW_MODEL) || (includeEveryThing == includeEverything::includeEverythingYes))
    formattedOutput += padString(String(p->model), modelWidth);

  if ((SETTINGS_SHOW_STATE) || (includeEveryThing == includeEverything::includeEverythingYes))
    if (String(p->model).indexOf("KP") == 0)
      formattedOutput += padString("unknown", stateWidth);
    else
      formattedOutput += padString(String((p->state) == 0 ? "off" : "on"), stateWidth);

  return formattedOutput;
}

void outputDeviceDetails(KASASmartPlug *p)
{

  String formattedOutput;

  formattedOutput = createFormattedOutputLine(p, includeEverything::includeEverythingYes);
  sendOutput(formattedOutput, addNewLineYes, sendToSerialYes, sendToTFTDisplayNo);

  formattedOutput = createFormattedOutputLine(p, includeEverything::includeEverythingNo);
  sendOutput(formattedOutput, addNewLineYes, sendToSerialNo, sendToTFTDisplayYes);
}

// run a broadcast scan to quickly find devices - however this may not find all devices
void performBroadcastScan()
{

  virtualWindow->clear();
  virtualWindow->setColors(SETTINGS_BROADCAST_SCAN_TFT_COLOUR, TFT_BLACK);

  sendOutput("");
  sendOutput("Start broadcast scan");
  sendOutput("");

  String networkBroadcastAddress = "255.255.255.255";

  kasaUtil.SetDebug(false);
  int count = kasaUtil.ScanDevices(SETTINGS_BROADCAST_SCAN_TIMEOUT, networkBroadcastAddress);

  for (int i = 0; i < count; i++)
  {
    KASASmartPlug *p = kasaUtil.GetSmartPlugByIndex(i);
    if (p != NULL)
    {
      allDevices.push_back(*p);
      outputDeviceDetails(p);
    }
  }

  sendOutput("");
  sendOutput("End broadcast scan - ", addNewLineNo);
  if (count == 0)
    sendOutput("no devices found");
  else if (count == 1)
    sendOutput("one device found");
  else
    sendOutput(String(count) + " devices found");
}

// run a direct scan to find devices that were not found by the broadcast scan
void performDirectScan()
{
  virtualWindow->clear();
  virtualWindow->setColors(SETTINGS_DIRECT_SCAN_TFT_COLOUR, TFT_BLACK);

  sendOutput("");
  sendOutput("Start direct scan");
  sendOutput("");
  String scanRange = "Scanning from " + String(SETTINGS_FIRST_THREE_OCTETS_OF_SUBNET_TO_SCAN) + "." + String(SETTINGS_START_IP_HOST_OCTET) + " to " + String(SETTINGS_FIRST_THREE_OCTETS_OF_SUBNET_TO_SCAN) + "." + String(SETTINGS_END_IP_HOST_OCTET);
  sendOutput(scanRange);
  sendOutput("");

  int count = 0;

  for (int i = SETTINGS_START_IP_HOST_OCTET; i <= SETTINGS_END_IP_HOST_OCTET; i++)
  {

    String workingIPAddress = String(SETTINGS_FIRST_THREE_OCTETS_OF_SUBNET_TO_SCAN) + "." + String(i);

    bool deviceAlreadyFoundInBroadcastScan = false;

    // if the device was previously found in the broadcast scan then report it
    for (auto &device : allDevices)
    {
      if (String(device.ip_address) == workingIPAddress)
      {
        count++;
        deviceAlreadyFoundInBroadcastScan = true;

        if (SETTINGS_SHOW_TWO_COLOURS_IN_THE_DIRECT_SCAN)
          virtualWindow->setColors(SETTINGS_BROADCAST_SCAN_TFT_COLOUR, TFT_BLACK);

        outputDeviceDetails(&device);
        break;
      }
    }

    // if the device was previously found in the broadcast scan then don't bother to scan for it again
    if (deviceAlreadyFoundInBroadcastScan)
    {
      // do nothing
    }
    else
    {

      int found = kasaUtil.ScanDevices(SETTINGS_DIRECT_SCAN_TIMEOUT, workingIPAddress);

      if (found == 1)
      {

        count++;

        KASASmartPlug *p = kasaUtil.GetSmartPlugByIndex(0);

        allDevices.push_back(*p);

        if (SETTINGS_SHOW_TWO_COLOURS_IN_THE_DIRECT_SCAN)
          virtualWindow->setColors(SETTINGS_DIRECT_SCAN_TFT_COLOUR, TFT_BLACK);

        outputDeviceDetails(p);
      }
    }
  }

  if (SETTINGS_SHOW_TWO_COLOURS_IN_THE_DIRECT_SCAN)
    virtualWindow->setColors(SETTINGS_DIRECT_SCAN_TFT_COLOUR, TFT_BLACK);

  sendOutput("");
  sendOutput("End direct scan - ", addNewLineNo);
  if (count == 0)
    sendOutput("no devices found");
  else if (count == 1)
    sendOutput("one device found");
  else
    sendOutput(String(count) + " devices found");
}

int ipToInt(const String &ip)
{
  int result = 0;
  int shift = 24;
  int start = 0;
  int end = ip.indexOf('.');
  while (end != -1)
  {
    result |= (ip.substring(start, end).toInt() << shift);
    shift -= 8;
    start = end + 1;
    end = ip.indexOf('.', start);
  }
  result |= (ip.substring(start).toInt() << shift);
  return result;
}

void reportCombinedResults()
{

  virtualWindow->clear();
  virtualWindow->setColors(SETTINGS_COMBINED_RESULTS_TFT_COLOUR, TFT_BLACK);

  sendOutput("");
  sendOutput("All devices");
  sendOutput("");

  int count = allDevices.size();

  if (count == 0)
  {
    sendOutput("No devices found");
  }
  else if (count == 1)
  {
    KASASmartPlug *p = &allDevices[0];
    outputDeviceDetails(p);
    sendOutput("");
    sendOutput("One device found");
  }
  else
  {
    if (SETTINGS_SORT_BY_ALIAS)
    {
      sendOutput("Sorting devices by alias", addNewLine::addNewLineYes, sendToSerial::sendToSerialYes, sendToTFTDisplay::sendToTFTDisplayNo);
      // Sort allDevices by alias
      std::sort(allDevices.begin(), allDevices.end(), [](const KASASmartPlug &a, const KASASmartPlug &b)
                { 
              if (a.alias == nullptr && b.alias == nullptr) return false;
              if (a.alias == nullptr) return false;
              if (b.alias == nullptr) return true;
              return String(a.alias) < String(b.alias); });
    }
    else
    {
      sendOutput("Sorting devices by IP address", addNewLine::addNewLineYes, sendToSerial::sendToSerialYes, sendToTFTDisplay::sendToTFTDisplayNo);
      // Sort allDevices by IP address
      std::sort(allDevices.begin(), allDevices.end(), [](const KASASmartPlug &a, const KASASmartPlug &b)
                { 
              return ipToInt(a.ip_address) < ipToInt(b.ip_address); });
    }

    for (int i = 0; i < count; i++)
    {
      KASASmartPlug *p = &allDevices[i];
      outputDeviceDetails(p);
    }

    sendOutput("");
    sendOutput(String(count) + " devices found");
  }
}

void reportRunTime()
{

  unsigned long totalRunTimeSeconds = millis() / 1000;
  unsigned long minutes = totalRunTimeSeconds / 60;
  unsigned long seconds = totalRunTimeSeconds % 60;
  sendOutput("");
  sendOutput("Total run time: " + String(minutes) + " minutes : " + String(seconds) + " seconds", addNewLine::addNewLineYes, sendToSerial::sendToSerialYes, sendToTFTDisplay::sendToTFTDisplayNo);
}

void setup()
{

  Serial.begin(115200);

  setupDisplay();

  virtualWindow = new VirtualWindow(&touchPanel, &showPanel, SETTINGS_MAX_BUFFER_ROWS, SETTINGS_MAX_BUFFER_COLUMNS);
  virtualWindow->setColors(TFT_GREEN, TFT_BLACK);

  connectToWiFi();

  delay(SETTINGS_DELAY_BETWEEN_SCREENS_IN_MILLISECONDS);

  if (SETTINGS_BROADCAST_SCAN_TIMEOUT > 0)
  {
    performBroadcastScan();
    delay(SETTINGS_DELAY_BETWEEN_SCREENS_IN_MILLISECONDS);
  };

  performDirectScan();

  delay(SETTINGS_DELAY_BETWEEN_SCREENS_IN_MILLISECONDS);

  reportCombinedResults();

  reportRunTime();
}

void loop()
{

  while (true)
  {
    virtualWindow->handleTouch();
    delay(50);
  }
}

// shift alt F to auto format code in Visual Studio Code