#define ENA 18
#define IN1 2
#define IN2 4

\const int freq = 1000;      // frekuensi PWM
const int pwmChannel = 0;
const int resolution = 8;   

const int KECEPATAN_MAJU = 255; 

void setup() {
  Serial.begin(115200);
  Serial.println("Program Motor Maju Dimulai...");
  
  // Set pin mode
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Setup PWM
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(ENA, pwmChannel);

  // --- Perintah untuk Motor Maju ---
  // Arah putaran maju (IN1 HIGH, IN2 LOW)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Atur kecepatan motor
  ledcWrite(pwmChannel, KECEPATAN_MAJU);

  Serial.print("Motor berputar maju dengan kecepatan: ");
  Serial.println(KECEPATAN_MAJU);
}

void loop() {
  }