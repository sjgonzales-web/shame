#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <time.h>
#include <DHT.h>

// ================= PIN CONFIG =================
#define DHTPIN 16
#define DHTTYPE DHT11

#define LED1 17
#define LED2 18
#define LED3 19
#define LED4 21
#define LED5 22

#define MOTOR_PIN 27   // ON/OFF only

// ================= FIREBASE =================
#define WEB_API_KEY "AIzaSyByPHN3USjnb8-F8qpkr4x7dsVPwcwlec8"
#define DATABASE_URL "https://shame-john-gonzales-default-rtdb.firebaseio.com"
#define USER_EMAIL "shame@gmail.com"
#define USER_PASS "12345678"

// ================= TIME =================
#define GMT_OFFSET_SEC (8 * 3600)
#define DAYLIGHT_OFFSET_SEC 0

// ================= OBJECTS =================
DHT dht(DHTPIN, DHTTYPE);
WiFiManager wm;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= TIMER =================
unsigned long lastSend = 0;
const unsigned long interval = 10000; // 10 sec

// =====================================================

void initWiFi() {
  WiFi.mode(WIFI_STA);
  wm.setConnectTimeout(20);

  if (!wm.autoConnect("ESP32-Setup", "12345678")) {
    Serial.println("WiFi Failed. Restarting...");
    delay(1000);
    ESP.restart();
  }

  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());
}

void initTime() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC,
             "pool.ntp.org", "time.nist.gov");

  Serial.print("Syncing time");
  while (time(nullptr) < 1609459200) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTime Synced!");
}

void initFirebase() {
  config.api_key = WEB_API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Ready");
}

// =====================================================
// ================= CONTROL LISTENER ==================
// =====================================================

void listenControls() {

  String devices[] = {"LED1","LED2","LED3","LED4","LED5","MOTOR"};

  for (String dev : devices) {

    String path = "Controls/" + dev + "/state";

    if (Firebase.RTDB.getInt(&fbdo, path)) {

      int state = fbdo.intData();

      if (dev == "LED1") digitalWrite(LED1, state);
      if (dev == "LED2") digitalWrite(LED2, state);
      if (dev == "LED3") digitalWrite(LED3, state);
      if (dev == "LED4") digitalWrite(LED4, state);
      if (dev == "LED5") digitalWrite(LED5, state);

      // FIXED SPEED MOTOR (ON / OFF only)
      if (dev == "MOTOR") {
        digitalWrite(MOTOR_PIN, !state);
      }
    }
  }
}

// =====================================================
// ================= SEND SENSOR DATA ==================
// =====================================================

void sendData() {

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT Read Failed");
    return;
  }

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char dateStr[12];
  sprintf(dateStr, "%04d-%02d-%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday);

  char hourStr[3], minStr[3], secStr[3];
  sprintf(hourStr, "%02d", timeinfo.tm_hour);
  sprintf(minStr, "%02d", timeinfo.tm_min);
  sprintf(secStr, "%02d", timeinfo.tm_sec);

  String basePath = "DHT11/" + String(dateStr) + "/" +
                    String(hourStr) + "/" +
                    String(minStr) + "/" +
                    String(secStr);

  Firebase.RTDB.setFloat(&fbdo, basePath + "/temperature", temperature);
  Firebase.RTDB.setFloat(&fbdo, basePath + "/humidity", humidity);

  Serial.printf("Sent: %.1f°C | %.1f%%\n", temperature, humidity);
}

// =====================================================

void setup() {

  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);   // Added for motor

  dht.begin();

  initWiFi();
  initTime();
  initFirebase();
}

void loop() {

  if (Firebase.ready()) {
    listenControls();
  }

  if (millis() - lastSend > interval) {
    lastSend = millis();
    sendData();
  }

  delay(200);
}
