// digital pin 2 has a pushbutton attached to it (not used):
int pushButton = 2;
// digital pin 3 has an LED attached to it:
int ledPin = 3;
// analog pin A0 has a potentiometer:
int potPin = A0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // make the pushbutton's pin an input:
  pinMode(pushButton, INPUT_PULLUP);
  // make the LED's pin an output:
  pinMode(ledPin, OUTPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the potentiometer value (0-1023):
  int potValue = analogRead(potPin);
  
  // map potentiometer value to PWM range (0-255):
  int brightness = map(potValue, 0, 1023, 0, 255);
  
  // set LED brightness with PWM:
  analogWrite(ledPin, brightness);
  
  // print potentiometer value to Serial:
  Serial.print("Potenciometro: ");
  Serial.print(potValue);
  Serial.print(" | Brillo LED: ");
  Serial.println(brightness);
  
  delay(10);  // small delay for stability
}