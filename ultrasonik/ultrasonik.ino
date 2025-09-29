#define TRIGGER_PIN 12  // 
#define ECHO_PIN    14 // 
long duration;
float distanceCm;

void setup() {
  Serial.begin(115200);

  // Atur mode pin
  pinMode(TRIGGER_PIN, OUTPUT); // 
  pinMode(ECHO_PIN, INPUT);   // 
}

void loop() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);

    digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
  distanceCm = duration * 0.0343 / 2;

  Serial.print("Jarak: ");
  Serial.print(distanceCm);
  Serial.println(" cm");

  delay(5000);
}