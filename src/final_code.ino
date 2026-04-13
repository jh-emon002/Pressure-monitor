#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <time.h>


const char* ssid = "Utonium";
const char* password = "verticalline";


LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Preferences prefs;


#define SENSOR1 34
#define SENSOR2 35

#define BTN_MODE 14
#define BTN_UP 26
#define BTN_DOWN 27

#define LED1 18
#define LED2 19
#define BUZZER 23


int menuMode = 0;        // 0 = Normal, 1 = Threshold
int activeChannel = 1; 
int displayUnit = 0;     // 0 = kPa, 1 = mmHg

float threshold1_kPa = 100;
float threshold2_kPa = 100;

bool alarm1 = false;
bool alarm2 = false;

bool lastAlarm1 = false;
bool lastAlarm2 = false;


unsigned long lastLCD = 0;
unsigned long lastDebounce = 0;
unsigned long lastLogTime = 0;
unsigned long ipDisplayTime = 0;

const unsigned long THRESHOLD_DEBOUNCE = 300; 

unsigned long ch1CrossStart = 0;
unsigned long ch2CrossStart = 0;

unsigned long ch1ReturnStart = 0;
unsigned long ch2ReturnStart = 0;

const int LCD_INTERVAL = 300;
const int debounceDelay = 200;
const int LOG_DEBOUNCE = 500; 

bool showingIP = false;


#define MAX_EVENTS 20

String eventLog[MAX_EVENTS];
int eventIndex = 0;


bool lastMode = HIGH;
bool lastUp = HIGH;
bool lastDown = HIGH;


float kPaToMmHg(float kpa)
{
    return kpa * 7.50062; 
}

float mmHgToKPa(float mmhg)
{
    return mmhg / 7.50062;
}

String unitLabel()
{
    return (displayUnit == 0) ? "kPa" : "mmHg";
}


float readPressure(int ch)
{
    int raw =
        (ch == 1)
        ? analogRead(SENSOR1)
        : analogRead(SENSOR2);

    return map(raw, 0, 4095, 0, 200);  // Always in kPa
}

float convertPressure(float kpa)
{
    if (displayUnit == 0)
        return kpa;
    else
        return kPaToMmHg(kpa);
}

float getThreshold(int ch)
{
    float th_kpa = (ch == 1) ? threshold1_kPa : threshold2_kPa;
    
    if (displayUnit == 0)
        return th_kpa;
    else
        return kPaToMmHg(th_kpa);
}


void setupTime()
{
    configTime(21600, 0, "pool.ntp.org", "time.nist.gov");

    struct tm t;

    while (!getLocalTime(&t))
        delay(500);
}

String timestamp()
{
    struct tm t;

    if (!getLocalTime(&t))
        return "NO_TIME";

    char buf[30];

    strftime(
        buf,
        sizeof(buf),
        "%Y-%m-%d %H:%M:%S",
        &t
    );

    return String(buf);
}


void logToCSV(String timestamp, String event)
{
    File file =
        SPIFFS.open(
            "/events.csv",
            FILE_APPEND
        );

    if (!file)
    {
        Serial.println("File open failed");
        return;
    }

    file.print(timestamp);
    file.print(",");
    file.println(event);
    file.close();

    Serial.print(timestamp);
    Serial.print(",");
    Serial.println(event);
}


void addEvent(String msg)
{
    eventLog[eventIndex] = msg;

    eventIndex++;

    if (eventIndex >= MAX_EVENTS)
        eventIndex = 0;
}


void savePrefs()
{
    prefs.begin("sys", false);

    prefs.putFloat("th1", threshold1_kPa);
    prefs.putFloat("th2", threshold2_kPa);
    prefs.putInt("ch", activeChannel);
    prefs.putInt("unit", displayUnit);

    prefs.end();
}

void loadPrefs()
{
    prefs.begin("sys", true);

    threshold1_kPa =
        prefs.getFloat("th1", 100);

    threshold2_kPa =
        prefs.getFloat("th2", 100);

    activeChannel =
        prefs.getInt("ch", 1);
    
    displayUnit =
        prefs.getInt("unit", 0);

    prefs.end();
}


String modeLabel()
{
    if (menuMode == 0)
        return "NRM";

    return "THR";
}

void showMode()
{
    lcd.setCursor(13, 1);
    lcd.print("   ");
    lcd.setCursor(13, 1);
    lcd.print(modeLabel());
}

void updateLCD()
{
    if (showingIP && (millis() - ipDisplayTime > 3000))
    {
        showingIP = false;
        lcd.clear();
    }

    if (showingIP)
        return;

    lcd.setCursor(0, 0);

    lcd.print("CH");
    lcd.print(activeChannel);
    lcd.print(": ");

    float pressure = readPressure(activeChannel);
    float displayPressure = convertPressure(pressure);

    if (displayUnit == 0)
        lcd.print(displayPressure, 1);
    else
        lcd.print(displayPressure, 0); 

    lcd.print(unitLabel());
    lcd.print("  ");

    lcd.setCursor(0, 1);

    lcd.print("TH:");

    float th = getThreshold(activeChannel);
    
    if (displayUnit == 0)
        lcd.print(th, 1);
    else
        lcd.print(th, 0);

    lcd.print(unitLabel());
    lcd.print(" ");

    showMode();
}

void displayIP()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    
    showingIP = true;
    ipDisplayTime = millis();
}


void incThreshold()
{
    float increment = (displayUnit == 0) ? 1.0 : 7.5;  // 1 kPa or ~1 mmHg equivalent
    
    if (activeChannel == 1)
    {
        if (displayUnit == 0)
            threshold1_kPa += increment;
        else
            threshold1_kPa += mmHgToKPa(increment);
    }
    else
    {
        if (displayUnit == 0)
            threshold2_kPa += increment;
        else
            threshold2_kPa += mmHgToKPa(increment);
    }

    savePrefs();
}

void decThreshold()
{
    float decrement = (displayUnit == 0) ? 1.0 : 7.5;
    
    if (activeChannel == 1)
    {
        if (displayUnit == 0)
            threshold1_kPa -= decrement;
        else
            threshold1_kPa -= mmHgToKPa(decrement);
    }
    else
    {
        if (displayUnit == 0)
            threshold2_kPa -= decrement;
        else
            threshold2_kPa -= mmHgToKPa(decrement);
    }

    savePrefs();
}


void checkAlarms()
{
    float p1 = readPressure(1);
    float p2 = readPressure(2);

    

unsigned long now = millis();


if (!alarm1)
{
    if (p1 > threshold1_kPa)
    {
        if (ch1CrossStart == 0)
            ch1CrossStart = now;

        if (now - ch1CrossStart >= THRESHOLD_DEBOUNCE)
        {
            alarm1 = true;
            ch1CrossStart = 0;
        }
    }
    else
    {
        ch1CrossStart = 0;
    }
}
else
{
    if (p1 <= threshold1_kPa)
    {
        if (ch1ReturnStart == 0)
            ch1ReturnStart = now;

        if (now - ch1ReturnStart >= THRESHOLD_DEBOUNCE)
        {
            alarm1 = false;
            ch1ReturnStart = 0;
        }
    }
    else
    {
        ch1ReturnStart = 0;
    }
}


if (!alarm2)
{
    if (p2 > threshold2_kPa)
    {
        if (ch2CrossStart == 0)
            ch2CrossStart = now;

        if (now - ch2CrossStart >= THRESHOLD_DEBOUNCE)
        {
            alarm2 = true;
            ch2CrossStart = 0;
        }
    }
    else
    {
        ch2CrossStart = 0;
    }
}
else
{
    if (p2 <= threshold2_kPa)
    {
        if (ch2ReturnStart == 0)
            ch2ReturnStart = now;

        if (now - ch2ReturnStart >= THRESHOLD_DEBOUNCE)
        {
            alarm2 = false;
            ch2ReturnStart = 0;
        }
    }
    else
    {
        ch2ReturnStart = 0;
    }
}
    digitalWrite(LED1, alarm1);
    digitalWrite(LED2, alarm2);

    if (alarm1 || alarm2)
        digitalWrite(BUZZER, HIGH);
    else
        digitalWrite(BUZZER, LOW);

    
    unsigned long currentTime = millis();
    
    if (currentTime - lastLogTime >= LOG_DEBOUNCE)
    {
        bool logOccurred = false;

        if (alarm1 && !lastAlarm1)
        {
            String ts = timestamp();
            String event = "CH1 crossed threshold";
            String displayMsg = "[" + ts + "] " + event;

            addEvent(displayMsg);
            logToCSV(ts, event);
            logOccurred = true;
        }

        if (!alarm1 && lastAlarm1)
        {
            String ts = timestamp();
            String event = "CH1 returned to normal";
            String displayMsg = "[" + ts + "] " + event;

            addEvent(displayMsg);
            logToCSV(ts, event);
            logOccurred = true;
        }

        if (alarm2 && !lastAlarm2)
        {
            String ts = timestamp();
            String event = "CH2 crossed threshold";
            String displayMsg = "[" + ts + "] " + event;

            addEvent(displayMsg);
            logToCSV(ts, event);
            logOccurred = true;
        }

        if (!alarm2 && lastAlarm2)
        {
            String ts = timestamp();
            String event = "CH2 returned to normal";
            String displayMsg = "[" + ts + "] " + event;

            addEvent(displayMsg);
            logToCSV(ts, event);
            logOccurred = true;
        }

        if (logOccurred)
            lastLogTime = currentTime;
    }

    lastAlarm1 = alarm1;
    lastAlarm2 = alarm2;
}


void buttons()
{
    if (millis() - lastDebounce <
        debounceDelay)
        return;

    bool m = digitalRead(BTN_MODE);
    bool u = digitalRead(BTN_UP);
    bool d = digitalRead(BTN_DOWN);

    // MODE button
    if (m == LOW && lastMode == HIGH)
    {
        menuMode++;

        if (menuMode > 1)
            menuMode = 0;

        lastDebounce = millis();
    }

    if (u == LOW && lastUp == HIGH)
    {
        if (menuMode == 0)
        {
            activeChannel =
                (activeChannel == 1)
                ? 2
                : 1;
            
            savePrefs();
        }
        else
        {
            incThreshold();
        }

        lastDebounce = millis();
    }

    if (d == LOW && lastDown == HIGH)
    {
        if (menuMode == 0)
        {
            displayUnit =
                (displayUnit == 0)
                ? 1
                : 0;
            
            savePrefs();
        }
        else
        {
            decThreshold();
        }

        lastDebounce = millis();
    }

    lastMode = m;
    lastUp = u;
    lastDown = d;
}


void setCORS()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}


void handleData()
{
    setCORS();
    
    StaticJsonDocument<1024> doc;

    float p1 = readPressure(1);
    float p2 = readPressure(2);

    // Send in both units
    doc["p1_kpa"] = p1;
    doc["p2_kpa"] = p2;
    doc["p1_mmhg"] = kPaToMmHg(p1);
    doc["p2_mmhg"] = kPaToMmHg(p2);
    
    doc["threshold1_kpa"] = threshold1_kPa;
    doc["threshold2_kpa"] = threshold2_kPa;
    doc["threshold1_mmhg"] = kPaToMmHg(threshold1_kPa);
    doc["threshold2_mmhg"] = kPaToMmHg(threshold2_kPa);
    
    doc["alarm1"] = alarm1;
    doc["alarm2"] = alarm2;
    doc["activeChannel"] = activeChannel;
    doc["displayUnit"] = unitLabel();

    JsonArray events = doc.createNestedArray("events");
    for (int i = 0; i < MAX_EVENTS; i++)
    {
        if (eventLog[i] != "")
        {
            events.add(eventLog[i]);
        }
    }

    String json;
    serializeJson(doc, json);

    server.send(200, "application/json", json);
}


void handleSet()
{
    setCORS();
    
    int ch = server.arg("ch").toInt();
    float value = server.arg("value").toFloat();
    String unit = server.arg("unit");  


    float value_kpa = value;
    if (unit == "mmhg")
    {
        value_kpa = mmHgToKPa(value);
    }

    if (ch == 1)
        threshold1_kPa = value_kpa;

    if (ch == 2)
        threshold2_kPa = value_kpa;

    savePrefs();

    server.send(200, "text/plain", "OK");
}


void handleDownload()
{
    setCORS();
    
    File file = SPIFFS.open("/events.csv");

    if (!file)
    {
        server.send(404, "text/plain", "No log file");
        return;
    }

    server.streamFile(file, "text/csv");
    file.close();
}


void handleOptions()
{
    setCORS();
    server.send(200);
}


void setup()
{
    Serial.begin(115200);

    pinMode(BTN_MODE, INPUT_PULLUP);
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    lcd.init();
    lcd.backlight();

    loadPrefs();


    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS failed");
    }


    if (!SPIFFS.exists("/events.csv"))
    {
        File file = SPIFFS.open("/events.csv", FILE_WRITE);
        file.println("Timestamp,Event");
        file.close();
    }


    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting WiFi");
    
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    setupTime();

    server.on("/data", HTTP_GET, handleData);
    server.on("/set", HTTP_GET, handleSet);
    server.on("/download", HTTP_GET, handleDownload);
    
    server.on("/data", HTTP_OPTIONS, handleOptions);
    server.on("/set", HTTP_OPTIONS, handleOptions);
    server.on("/download", HTTP_OPTIONS, handleOptions);

    server.begin();

    Serial.println("ESP32 Data Server Ready");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    displayIP();
}


void loop()
{
    buttons();
    checkAlarms();
    server.handleClient();

    if (millis() - lastLCD > LCD_INTERVAL)
    {
        updateLCD();
        lastLCD = millis();
    }
}

