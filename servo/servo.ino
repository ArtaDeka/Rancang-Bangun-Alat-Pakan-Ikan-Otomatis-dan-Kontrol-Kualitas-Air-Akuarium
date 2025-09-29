#include <ESP32Servo.h>

Servo myservo;

const int servoPin = 25;

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myservo.attach(servoPin); 

  Serial.println("Program Pengujian Servo Dimulai!");
}

void loop() {
  Serial.println("Bergerak ke posisi 0 derajat...");
  myservo.write(0);   // Perintahkan servo untuk bergerak ke sudut 0 derajat
  delay(1500);        // Beri jeda 1.5 detik agar servo sempat mencapai posisi

  Serial.println("Bergerak ke posisi 90 derajat...");
  myservo.write(90);  // Perintahkan servo untuk bergerak ke sudut 90 derajat (posisi tengah)
  delay(1500);        // Jeda 1.5 detik

  Serial.println("Bergerak ke posisi 180 derajat...");
  myservo.write(180); // Perintahkan servo untuk bergerak ke sudut 180 derajat
  delay(2000);        // Jeda 2 detik sebelum memulai pengujian berikutnya

}