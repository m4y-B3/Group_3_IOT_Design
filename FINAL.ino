#define BLYNK_TEMPLATE_ID "TMPL6fqbqLXnV"
#define BLYNK_TEMPLATE_NAME "Museum Environmental Monitoring"
#define BLYNK_AUTH_TOKEN "SsRTVNGYDOrbepbX5IZu5U6oBJgGdYjc"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHTesp.h>

BlynkTimer timer;
DHTesp dht;

// ==== Pin Definitions ====
#define DHT_PIN 14
#define LDR_PIN 12
#define RELAY_PIN 10
#define LED_PIN 18

// ==== Blynk Virtual Pins ====
#define VPIN_SYSTEM_POWER V0    // System Power
#define VPIN_TEMPERATURE V1     // Temperature (°C)
#define VPIN_HUMIDITY V2        // Humidity (%)
#define VPIN_LIGHT V3           // Light Status
#define VPIN_FAN_STATUS V4      // Fan Status
#define VPIN_LED_STATUS V5      // LED Status

// ==== Threshold ====
#define TEMP_THRESHOLD 40.0
#define HUM_THRESHOLD 80.0

// Global variables
float currentTemp = 0;
float currentHum = 0;
bool isLightOn = false;
bool isFanOn = false;
bool isLedOn = false;
bool systemOn = true;

// Timer IDs
int timerTempHumID;
int timerLdrID;
int timerSendDataID;

// Wi-Fi Credentials
char ssid[] = "msshmrb";
char pass[] = "kabitniseungmo";
char auth[] = BLYNK_AUTH_TOKEN;

void setup() {
  Serial.begin(115200);
  delay(100);

  Blynk.begin(auth, ssid, pass);

  dht.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup timers for periodic sensor readings
  timerTempHumID = timer.setInterval(5000L, readTemperatureHumidity);
  timerLdrID = timer.setInterval(5000L, readLDR);
  timerSendDataID = timer.setInterval(5000L, sendDataToBlynk);
}


// ==== Reading Temperature and Humidity + Fan Controlling ====
void readTemperatureHumidity() {
  if (!systemOn) return;

  // ==== Reading Temperature and Humidity
  TempAndHumidity data = dht.getTempAndHumidity();

  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  currentTemp = data.temperature;
  currentHum = data.humidity;

  Serial.printf("Temperature: %.2f°C\tHumidity: %.2f%%\n", data.temperature, data.humidity);

  // ==== Fan Controlling ====
  if (currentHum > HUM_THRESHOLD || currentTemp > TEMP_THRESHOLD) {
    digitalWrite(RELAY_PIN, HIGH);  // Turn ON fan
    isFanOn = true;
    Serial.println("⚠️ Fan ON");
  } else {
    digitalWrite(RELAY_PIN, LOW);   // Turn OFF fan
    isFanOn = false;
    Serial.println("✅ Fan OFF");
  }
}

// ==== Reading Lighting + Warning LED ====
void readLDR(){
  if (!systemOn) return;

  int lightState = digitalRead(LDR_PIN);  // Read digital value

  if (currentHum > HUM_THRESHOLD || currentTemp > TEMP_THRESHOLD || lightState == LOW) {
    digitalWrite(LED_PIN, HIGH);  // Turn ON warning LED
    isLedOn = true;
  } else {
    digitalWrite(LED_PIN, LOW);   // Turn OFF warning LED
    isLedOn = false;
  }

  if (lightState == LOW) {
    Serial.println("💡 BRIGHT");
    isLightOn = true;
  } else {
    Serial.println("🌑 DARK");
    isLightOn = false;
  }

}

// ==== Send Data to Blynk ====
void sendDataToBlynk() {
  if (!systemOn) return;

  Blynk.virtualWrite(VPIN_TEMPERATURE, currentTemp);
  Blynk.virtualWrite(VPIN_HUMIDITY, currentHum);
  Blynk.virtualWrite(VPIN_LIGHT, isLightOn ? "BRIGHT" : "DARK");
  Blynk.virtualWrite(VPIN_FAN_STATUS, isFanOn ? "ON" : "OFF");
  Blynk.virtualWrite(VPIN_LED_STATUS, isLedOn ? "ON" : "OFF");
  
  Serial.println("Data sent to Blynk");
}

// ==== Blynk Virtual Pin Handler - System Power Control ====
BLYNK_WRITE(V0) {
  int powerControl = param.asInt();
  systemOn = (powerControl == 1);

  if (!systemOn) {
    // Stop timers to stop sensor readings and data sending
    timer.deleteTimer(timerTempHumID);
    timer.deleteTimer(timerLdrID);
    timer.deleteTimer(timerSendDataID);

    // Turn off outputs
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    isFanOn = false;
    isLedOn = false;
    Serial.println("❌ System powered OFF");
  } else {
    // Restart timers
    timerTempHumID = timer.setInterval(5000L, readTemperatureHumidity);
    timerLdrID = timer.setInterval(5000L, readLDR);
    timerSendDataID = timer.setInterval(5000L, sendDataToBlynk);

    Serial.println("✅ System powered ON");
  }
}

// ==== Blynk Connection Status ====
BLYNK_CONNECTED() {
  Serial.println("Connected to Blynk server");
  // Request the server to send the latest values for all pins
  Blynk.syncAll();
}

BLYNK_DISCONNECTED() {
  Serial.println("Disconnected from Blynk server");
}

void loop() {
  Blynk.run();  // Handle Blynk operations
  timer.run();        // Handle timer events
}
