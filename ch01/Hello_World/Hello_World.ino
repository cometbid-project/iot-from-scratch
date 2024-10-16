const byte led_gpio = 2;
void setup() {
  // put your setup code here, to run once:
  pinMode(led_gpio, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(led_gpio, HIGH);
  delay(500);
  digitalWrite(led_gpio, LOW);
  delay(500);
}