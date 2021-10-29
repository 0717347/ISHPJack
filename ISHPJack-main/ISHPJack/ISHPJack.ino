

/***************************************************
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#define FORMAT_SPIFFS_IF_FAILED true

//The moisture value picked up by the moisture sensor
int WaterVal = 0;
//The pin that the soil moisture sensor is connected to
const int SoilPin = 12;
//set the ideal moisture value of the soil
#define OPTIMAL_WATER_VALUE 4200
// RTC
#include "RTClib.h"

RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#include "FS.h"
// EINK
#include "Adafruit_ThinkInk.h"
//SD libraries
//#include <SPI.h>
//#include <SD.h>
//MotorshieldStuff
#include <Adafruit_MotorShield.h>
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *myMotor = AFMS.getMotor(4);
bool pumpIsRunning = false

                     // Wifi
#define FORMAT_SPIFFS_IF_FAILED true

                     // Wifi & Webserver
// Wifi & Webserver
#include "WiFi.h"
#include "SPIFFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wifiConfig.h"
AsyncWebServer server(80);

const char* http_username = "Password";
const char* http_password = "Username";

String loginIndex, serverIndex;



//Set defines for EInk feather
#define EPD_CS      15
#define EPD_DC      33
#define SRAM_CS     32
#define EPD_RESET   -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    -1 // can set to -1 to not use a pin (will wait a fixed delay)

// 2.13" Monochrome displays with 250x122 pixels and SSD1675 chipset
ThinkInk_213_Mono_B72 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

void setup() {
  //Tell the board to accept input from the soil moisture sensor
  pinMode(SoilPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }

  // RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // The following line can be uncommented if the time needs to be reset.
  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  rtc.start();

  //EINK
  display.begin(THINKINK_MONO);
  display.clearBuffer();

 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println();
  Serial.print("Connected to the Internet");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
  String processor(const String & var) { //"var" is not a descriptive variable name
    if (var == "DATETIME") {
      String datetime = getTimeAsString() + " " + getDateAsString();
      return datetime;
    }
    if (var == "MOISTURE") {
      return String(WaterVal);
    }
    return String();
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  //LED Control
  server.on("/LEDOn", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    digitalWrite(LED_BUILTIN, HIGH);
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });
  server.on("/LEDOff", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    digitalWrite(LED_BUILTIN, LOW);
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  // Pump Control
  server.on("/PumpOn", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    myMotor->run(FORWARD); // May need to change to BACKWARD
    pumpIsRunning = true;
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  server.on("/PumpAuto", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    for (int WaterVal in 0 to OPTIMAL_WATER_VALUE) {
      myMotor->run(FORWARD);
      pumpIsRunning = true;
      request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
    }
  });

  server.on("/PumpOff", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    myMotor->run(RELEASE);
    pumpIsRunning = false;
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  server.on("/logOutput", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("output");
    request->send(SPIFFS, "/logEvents.csv", "text/html", true);
  });




  void loop() {
    updateScreen();
    readSoil();
    // server.handleClient();
  }

  void updateScreen() {
    // Gets the current date and time, and writes it to the Eink display.
    String currentTime = getDateTimeAsString();

    drawText("The Current Time and\nDate is", EPD_BLACK, 2, 0, 0);

    // writes the current time on the bottom half of the display (y is height)
    drawText(currentTime, EPD_BLACK, 2, 0, 75);

    // Draws a line from the leftmost pixel, on line 50, to the rightmost pixel (250) on line 50.
    display.drawLine(0, 50, 250, 50, EPD_BLACK);
    display.display();

    // waits 180 seconds (3 minutes) as per guidelines from adafruit. This delays loop() by 3 minutes. Hmm.
    delay(180000);
  }

  void drawText(String text, uint16_t color, int textSize, int x, int y) {
    display.setCursor(x, y);
    display.setTextColor(color);
    display.setTextSize(textSize);
    display.setTextWrap(true);
    display.print(text);

  }

  String getDateTimeAsString() {
    DateTime now = rtc.now();

    //Prints the date and time to the Serial monitor for debugging.
    /*
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" (");
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();
    */

    // Converts the date and time into a human-readable format.
    char humanReadableDate[20];
    sprintf(humanReadableDate, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());

    return humanReadableDate;
  }

  //Function to read the moisture value of the sensor, then print it to the serial monitor
  //Should probably be logged as well
  void readSoil() {
    WaterVal = analogRead(SoilPin);
    delay(1200);
    Serial.print("Soil Moisture = ");
    //get soil moisture value from the function below and print it
    Serial.println(WaterVal);

  }



  void logEvent(String dataToLog) {
    /*
       Log entries to a file stored in SPIFFS partition on the ESP32.
    */
    // Get the updated/current time
    DateTime rightNow = rtc.now();
    char csvReadableDate[25];
    sprintf(csvReadableDate, "%02d,%02d,%02d,%02d,%02d,%02d,",  rightNow.year(), rightNow.month(), rightNow.day(), rightNow.hour(), rightNow.minute(), rightNow.second());

    String logTemp = csvReadableDate + dataToLog + "\n"; // Add the data to log onto the end of the date/time

    const char * logEntry = logTemp.c_str(); //convert the logtemp to a char * variable

    //Add the log entry to the end of logevents.csv
    appendFile(SPIFFS, "/logEvents.csv", logEntry);

    // Output the logEvents - FOR DEBUG ONLY. Comment out to avoid spamming the serial monitor.
    //  readFile(SPIFFS, "/logEvents.csv");

    Serial.print("\nEvent Logged: ");
    Serial.println(logEntry);
  }

  // SPIFFS file functions
  void readFile(fs::FS & fs, const char * path) {
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      Serial.println("- failed to open file for reading");
      return;
    }

    Serial.println("- read from file:");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  }

  void writeFile(fs::FS & fs, const char * path, const char * message) {
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("- failed to open file for writing");
      return;
    }
    if (file.print(message)) {
      Serial.println("- file written");
    } else {
      Serial.println("- write failed");
    }
    file.close();
  }

  void appendFile(fs::FS & fs, const char * path, const char * message) {
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      Serial.println("- failed to open file for appending");
      return;
    }
    if (file.print(message)) {
      Serial.println("- message appended");
    } else {
      Serial.println("- append failed");
    }
    file.close();
  }

  void renameFile(fs::FS & fs, const char * path1, const char * path2) {
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
      Serial.println("- file renamed");
    } else {
      Serial.println("- rename failed");
    }
  }

  void deleteFile(fs::FS & fs, const char * path) {
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
      Serial.println("- file deleted");
    } else {
      Serial.println("- delete failed");
    }
  }


  String getDateAsString() {
    DateTime now = rtc.now();

    // Converts the date into a human-readable format.
    char humanReadableDate[20];
    sprintf(humanReadableDate, "%02d/%02d/%02d", now.day(), now.month(), now.year());

    return humanReadableDate;
  }

  String getTimeAsString() {
    DateTime now = rtc.now();

    // Converts the time into a human-readable format.
    char humanReadableTime[20];
    sprintf(humanReadableTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    return humanReadableTime;
  }
