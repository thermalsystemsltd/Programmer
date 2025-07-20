void setup() {
  Serial.begin(115200);
  Serial.println("Test sketch loaded successfully!");
}

void loop() {
  delay(1000);
  Serial.println("Hello from ESP8266!");
} 