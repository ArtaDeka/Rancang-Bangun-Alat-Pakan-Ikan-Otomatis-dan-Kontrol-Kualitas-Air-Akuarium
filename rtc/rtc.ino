#include <Wire.h>
#include <RTClib.h>

// Buat objek RTC
RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};

void setup() {
  Serial.begin(115200);
  
  delay(2000); 

  // Inisialisasi I2C (untuk RTC)
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("Tidak dapat menemukan modul RTC DS3231! Periksa koneksi.");
    Serial.flush();
    while (1); // Hentikan program jika RTC tidak ditemukan
  }
  Serial.println("Modul RTC DS3231 ditemukan.");
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  }

void loop() {
  DateTime now = rtc.now();
  
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(")");
  
  Serial.print(" ");
  if(now.hour() < 10) Serial.print('0'); // Tambah '0' di depan jika jam < 10
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10) Serial.print('0'); // Tambah '0' di depan jika menit < 10
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10) Serial.print('0'); // Tambah '0' di depan jika detik < 10
  Serial.print(now.second(), DEC);
  Serial.println();

    Serial.print("Suhu internal modul: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");
  Serial.println("----------------------------------------------");

  delay(1000);
}