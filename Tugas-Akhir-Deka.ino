// ------ 1. KONFIGURASI BLYNK & WIFI ------
#define BLYNK_TEMPLATE_ID "TMPL64Ym08Qhk"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_PRINT Serial

const char* auth = "bLOWROJ22r_G0zV4EHhxf2XhNGdPRc6i"; // Masukkan Auth Token Blynk Anda
const char* ssid = "AhmatStarboy";                       // Masukkan nama WiFi Anda
const char* pass = "SempakeAhmatUkuran30";              // Masukkan password WiFi Anda

// ------ 2. LIBRARY ------
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"
#include <ESP32Servo.h>

// ------ 3. KONFIGURASI PIN PERANGKAT ------
const int ULTRASONIC_PAKAN_TRIG_PIN = 12;
const int ULTRASONIC_PAKAN_ECHO_PIN = 14;
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;
const int DS18B20_PIN = 23;
const int TURBIDITY_PIN_ADC = 34;
const int PELTIER_RELAY_PIN = 16;
const int ENA_PIN = 18;
const int IN1_PIN = 2;
const int IN2_PIN = 4;
const int SERVO_PIN = 25;
const int POMPA_O2_RELAY_PIN = 27;

// ------ 4. PENGATURAN KALIBRASI & LOGIKA ------
const float R1 = 10000.0;
const float R2 = 20000.0;
const float TEGANGAN_Jernih = 1.8;
const float TEGANGAN_Keruh  = 0.1;
const int NTU_Jernih = 0;
const int NTU_Keruh = 100;
const float SUHU_PELTIER_ON  = 25.0;
const float SUHU_PELTIER_OFF = 24.0;

// =================================================================
// PERUBAHAN DIMULAI DI SINI
// Variabel pakan kosong dan pakan habis digabung menjadi satu
// =================================================================
const int JARAK_PAKAN_PENUH_CM = 4;
const int JARAK_PAKAN_HABIS = 14; // Variabel ini sekarang mendefinisikan jarak untuk 0% sekaligus trigger notifikasi

// Pengaturan takaran pakan berdasarkan putaran
const int POSISI_BUKA_SERVO = 45;
const int POSISI_TUTUP_SERVO = 90;
const int DELAY_BUKA_MS = 500;
const int DELAY_ANTAR_PUTARAN_MS = 500;

// ------ 5. DEFINISI PARAMETER & STATE LOGIKA FUZZY ------
const float SUHU_DINGIN[] = {15, 15, 18, 21};
const float SUHU_NORMAL[] = {19, 23, 25};
const float SUHU_PANAS[]  = {24, 27, 32, 32};
const float KEKERUHAN_JERNIH[] = {0, 0, 10, 25};
const float KEKERUHAN_SEDANG[] = {15, 35, 55};
const float KEKERUHAN_KERUH[]  = {45, 60, 100, 100};
const float MOTOR_LAMBAT[] = {0, 30, 50};
const float MOTOR_SEDANG[] = {40, 60, 80};
const float MOTOR_CEPAT[]  = {70, 80, 100, 100};

struct AppState {
  float suhuSaatIni;
  float kekeruhanSaatIni;
  long  jarakPakanCm;
  float persentasePakan;
  struct { float dingin, normal, panas; } suhu;
  struct { float jernih, sedang, keruh; } kekeruhan;
  float aturan[10];
  float outputKecepatanMotor;
  int pwmMotor;
  int scheduleHourPagi, scheduleMinutePagi;
  bool hasActionPagi;
  int scheduleHourSore, scheduleMinuteSore;
  bool hasActionSore;
  bool notifPakanTerkirim;
};

AppState state = {0};
int jumlahPutaranPakan = 1;

// ------ 6. Inisialisasi Objek & Timer ------
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
RTC_DS3231 rtc;
BlynkTimer timer;
Servo pakanServo;

// ------ FUNGSI-FUNGSI BACA SENSOR ------
void bacaSuhu() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if(tempC != DEVICE_DISCONNECTED_C) {
    state.suhuSaatIni = tempC;
  }
}

void bacaKekeruhan() {
  int adcRaw = analogRead(TURBIDITY_PIN_ADC);
  float voltageAtPin = (adcRaw / 4095.0) * 3.3;
  float sensorVoltage = voltageAtPin * (R1 + R2) / R2;
  long sensorVoltageInt = sensorVoltage * 1000;
  long teganganJernihInt = TEGANGAN_Jernih * 1000;
  long teganganKeruhInt = TEGANGAN_Keruh * 1000;
  float ntu = map(sensorVoltageInt, teganganKeruhInt, teganganJernihInt, NTU_Keruh, NTU_Jernih);
  state.kekeruhanSaatIni = constrain(ntu, 0, 100);
}

// =================================================================
// PERUBAHAN PADA FUNGSI BACA LEVEL PAKAN
// Menggunakan variabel baru `JARAK_PAKAN_HABIS`
// =================================================================
void bacaLevelPakan() {
  digitalWrite(ULTRASONIC_PAKAN_TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PAKAN_TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PAKAN_TRIG_PIN, LOW);
  
  long duration = pulseIn(ULTRASONIC_PAKAN_ECHO_PIN, HIGH, 25000);
  long currentDistance = duration * 0.0343 / 2;
  
  state.jarakPakanCm = currentDistance;
  
  // Batasi (constrain) pembacaan jarak antara jarak penuh dan jarak habis
  long constrainedDistance = constrain(currentDistance, JARAK_PAKAN_PENUH_CM, JARAK_PAKAN_HABIS);
  
  // Petakan (map) nilai jarak ke rentang persentase menggunakan variabel baru
  float persentase = map(constrainedDistance, JARAK_PAKAN_PENUH_CM, JARAK_PAKAN_HABIS, 100, 0);
  
  state.persentasePakan = persentase;
}
// =================================================================
// AKHIR DARI PERUBAHAN
// =================================================================


// ------ FUNGSI HIMPUNAN KEANGGOTAAN (FUZZY) ------
float trimf(float x, const float params[]) {
  float a = params[0], b = params[1], c = params[2];
  if (x <= a || x >= c) return 0.0;
  if (x > a && x <= b) return (x - a) / (b - a);
  if (x > b && x < c) return (c - x) / (c - b);
  return 0.0;
}

float trapmf(float x, const float params[]) {
  float a = params[0], b = params[1], c = params[2], d = params[3];
  if (x <= a || x >= d) return 0.0;
  if (x > a && x < b) return (x - a) / (b - a);
  if (x >= b && x <= c) return 1.0;
  if (x > c && x < d) return (d - x) / (d - c);
  return 0.0;
}

// ------ FUNGSI INTI LOGIKA FUZZY ------
void fuzzifikasi() {
  state.suhu.dingin = trapmf(state.suhuSaatIni, SUHU_DINGIN);
  state.suhu.normal = trimf(state.suhuSaatIni, SUHU_NORMAL);
  state.suhu.panas  = trapmf(state.suhuSaatIni, SUHU_PANAS);
  state.kekeruhan.jernih = trapmf(state.kekeruhanSaatIni, KEKERUHAN_JERNIH);
  state.kekeruhan.sedang = trimf(state.kekeruhanSaatIni, KEKERUHAN_SEDANG);
  state.kekeruhan.keruh  = trapmf(state.kekeruhanSaatIni, KEKERUHAN_KERUH);
}

void inferensi() {
  state.aturan[1] = min(state.suhu.dingin, state.kekeruhan.jernih);
  state.aturan[2] = min(state.suhu.dingin, state.kekeruhan.sedang);
  state.aturan[3] = min(state.suhu.dingin, state.kekeruhan.keruh);
  state.aturan[4] = min(state.suhu.normal, state.kekeruhan.jernih);
  state.aturan[5] = min(state.suhu.normal, state.kekeruhan.sedang);
  state.aturan[6] = min(state.suhu.normal, state.kekeruhan.keruh);
  state.aturan[7] = min(state.suhu.panas,  state.kekeruhan.jernih);
  state.aturan[8] = min(state.suhu.panas,  state.kekeruhan.sedang);
  state.aturan[9] = min(state.suhu.panas,  state.kekeruhan.keruh);
}

void defuzzifikasiCentroid() {
  float pembilang = 0, penyebut = 0;
  float motor_lambat_strength = max(state.aturan[1], state.aturan[4]);
  float motor_sedang_strength = max(max(state.aturan[2], state.aturan[5]), state.aturan[7]);
  float motor_cepat_strength  = max(max(state.aturan[3], state.aturan[6]), max(state.aturan[8], state.aturan[9]));
  for (int y = 0; y <= 100; y++) {
    float mu_lambat = trimf(y, MOTOR_LAMBAT);
    float mu_sedang = trimf(y, MOTOR_SEDANG);
    float mu_cepat  = trapmf(y, MOTOR_CEPAT);
    float mu_clipped_lambat = min(motor_lambat_strength, mu_lambat);
    float mu_clipped_sedang = min(motor_sedang_strength, mu_sedang);
    float mu_clipped_cepat  = min(motor_cepat_strength, mu_cepat);
    float mu_aggregated = max(max(mu_clipped_lambat, mu_clipped_sedang), mu_clipped_cepat);
    pembilang += y * mu_aggregated;
    penyebut  += mu_aggregated;
  }
  if (penyebut == 0) {
    state.outputKecepatanMotor = 0;
  } else {
    state.outputKecepatanMotor = pembilang / penyebut;
  }
}

// ------ FUNGSI KONTROL AKTUATOR ------
void aktivasiPakan() {
  Serial.println("JADWAL PEMBERIAN PAKAN!");
  Serial.print("Takaran diatur untuk: "); Serial.print(jumlahPutaranPakan); Serial.println(" putaran.");
  for (int i = 0; i < jumlahPutaranPakan; i++) {
    Serial.print("Putaran ke-"); Serial.println(i + 1);
    pakanServo.write(POSISI_BUKA_SERVO);
    delay(DELAY_BUKA_MS);
    pakanServo.write(POSISI_TUTUP_SERVO);
    delay(DELAY_ANTAR_PUTARAN_MS);
  }

  Serial.println("Siklus pemberian pakan selesai.");
  Blynk.logEvent("pakan_selesai", "Pemberian pakan telah selesai dilakukan.");
}

void kontrolAktuator() {
  state.pwmMotor = map(state.outputKecepatanMotor, 0, 100, 0, 255);
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, state.pwmMotor);
  if (state.suhuSaatIni >= SUHU_PELTIER_ON) {
    digitalWrite(PELTIER_RELAY_PIN, LOW);
  }
  else if (state.suhuSaatIni <= SUHU_PELTIER_OFF) {
    digitalWrite(PELTIER_RELAY_PIN, HIGH);
  }
}

// ------ FUNGSI UTAMA & LOOP EKSEKUSI ------
void ambilDataDanJalankanFuzzy() {
  bacaSuhu();
  bacaKekeruhan();
  if (state.suhuSaatIni == -127.0) {
    Serial.println("Gagal membaca sensor suhu, skip loop fuzzy...");
    return;
  }

  fuzzifikasi();
  inferensi();
  defuzzifikasiCentroid();
  kontrolAktuator();
  tampilkanDataSerial();
}

void kirimDataKeBlynk() {
  Blynk.virtualWrite(V0, state.suhuSaatIni);
  Blynk.virtualWrite(V2, state.kekeruhanSaatIni);
}

void cekDanKirimSisaPakan() {
  Serial.println("Menerima perintah dari Blynk untuk cek pakan...");
  bacaLevelPakan();
  Blynk.virtualWrite(V6, state.persentasePakan);
  Serial.print("Data pakan dikirim ke Blynk: ");
  Serial.print(state.persentasePakan, 2); Serial.println(" %");
  if (state.jarakPakanCm >= JARAK_PAKAN_HABIS && !state.notifPakanTerkirim) {
    Blynk.logEvent("pakan_hampir_habis", "Pakan ikan hampir habis, segera isi ulang!");
    state.notifPakanTerkirim = true;
  }
  else if (state.jarakPakanCm < JARAK_PAKAN_HABIS) {
    if (state.notifPakanTerkirim) {
       Serial.println("Pakan telah diisi ulang, flag notifikasi direset.");
    }
    state.notifPakanTerkirim = false;
  }
}


void cekJadwalPakan() {
  DateTime now = rtc.now();
  if (now.hour() == 0 && now.minute() == 0) {
    if (state.hasActionPagi || state.hasActionSore) {
        state.hasActionPagi = false;
        state.hasActionSore = false;
        Serial.println("Flag jadwal harian direset.");
    }
  }
  
  if (state.scheduleHourPagi != -1 && now.hour() == state.scheduleHourPagi && now.minute() == state.scheduleMinutePagi && !state.hasActionPagi) {
    aktivasiPakan();
    state.hasActionPagi = true;
  }
  if (state.scheduleHourSore != -1 && now.hour() == state.scheduleHourSore && now.minute() == state.scheduleMinuteSore && !state.hasActionSore) {
    aktivasiPakan();
    state.hasActionSore = true;
  }
}

// ------ FUNGSI TAMPILAN & BLYNK ------
void tampilkanDataSerial() {
  Serial.println("---------------------------------");
  Serial.print("Suhu Saat Ini: "); Serial.print(state.suhuSaatIni, 2);
  Serial.println(" C");
  Serial.print("Kekeruhan Saat Ini: "); Serial.print(state.kekeruhanSaatIni, 2); Serial.println(" NTU");
  Serial.println("--- Hasil Fuzzy ---");
  Serial.print("Output Kecepatan Motor (0-100): "); Serial.println(state.outputKecepatanMotor);
  Serial.println("--- Aksi Aktuator ---");
  Serial.print("PWM Motor Dikirim (0-255): "); Serial.println(state.pwmMotor);
  Serial.print("Status Relay Peltier: ");
  Serial.println(digitalRead(PELTIER_RELAY_PIN) == LOW ? "NYALA" : "MATI");
  Serial.println("--- Data Pakan---");
  Serial.print("Jarak Pakan: "); Serial.print(state.jarakPakanCm); Serial.println(" cm");
  Serial.print("Sisa Pakan: ");
  Serial.print(state.persentasePakan, 2); Serial.println(" %");
  
  Serial.println("--- Status Jadwal Pakan ---");
  DateTime now = rtc.now();
  Serial.print("Waktu RTC: ");
  if(now.hour() < 10) Serial.print("0");
  Serial.print(now.hour());
  Serial.print(":");
  if(now.minute() < 10) Serial.print("0");
  Serial.print(now.minute());

  Serial.print(" | Pagi: ");
  if(state.scheduleHourPagi < 0) Serial.print("--:--");
  else {
    if(state.scheduleHourPagi < 10) Serial.print("0");
    Serial.print(state.scheduleHourPagi);
    Serial.print(":");
    if(state.scheduleMinutePagi < 10) Serial.print("0");
    Serial.print(state.scheduleMinutePagi);
  }
  Serial.print(" (Status: ");
  Serial.print(state.hasActionPagi ? "Sudah" : "Belum");
  Serial.print(")");

  Serial.print(" | Sore: ");
  if(state.scheduleHourSore < 0) Serial.print("--:--");
  else {
    if(state.scheduleHourSore < 10) Serial.print("0");
    Serial.print(state.scheduleHourSore);
    Serial.print(":");
    if(state.scheduleMinuteSore < 10) Serial.print("0");
    Serial.print(state.scheduleMinuteSore);
  }
  Serial.print(" (Status: ");
  Serial.print(state.hasActionSore ? "Sudah" : "Belum");
  Serial.print(")");
  Serial.println();
  
  Serial.println("---------------------------------");
}

BLYNK_WRITE(V4) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    state.scheduleHourPagi = t.getStartHour();
    state.scheduleMinutePagi = t.getStartMinute();
    state.hasActionPagi = false;
    Serial.print("Jadwal PAGI diatur ke: ");
    Serial.print(state.scheduleHourPagi); Serial.print(":"); Serial.println(state.scheduleMinutePagi);
  }
}

BLYNK_WRITE(V5) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    state.scheduleHourSore = t.getStartHour();
    state.scheduleMinuteSore = t.getStartMinute();
    state.hasActionSore = false;
    Serial.print("Jadwal SORE diatur ke: ");
    Serial.print(state.scheduleHourSore); Serial.print(":"); Serial.println(state.scheduleMinuteSore);
  }
}

BLYNK_WRITE(V1) {
  int value = param.asInt();
  if (value == 1) { 
    cekDanKirimSisaPakan();
  }
}

BLYNK_WRITE(V3) {
  jumlahPutaranPakan = param.asInt();
  Serial.print("Takaran pakan diatur ke: ");
  Serial.println(jumlahPutaranPakan);
}

// BARU: Fungsi untuk kontrol manual Pompa O2
BLYNK_WRITE(V8) {
  int statusTombol = param.asInt(); // Baca status tombol (0 atau 1)
  if (statusTombol == 1) {
    digitalWrite(POMPA_O2_RELAY_PIN, HIGH); // Nyalakan relay
    Serial.println("Pompa O2 Dinyalakan secara manual.");
  } else {
    digitalWrite(POMPA_O2_RELAY_PIN, LOW); // Matikan relay
    Serial.println("Pompa O2 Dimatikan secara manual.");
  }
}

// ------ SETUP & LOOP ------
void setup() {
  Serial.begin(115200);
  Serial.println("Inisialisasi Sistem Kontrol Fuzzy & Blynk (Servo Mode)...");
  WiFi.begin(ssid, pass);
  int wifi_try_count = 0;
  while (WiFi.status() != WL_CONNECTED) { delay(500);
    Serial.print(".");
  if (wifi_try_count++ > 30) { Serial.println("\nGAGAL terhubung WiFi."); while(1); } }
  Serial.println("\nBerhasil terhubung WiFi!");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sensors.begin();
  rtc.begin();
  pakanServo.attach(SERVO_PIN);
  pakanServo.write(POSISI_TUTUP_SERVO);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(PELTIER_RELAY_PIN, OUTPUT);
  pinMode(ULTRASONIC_PAKAN_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_PAKAN_ECHO_PIN, INPUT);
  pinMode(POMPA_O2_RELAY_PIN, OUTPUT);

  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 0);
  digitalWrite(PELTIER_RELAY_PIN, HIGH);
  digitalWrite(POMPA_O2_RELAY_PIN, HIGH);
  
  Blynk.begin(auth, ssid, pass);
  Blynk.syncVirtual(V3, V4, V5, V8);

  timer.setInterval(5000L, ambilDataDanJalankanFuzzy);
  timer.setInterval(120000L, kirimDataKeBlynk);
  timer.setInterval(6000L, cekJadwalPakan);
  
  Serial.println("Sistem Siap Digunakan!");
}

void loop() {
  Blynk.run();
  timer.run();
}