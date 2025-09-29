#include <OneWire.h>
#include <DallasTemperature.h>

const int oneWireBus = 23;

OneWire oneWire(oneWireBus);

DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  
  sensors.begin();
  
  Serial.println("\n===== Program Uji Coba Sensor Suhu DS18B20 =====");
}

void loop() {
  Serial.print("Meminta pembacaan suhu...");
  sensors.requestTemperatures(); 
  Serial.println(" Selesai.");
  

  float tempC = sensors.getTempCByIndex(0);

  float tempF = sensors.getTempFByIndex(0);

  if(tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Gagal membaca suhu dari sensor!");
    Serial.println("Pastikan koneksi dan resistor pull-up sudah benar.");
  } else {
    Serial.print("Suhu: ");
    Serial.print(tempC);
    Serial.print("C");

    Serial.print("  |  ");

    Serial.print(tempF);
    Serial.println("F");
  }

  delay(2000);
}