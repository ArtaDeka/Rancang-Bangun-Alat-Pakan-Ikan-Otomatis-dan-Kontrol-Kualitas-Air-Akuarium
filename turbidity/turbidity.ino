


/*
 * =================================================================
 * PROGRAM SENSOR TURBIDITAS DENGAN ESP32 & ADS1115 (FINAL)
 * =================================================================
 * * Deskripsi:
 * Program ini membaca nilai analog dari sensor turbiditas secara presisi
 * menggunakan modul ADC eksternal ADS1115.
 * * Wiring (Sesuai Panduan Terakhir):
 * 1. Sensor Vout -> Pembagi Tegangan (10k & 20k)
 * 2. Titik tengah Pembagi Tegangan -> Pin A0 di ADS1115
 * 3. ADS1115 VDD -> 3.3V ESP32
 * 4. ADS1115 GND -> GND
 * 5. ADS1115 SDA -> GPIO 21 ESP32
 * 6. ADS1115 SCL -> GPIO 22 ESP32
 * * Dibuat oleh: Gemini
 * Tanggal: 25 Agustus 2025
 */

#include <Wire.h> // Library ini juga digunakan untuk ADS1115
#include <Adafruit_ADS1X15.h>
// Inisialisasi objek ADS1115
Adafruit_ADS1115 ads;

// --- KONFIGURASI PEMBAGI TEGANGAN ---
// Sesuaikan jika Anda menggunakan nilai resistor yang berbeda
const float R1 = 10000.0; // Resistor dari sensor ke titik tengah
const float R2 = 20000.0; // Resistor dari titik tengah ke GND

// ======================================================================
//  BAGIAN WAJIB UNTUK DIKALIBRASI
// ======================================================================
// Ganti nilai di bawah ini dengan hasil pengukuran Anda sendiri
// untuk mendapatkan hasil yang akurat.
const float TEGANGAN_Jernih = 2.50; // CONTOH: Tegangan saat sensor di air jernih
const float TEGANGAN_Keruh  = 1.0; // CONTOH: Tegangan saat sensor di air keruh

// Anda bisa menentukan sendiri nilai NTU untuk kondisi jernih dan keruh
const int NTU_Jernih = 0;
const int NTU_Keruh = 100; // Contoh nilai NTU untuk air yang sangat keruh
// ======================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nMemulai Program Sensor Turbiditas dengan ADS1115...");

  // Mulai komunikasi I2C dan inisialisasi ADS1115
  if (!ads.begin()) {
    Serial.println("Error: Tidak dapat menemukan ADS1115. Periksa wiring!");
    while (1); // Hentikan program jika sensor tidak terdeteksi
  }

  // Atur Gain Amplifier (PGA)
  // GAIN_ONE memberikan rentang pengukuran +/- 4.096V.
  // Ini adalah pilihan terbaik untuk membaca output pembagi tegangan
  // yang maksimalnya sekitar 3.3V, memberikan resolusi yang baik.
  ads.setGain(GAIN_ONE);
  
  Serial.println("ADS1115 terdeteksi dan siap digunakan.");
  Serial.println("--------------------------------------------------");
  delay(1000);
}

void loop() {
  // Langkah 1: Baca nilai ADC mentah dari ADS1115 (channel A0)
  int16_t adcRaw = ads.readADC_SingleEnded(0);

  // Langkah 2: Konversi nilai mentah ke tegangan di pin A0 ADS1115
  float voltageAtAdc = ads.computeVolts(adcRaw);

  // Langkah 3: Hitung tegangan asli dari sensor sebelum melewati pembagi tegangan
  float sensorVoltage = voltageAtAdc * (R1 + R2) / R2;

  // Langkah 4: Konversi tegangan sensor ke nilai NTU menggunakan kalibrasi (fungsi map)
  // Kita kalikan dengan 1000 agar menjadi integer untuk menjaga presisi saat mapping
  long sensorVoltageInt = sensorVoltage * 1000;
  long teganganJernihInt = TEGANGAN_Jernih * 1000;
  long teganganKeruhInt = TEGANGAN_Keruh * 1000;
  
  float ntu = map(sensorVoltageInt, teganganKeruhInt, teganganJernihInt, NTU_Keruh, NTU_Jernih);

  // Langkah 5: Batasi hasilnya agar tidak menampilkan angka aneh jika di luar rentang kalibrasi
  if (sensorVoltage >= TEGANGAN_Jernih) {
    ntu = NTU_Jernih;
  }
  if (sensorVoltage <= TEGANGAN_Keruh) {
    ntu = NTU_Keruh;
  }

  // Langkah 6: Tampilkan semua informasi ke Serial Monitor
  Serial.print("Nilai ADC Mentah: ");
  Serial.print(adcRaw);
  Serial.print(" | Tegangan Asli Sensor: ");
  Serial.print(sensorVoltage, 3); // Tampilkan 3 desimal untuk presisi
  Serial.print(" V");
  Serial.print(" | Kekeruhan Terkalibrasi: ");
  Serial.print(ntu, 0); // Tampilkan NTU sebagai bilangan bulat
  Serial.println(" NTU");

  delay(2000); // Jeda 2 detik sebelum pembacaan berikutnya
}