#include <OneWire.h>
#include <DallasTemperature.h>

//========= 1. DEFINISI PIN HARDWARE =========
const int ONE_WIRE_BUS_PIN  = 23;
const int TURBIDITY_PIN     = 34;
const int ENA_PIN           = 18;
const int IN1_PIN           = 2;
const int IN2_PIN           = 4;
const int PELTIER_RELAY_PIN = 16;
const int MOTOR_RELAY_PIN = 27; 

//========= 2. PENGATURAN DAN KALIBRASI =========
// Sensor Suhu
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);

// Sensor Turbidity
const float R1 = 10000.0;
const float R2 = 20000.0;
const float TEGANGAN_Jernih = 2.0;
const float TEGANGAN_Keruh  = 0.0;
const int NTU_Jernih = 0;
const int NTU_Keruh = 100;

// Pengaturan Termostat Peltier
const float SUHU_PELTIER_ON  = 25.0; // Peltier NYALA jika suhu >= nilai ini
const float SUHU_PELTIER_OFF = 24.0; // Peltier MATI jika suhu <= nilai ini

//========= 3. DEFINISI PARAMETER FUZZY =========
// Parameter Himpunan Input Suhu
const float SUHU_DINGIN[] = {15, 15, 18, 21};
const float SUHU_NORMAL[] = {19, 23, 25};
const float SUHU_PANAS[]  = {24, 27, 32, 32};

// Parameter Himpunan Input Kekeruhan
const float KEKERUHAN_JERNIH[] = {0, 0, 10, 25};
const float KEKERUHAN_SEDANG[] = {15, 35, 55};
const float KEKERUHAN_KERUH[]  = {45, 60, 100, 100};

// Parameter Himpunan Output Motor-Filter
const float MOTOR_LAMBAT[] = {0, 30, 50};
const float MOTOR_SEDANG[] = {40, 60, 80};
const float MOTOR_CEPAT[]  = {70, 80, 100, 100};

// Variabel state untuk menyimpan hasil kalkulasi fuzzy
struct FuzzyState {
  float suhuSaatIni;
  float kekeruhanSaatIni;
  
  // Derajat keanggotaan input
  struct {
    float dingin, normal, panas;
  } suhu;

  struct {
    float jernih, sedang, keruh;
  } kekeruhan;

  // Kekuatan aturan (firing strengths)
  float aturan[10]; // aturan[0] tidak dipakai, mulai dari 1-9

  // Hasil akhir
  float outputKecepatanMotor;
  int pwmMotor;
};

FuzzyState state;

//========= SETUP =========
void setup() {
  Serial.begin(115200);
  Serial.println("Sistem Kontrol Kualitas Air Akuarium Dimulai...");

  sensors.begin();
  
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(PELTIER_RELAY_PIN, OUTPUT);
  pinMode(MOTOR_RELAY_PIN, OUTPUT);

  // Inisialisasi aktuator ke kondisi mati
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 0);
  digitalWrite(PELTIER_RELAY_PIN, HIGH); // HIGH = MATI untuk relay Active LOW
  digitalWrite(MOTOR_RELAY_PIN, HIGH);
}

//========= LOOP = DIJALANKAN BERULANG-ULANG =========
void loop() {
  // Langkah 1: Baca semua sensor
  state.suhuSaatIni = bacaSuhu();
  state.kekeruhanSaatIni = bacaKekeruhan();
  
  if (state.suhuSaatIni == -127.0) {
    Serial.println("Gagal membaca sensor suhu, skip loop...");
    delay(5000);
    return;
  }

  // Langkah 2: Jalankan Mesin Logika Fuzzy
  fuzzifikasi();
  inferensi();
  defuzzifikasiCentroid(); // Menggunakan metode Centroid

  // Langkah 3: Tentukan aksi untuk aktuator
  kontrolAktuator();

  // Langkah 4: Tampilkan data ke Serial Monitor
  tampilkanDataSerial();
  
  delay(2000);
}

/****************************************************************
 * FUNGSI-FUNGSI
 ****************************************************************/

// --- Fungsi Baca Sensor ---
float bacaSuhu() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if(tempC == DEVICE_DISCONNECTED_C) return -127.0;
  return tempC;
}

float bacaKekeruhan() {
  int adcRaw = analogRead(TURBIDITY_PIN);
  float voltageAtPin = (adcRaw / 4095.0) * 3.3;
  float sensorVoltage = voltageAtPin * (R1 + R2) / R2;
  
  // Konversi tegangan ke NTU
  long sensorVoltageInt = sensorVoltage * 1000;
  long teganganJernihInt = TEGANGAN_Jernih * 1000;
  long teganganKeruhInt = TEGANGAN_Keruh * 1000;
  
  float ntu = map(sensorVoltageInt, teganganKeruhInt, teganganJernihInt, NTU_Keruh, NTU_Jernih);
  
  // Batasi nilai ntu antara 0 dan 100
  return constrain(ntu, 0, 100);
}

// --- Fungsi Himpunan Keanggotaan ---
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

// --- Fungsi Inti Logika Fuzzy ---
void fuzzifikasi() {
  state.suhu.dingin = trapmf(state.suhuSaatIni, SUHU_DINGIN);
  state.suhu.normal = trimf(state.suhuSaatIni, SUHU_NORMAL);
  state.suhu.panas  = trapmf(state.suhuSaatIni, SUHU_PANAS);

  state.kekeruhan.jernih = trapmf(state.kekeruhanSaatIni, KEKERUHAN_JERNIH);
  state.kekeruhan.sedang = trimf(state.kekeruhanSaatIni, KEKERUHAN_SEDANG);
  state.kekeruhan.keruh  = trapmf(state.kekeruhanSaatIni, KEKERUHAN_KERUH);
}

void inferensi() {
  state.aturan[1] = min(state.suhu.dingin, state.kekeruhan.jernih); // Output Lambat
  state.aturan[2] = min(state.suhu.dingin, state.kekeruhan.sedang); // Output Sedang
  state.aturan[3] = min(state.suhu.dingin, state.kekeruhan.keruh);  // Output Cepat
  state.aturan[4] = min(state.suhu.normal, state.kekeruhan.jernih); // Output Lambat
  state.aturan[5] = min(state.suhu.normal, state.kekeruhan.sedang); // Output Sedang
  state.aturan[6] = min(state.suhu.normal, state.kekeruhan.keruh);  // Output Cepat
  state.aturan[7] = min(state.suhu.panas,  state.kekeruhan.jernih); // Output Sedang
  state.aturan[8] = min(state.suhu.panas,  state.kekeruhan.sedang); // Output Cepat
  state.aturan[9] = min(state.suhu.panas,  state.kekeruhan.keruh);  // Output Cepat
}

void defuzzifikasiCentroid() {
  float pembilang = 0, penyebut = 0;
  
  // Agregasi kekuatan aturan untuk setiap kategori output
  float motor_lambat_strength = max(state.aturan[1], state.aturan[4]);
  float motor_sedang_strength = max(max(state.aturan[2], state.aturan[5]), state.aturan[7]);
  float motor_cepat_strength  = max(max(state.aturan[3], state.aturan[6]), max(state.aturan[8], state.aturan[9]));
  
  // Diskritisasi: Lakukan sampling pada rentang output (0-100)
  for (int y = 0; y <= 100; y++) {
    // Dapatkan derajat keanggotaan titik y pada setiap himpunan output
    float mu_lambat = trimf(y, MOTOR_LAMBAT);
    float mu_sedang = trimf(y, MOTOR_SEDANG);
    float mu_cepat  = trapmf(y, MOTOR_CEPAT);
    
    // Terapkan hasil inferensi (clipping)
    float mu_clipped_lambat = min(motor_lambat_strength, mu_lambat);
    float mu_clipped_sedang = min(motor_sedang_strength, mu_sedang);
    float mu_clipped_cepat  = min(motor_cepat_strength, mu_cepat);
    
    // Agregasi: Ambil nilai maksimum dari semua himpunan output yang terpotong
    float mu_aggregated = max(max(mu_clipped_lambat, mu_clipped_sedang), mu_clipped_cepat);
    
    // Lakukan penjumlahan untuk formula centroid
    pembilang += y * mu_aggregated;
    penyebut  += mu_aggregated;
  }
  
  if (penyebut == 0) {
    state.outputKecepatanMotor = 0;
  } else {
    state.outputKecepatanMotor = pembilang / penyebut;
  }
}

// --- Fungsi Kontrol & Tampilan ---
void kontrolAktuator() {
  // --- Kontrol Motor (Berdasarkan Hasil Fuzzy) ---
  state.pwmMotor = map(state.outputKecepatanMotor, 0, 100, 0, 255);
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, state.pwmMotor);
  
  // --- Kontrol Peltier (Logika Termostat dengan Hysteresis) ---
  if (state.suhuSaatIni >= SUHU_PELTIER_ON) {
    digitalWrite(PELTIER_RELAY_PIN, LOW); // LOW = NYALA
  }
  else if (state.suhuSaatIni <= SUHU_PELTIER_OFF) {
    digitalWrite(PELTIER_RELAY_PIN, HIGH); // HIGH = MATI
  }
  // Jika suhu berada di antara OFF dan ON, status relay tidak berubah (hysteresis)
}

void tampilkanDataSerial() {
  Serial.println("---------------------------------");
  Serial.print("Suhu Saat Ini: "); Serial.print(state.suhuSaatIni, 2); Serial.println(" C");
  Serial.print("Kekeruhan Saat Ini (0-100): "); Serial.print(state.kekeruhanSaatIni, 2); Serial.println(" NTU");
  Serial.println();
  Serial.println("--- Hasil Fuzzy ---");
  Serial.print("Output Kecepatan Motor: "); Serial.println(state.outputKecepatanMotor);
  Serial.println();
  Serial.println("--- Aksi Aktuator ---");
  Serial.print("PWM Motor Dikirim: "); Serial.println(state.pwmMotor);
  Serial.print("Status Relay Peltier: ");
  Serial.println(digitalRead(PELTIER_RELAY_PIN) == LOW ? "NYALA" : "MATI");
   Serial.print("Status Relay Motor: "); 
  Serial.println(digitalRead(MOTOR_RELAY_PIN) == LOW ? "NYALA" : "MATI");
  Serial.println("---------------------------------");
}