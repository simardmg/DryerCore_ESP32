#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <esp_task_wdt.h>

// ==== CONFIG ====
#define DHTPIN 4
#define DHTTYPE DHT11
#define RELAY_PIN 5
#define LED_PIN 48

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

const float MAX_TEMP = 70.0;
const uint32_t READ_INTERVAL = 2000;
const uint32_t SENSOR_TIMEOUT = 10000;
const uint32_t MANUAL_TIMEOUT = 3600000;

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// ==== PROFILES ====
struct Profile {
	const char* name;
	float target;
	float hysteresis;
	uint32_t duration;
};

Profile profiles[] = {
	{"PLA", 45.0, 2.0, 2UL * 3600000},
	{"PETG", 55.0, 2.0, 4UL * 3600000},
	{"ABS", 60.0, 2.0, 3UL * 3600000},
	{"ASA", 60.0, 2.0, 4UL * 3600000},
	{"TPU", 50.0, 2.0, 4UL * 3600000},
	{"NYLON", 65.0, 2.0, 6UL * 3600000}
};

Profile* current = nullptr;

// ==== STATE ====
enum Mode { IDLE, AUTO, MANUAL_ON, MANUAL_OFF };

volatile float temp = 0;
volatile float hum = 0;
volatile bool relayState = false;
volatile bool sensorFault = false;

volatile Mode mode = IDLE;

uint32_t startTime = 0;
uint32_t lastRead = 0;
uint32_t lastValid = 0;

char reason[64] = "Booting";

// ==== UI (unchanged, embedded HTML) ====
const char* html PROGMEM = R"rawliteral(
<!-- keep your full HTML here unchanged -->
)rawliteral";

// ==== ROUTES ====

void handleRoot() {
  server.send_P(200, "text/html", html);
}

void handleData() {
  uint32_t now = millis();
  uint32_t rem = 0;

  if (mode == AUTO && current) {
    if (current->duration > (now - startTime))
      rem = (current->duration - (now - startTime)) / 1000;
  } else if (mode == MANUAL_ON) {
    if (MANUAL_TIMEOUT > (now - startTime))
      rem = (MANUAL_TIMEOUT - (now - startTime)) / 1000;
  }

  char json[256];
  snprintf(json, sizeof(json),
           "{\"t\":%.1f,\"h\":%.1f,\"r\":%s,\"rsn\":\"%s\",\"err\":%s,\"rem\":%lu}",
           temp, hum,
           relayState ? "true" : "false",
           reason,
           sensorFault ? "true" : "false",
           rem);

  server.send(200, "application/json", json);
}

void handleCmd() {
  String a = server.arg("a");

  if (a == "auto") {
    String m = server.arg("m");
    for (auto& p : profiles) {
      if (m == p.name) {
        current = &p;
        mode = AUTO;
        startTime = millis();
        server.send(200, "text/plain", "OK");
        return;
      }
    }
  }

  else if (a == "mon") {
    mode = MANUAL_ON;
    startTime = millis();
    server.send(200, "text/plain", "OK");
    return;
  }

  else if (a == "moff") {
    mode = MANUAL_OFF;
    server.send(200, "text/plain", "OK");
    return;
  }

  server.send(400, "text/plain", "Bad");
}

// ==== CONTROL LOOP ====

void controlTask(void*) {
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL);

  char lastReason[64] = "";

  for (;;) {
    esp_task_wdt_reset();
    uint32_t now = millis();

    // Read sensor
    if (now - lastRead > READ_INTERVAL) {
      lastRead = now;

      float t = dht.readTemperature();
      float h = dht.readHumidity();

      if (!isnan(t) && !isnan(h)) {
        temp = t;
        hum = h;
        lastValid = now;
        sensorFault = false;
      }
    }

    // Safety first
    if (now - lastValid > SENSOR_TIMEOUT) {
      sensorFault = true;
      mode = IDLE;
      relayState = false;
      snprintf(reason, sizeof(reason), "Sensor timeout");
    }
    else if (temp >= MAX_TEMP) {
      mode = IDLE;
      relayState = false;
      snprintf(reason, sizeof(reason), "Overtemp %.1fC", temp);
    }
    else {
      switch (mode) {

        case IDLE:
        case MANUAL_OFF:
          relayState = false;
          snprintf(reason, sizeof(reason), "Idle");
          break;

        case MANUAL_ON:
          if (now - startTime > MANUAL_TIMEOUT) {
            mode = IDLE;
            relayState = false;
          } else {
            relayState = true;
            snprintf(reason, sizeof(reason), "Manual ON");
          }
          break;

        case AUTO:
          if (!current) {
            mode = IDLE;
            break;
          }

          if (now - startTime > current->duration) {
            mode = IDLE;
            relayState = false;
            snprintf(reason, sizeof(reason), "Done");
          }
          else if (temp < (current->target - current->hysteresis)) {
            relayState = true;
            snprintf(reason, sizeof(reason), "Heating");
          }
          else if (temp >= current->target) {
            relayState = false;
            snprintf(reason, sizeof(reason), "Holding");
          }
          break;
      }
    }

    // Log only on change
    if (strcmp(reason, lastReason) != 0) {
      Serial.println(reason);
      strncpy(lastReason, reason, sizeof(lastReason));
    }

    // Relay is active LOW
    digitalWrite(RELAY_PIN, relayState ? LOW : HIGH);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ==== WEB LOOP ====

void webTask(void*) {
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ==== SETUP ====

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  dht.begin();

  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    retry++;
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/cmd", handleCmd);

  server.begin();

  xTaskCreatePinnedToCore(controlTask, "ctrl", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(webTask, "web", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}