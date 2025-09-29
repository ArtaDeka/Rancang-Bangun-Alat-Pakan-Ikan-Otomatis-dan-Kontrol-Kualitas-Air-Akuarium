const int TURBIDITY_PIN = 34;
const float R1 = 10000.0;
const float R2 = 20000.0;
const float VOLTAGE_REFERENCE = 3.3;
const int ADC_RESOLUTION = 4096;
const int NUM_SAMPLES = 100;


// Titik 1: Air Jernih
const float VOLTAGE_JERNIH = 2.0;
const float NTU_JERNIH = 0.0;

// Titik 2: Air Agak Keruh 
const float VOLTAGE_AGAK_KERUH_12 = 1.82; 
const float NTU_AGAK_KERUH_12 = 12.0;

// Titik 3: Air Keruh 
const float VOLTAGE_KERUH_237 = 1.33;  
const float NTU_KERUH_237 = 237.0;

void setup() {
  Serial.begin(115200);
  Serial.println("Membaca Sensor Turbiditas (Kalibrasi Multi-Titik)...");
  Serial.println("Titik 0 NTU diatur pada 2.0V");
}

void loop() {
  float totalRawValue = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    totalRawValue += analogRead(TURBIDITY_PIN);
    delay(1);
  }
  
  float avgRawValue = totalRawValue / NUM_SAMPLES;
  float voltageAtPin = (avgRawValue / (ADC_RESOLUTION - 1)) * VOLTAGE_REFERENCE;
  float sensorVoltage = voltageAtPin * (R1 + R2) / R2;

  float ntu = 0.0;

  // --- LOGIKA KALIBRASI MULTI-TITIK ---
  
  if (sensorVoltage >= VOLTAGE_JERNIH) {
    ntu = 0;

  } else if (sensorVoltage >= VOLTAGE_AGAK_KERUH_12) {
    ntu = fmap(sensorVoltage, VOLTAGE_AGAK_KERUH_12, VOLTAGE_JERNIH, NTU_AGAK_KERUH_12, NTU_JERNIH);

  } else if (sensorVoltage >= VOLTAGE_KERUH_237) {
    ntu = fmap(sensorVoltage, VOLTAGE_KERUH_237, VOLTAGE_AGAK_KERUH_12, NTU_KERUH_237, NTU_AGAK_KERUH_12);
  
  } else {
    ntu = NTU_KERUH_237; 
  }
  
  // Batasi nilai terendah agar tidak menjadi negatif
  if (ntu < 0) {
    ntu = 0;
  }

  // --- TAMPILKAN HASIL ---
  Serial.print("Tegangan Asli Sensor: ");
  Serial.print(sensorVoltage, 3);
  Serial.print(" V");
  Serial.print(" | Estimasi Kekeruhan (Terkalibrasi): ");
  Serial.print(ntu, 2);
  Serial.println(" NTU");

  delay(2000);
}


float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}