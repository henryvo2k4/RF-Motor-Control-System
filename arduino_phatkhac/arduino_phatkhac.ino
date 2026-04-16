#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(8, 9);  // CE, CSN
const byte address[6] = "00001";

#define VRX1 A0
#define VRY1 A1

#define VRX2 A2
#define VRY2 A3

char sendText[32];

void setup() {
  Serial.begin(9600);

  radio.begin();
//  radio.setChannel(90);   // đổi kênh sang 2490 MHz
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
  Serial.println("Transmitter started...");
}


void loop() {
  int x1 = analogRead(VRX1);
  int y1 = analogRead(VRY1);

  int x2 = analogRead(VRX2);
  int y2 = analogRead(VRY2);

  String message = "A (" + String(x1) + "," + String(y1) + "); ";
  message += "B (" + String(x2) + "," + String(y2) + ")";

  Serial.println("GUI: " + message);
  message.toCharArray(sendText, 32);
  radio.write(sendText, strlen(sendText) + 1);

  delay(200);
}
