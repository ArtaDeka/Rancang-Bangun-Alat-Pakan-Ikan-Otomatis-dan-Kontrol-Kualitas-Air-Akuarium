#include <OneWire.h>
#include <DallasTemperature.h>

// --- Definisi Pin Relay ---
const int RELAY1_PIN = 16;
const int RELAY2_PIN = 27;

// --- Definisi Pin Sensor Suhu ---
const int ONE_WIRE_BUS_PIN = 23; // Pin data untuk DS18B20

// --- Konfigurasi Status Relay ---
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// Inisialisasi library OneWire untuk berkomunikasi dengan sensor
OneWire oneWire(ONE_WIRE_BUS_PIN);

// Berikan referensi oneWire ke library DallasTemperature
DallasTemperature sensors(&oneWire);

void setup() {
  // Mulai komunikasi serial untuk menampilkan output
  Serial.begin(115200);
  Serial.println("Inisialisasi program relay & sensor DS18B20...");

  // --- Setup Relay ---
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, RELAY_ON); // Matikan Relay 1
  digitalWrite(RELAY2_PIN, RELAY_OFF);  // Nyalakan Relay 2
  
  Serial.println("Status: Relay 1 ON, Relay 2 OFF.");

  // --- Setup Sensor Suhu ---
  sensors.begin(); 
  Serial.println("Sensor suhu DS18B20 siap.");
}

void loop() {
  // Minta sensor untuk melakukan konversi suhu.
  sensors.requestTemperatures(); 
  
  // Baca suhu dalam Celcius 
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Tidak dapat membaca data dari sensor suhu!");
  } else {
    // Tampilkan suhu di Serial Monitor
    Serial.print("Suhu saat ini: ");
    Serial.print(tempC);
    Serial.println(" C");
  }
  delay(2000);
}