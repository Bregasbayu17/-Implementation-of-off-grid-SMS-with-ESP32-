#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

#include <SPI.h>
#include <LoRa.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define ss 18
#define rst 23
#define dio0 26

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const byte ROWS = 4; 
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {14, 12, 13, 15};
byte colPins[COLS] = {02, 00, 04, 25};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool connected = false;
unsigned long lastPingTime = 0;
String lastReceivedMessage = "";

void setup() {
  Serial.begin(115200);
  
  while (!Serial);
  Serial.println("LoRa Duplex Communication");
  SPI.begin(5, 19, 27, 18);
  LoRa.setSPI(SPI);
  LoRa.setPins(ss, rst, dio0);
  
  while(!LoRa.begin(433E6)){
    Serial.println(".");
    delay(500);
  }
  
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initialized");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Gagal menginisialisasi OLED");
    while (1);
  }
  display.clearDisplay();
  display.display();
}

String typedMessage = "";
const char* t9[] = {"0 ", ".,?!1", "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9"};
char lastKey = '\0';
unsigned long lastPressTime = 0;
int keyIndex = 0;
const int maxMessageLength = 30;

void loop() {
  char key = keypad.getKey();
  if (key) {
    unsigned long now = millis();
    if (key == lastKey && now - lastPressTime < 800) {
      keyIndex = (keyIndex + 1) % strlen(t9[key - '0']);
      typedMessage[typedMessage.length() - 1] = t9[key - '0'][keyIndex];
    } else {
      if (key >= '0' && key <= '9' && strlen(t9[key - '0']) > 0) {
        if (typedMessage.length() < maxMessageLength) {
          typedMessage += t9[key - '0'][0];
          keyIndex = 0;
        }
      } else if (key == 'A') {
        Serial.println("Pesan terkirim: " + typedMessage);
        LoRa.beginPacket();
        LoRa.print(typedMessage);
        LoRa.endPacket();
        typedMessage = "";
      } else if (key == '*') {
        typedMessage = "";
      } else if (key == 'D' ){
        if (!typedMessage.isEmpty()) {
          typedMessage.remove(typedMessage.length() - 1);
        }
      }
    }
    lastPressTime = now;
    lastKey = key;
    displayMessage();
  }
  receiveMessage();
  sendPing();
}

void sendPing() {
  if (millis() - lastPingTime > 5000) {
    LoRa.beginPacket();
    LoRa.print("PING");
    LoRa.endPacket();
    lastPingTime = millis();
  }
}

void receiveMessage() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }
    
    if (receivedMessage == "PING") {
      connected = true;
      Serial.println("ESP lain terkoneksi");
    } else {
      Serial.println("Pesan diterima: " + receivedMessage);
      lastReceivedMessage = receivedMessage;
    }
    displayMessage();
  }
}

void displayMessage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  if (connected) {
    display.println("Status: Terkoneksi");
  } else {
    display.println("Status: Tidak terkoneksi");
  }
  display.println("Terima: " + lastReceivedMessage);
  display.println("Ketik: " + typedMessage);
 
  display.display();
}

