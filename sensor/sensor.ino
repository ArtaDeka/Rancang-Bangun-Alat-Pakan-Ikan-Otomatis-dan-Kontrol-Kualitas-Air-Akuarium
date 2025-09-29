/***************************************************************************************
 * PROGRAM FINAL - MONITORING DENGAN JADWAL OTOMATIS RELAY
 * ---------------------------------------------------------------------
 * Perubahan:
 * - Serial Monitor secara rutin (setiap 5 detik) HANYA menampilkan data Suhu
 * dan Kekeruhan untuk monitoring real-time.
 * - Laporan lengkap (termasuk sisa pakan) akan ditampilkan di Serial Monitor
 * HANYA setelah jadwal pagi atau sore tercapai.
 ***************************************************************************************/

// ------ 1. KONFIGURASI BLYNK & WIFI ------
#define BLYNK_TEMPLATE_ID "TMPL64Ym08Qhk"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_PRINT Serial

const char* auth = "bLOWROJ22r_G0zV4EHhxf2XhNGdPRc6i";
const char* ssid = "VIP Putra";
const char* pass = "wiwin123";

// ------ 2. LIBRARY YANG DIBUTUHKAN ------
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"

// ------ 3. KONFIGURASI PIN PERANGKAT KERAS ------
const int ULTRASONIC_PAKAN_TRIG_PIN = 12; //
const int ULTRASONIC_PAKAN_ECHO_PIN = 14; //
const int DS18B20_PIN = 23;               //
const int TURBIDITY_PIN_ADC = 34;         //
const int I2C_SDA_PIN = 21;               //
const int I2C_SCL_PIN = 22;               //
const int RELAY_1_PIN = 16;               //
const int RELAY_2_PIN = 27;               //

// ------ 4. PENGATURAN KALIBRASI ------
const float R1 = 10000.0;               //
const float R2 = 20000.0;               //
const float TEGANGAN_Jernih = 2.0;      //
const float TEGANGAN_Keruh  = 0.5;      //
const int NTU_Jernih = 0;               //
const int NTU_Keruh = 100;              //
const int TINGGI_WADAH_PAKAN_CM = 20;     //

// ------ 5. Inisialisasi Objek & Variabel Global ------
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
RTC_DS3231 rtc;
BlynkTimer timer;

float temperatureC = 0.0;
float turbidityNTU = 0;
long distancePakanCm = 0;
int scheduleHourPagi = -1, scheduleMinutePagi = -1; //
bool hasActionPagi = false; //
int scheduleHourSore = -1, scheduleMinuteSore = -1; //
bool hasActionSore = false; //

// ------ FUNGSI-FUNGSI ------
void readTemperature() {
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);
}

void readTurbidityAndCalibrate() {
  int adcRaw = analogRead(TURBIDITY_PIN_ADC);
  float voltageAtPin = (adcRaw / 4095.0) * 3.3;
  float sensorVoltage = voltageAtPin * (R1 + R2) / R2;
  long sensorVoltageInt = sensorVoltage * 1000;
  long teganganJernihInt = TEGANGAN_Jernih * 1000;
  long teganganKeruhInt = TEGANGAN_Keruh * 1000;
  turbidityNTU = map(sensorVoltageInt, teganganKeruhInt, teganganJernihInt, NTU_Keruh, NTU_Jernih);
  if (sensorVoltage >= TEGANGAN_Jernih) { turbidityNTU = NTU_Jernih; }
  if (sensorVoltage <= TEGANGAN_Keruh) { turbidityNTU = NTU_Keruh; }
}

void checkFoodLevel() {
  digitalWrite(ULTRASONIC_PAKAN_TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PAKAN_TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PAKAN_TRIG_PIN, LOW);
  long duration = pulseIn(ULTRASONIC_PAKAN_ECHO_PIN, HIGH, 25000);
  long currentDistance = duration * 0.0343 / 2;
  distancePakanCm = constrain(currentDistance, 0, TINGGI_WADAH_PAKAN_CM);
}

// Fungsi BARU untuk menampilkan semua data di Serial Monitor
void printAllDataToSerial() {
  Serial.println("\n--- Laporan Status Lengkap (Dipicu Jadwal) ---");
  Serial.print("Suhu: "); Serial.print(temperatureC, 2); Serial.println(" C");
  Serial.print("Kekeruhan: "); Serial.print(turbidityNTU); Serial.println(" NTU");
  Serial.print("Jarak Pakan: "); Serial.print(distancePakanCm); Serial.println(" cm");
  Serial.println("----------------------------------------------");
}

void activateRelays() {
  printAllDataToSerial(); // Tampilkan laporan lengkap sebelum relay aktif
  Serial.println("JADWAL TERPENUHI! Mengaktifkan relay...");
  
  Blynk.virtualWrite(V7, 1);
  digitalWrite(RELAY_1_PIN, LOW);
  delay(5000);
  digitalWrite(RELAY_1_PIN, HIGH);
  Blynk.virtualWrite(V7, 0);
  delay(1000);
  Blynk.virtualWrite(V8, 1);
  digitalWrite(RELAY_2_PIN, LOW);
  delay(5000);
  digitalWrite(RELAY_2_PIN, HIGH);
  Blynk.virtualWrite(V8, 0);
  Serial.println("Siklus relay selesai.");
}

void sendAllSensorData() {
  readTemperature();
  readTurbidityAndCalibrate();
  checkFoodLevel(); // Pakan tetap dibaca untuk dikirim ke Blynk
  
  Blynk.virtualWrite(V0, temperatureC);
  Blynk.virtualWrite(V2, turbidityNTU);
  Blynk.virtualWrite(V6, distancePakanCm);

  // Tampilkan HANYA data rutin di Serial Monitor
  Serial.print("Monitoring Realtime -> Suhu: "); 
  Serial.print(temperatureC, 2); 
  Serial.print(" C | Kekeruhan: "); 
  Serial.print(turbidityNTU); 
  Serial.println(" NTU");
}

void checkSchedule() {
  DateTime now = rtc.now();
  if (now.hour() == 0 && now.minute() == 0) {
    if (hasActionPagi || hasActionSore) {
        hasActionPagi = false;
        hasActionSore = false;
        Serial.println("Flag jadwal harian direset.");
    }
  }
  if (scheduleHourPagi != -1 && now.hour() == scheduleHourPagi && now.minute() == scheduleMinutePagi && !hasActionPagi) {
    activateRelays();
    hasActionPagi = true;
  }
  if (scheduleHourSore != -1 && now.hour() == scheduleHourSore && now.minute() == scheduleMinuteSore && !hasActionSore) {
    activateRelays();
    hasActionSore = true;
  }
}

BLYNK_WRITE(V4) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    scheduleHourPagi = t.getStartHour();
    scheduleMinutePagi = t.getStartMinute();
    hasActionPagi = false;
    Serial.print("Jadwal PAGI diatur ke: ");
    Serial.print(scheduleHourPagi); Serial.print(":"); Serial.println(scheduleMinutePagi);
  }
}

BLYNK_WRITE(V5) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    scheduleHourSore = t.getStartHour();
    scheduleMinuteSore = t.getStartMinute();
    hasActionSore = false;
    Serial.print("Jadwal SORE diatur ke: ");
    Serial.print(scheduleHourSore); Serial.print(":"); Serial.println(scheduleMinuteSore);
  }
}

// ------ FUNGSI UTAMA ------
void setup() {
  Serial.begin(115200);
  Serial.println("\nInisialisasi Sistem (Serial Rapi)...");
  
  WiFi.begin(ssid, pass);
  int wifi_try_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
    if (wifi_try_count++ > 30) {
      Serial.println("\nGAGAL terhubung WiFi."); while(1);
    }
  }
  Serial.println("\nBerhasil terhubung WiFi!");
  
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sensors.begin();
  rtc.begin();
  pinMode(ULTRASONIC_PAKAN_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_PAKAN_ECHO_PIN, INPUT);
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  digitalWrite(RELAY_1_PIN, HIGH);
  digitalWrite(RELAY_2_PIN, HIGH);
  
  Blynk.begin(auth, ssid, pass);
  Blynk.syncVirtual(V4, V5);

  timer.setInterval(5000L, sendAllSensorData);
  timer.setInterval(10000L, checkSchedule);
  
  Serial.println("Sistem Siap Digunakan! Memulai monitoring rutin...");
}

void loop() {
  Blynk.run();
  timer.run();
}