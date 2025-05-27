#include <WiFi.h>
#include <PubSubClient.h>

// Pin Config - according to your hardware
#define LED1 18
#define LED2 19
#define LED3 21
#define BTN_MANUAL 5    // Only one manual button controlling LED3

const char* ssid = "TOPNET_C738";
const char* password = "id98or68qa";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// LED states
bool led1State = false;
bool led2State = false;
bool led3State = false;

// Button debounce vars
bool lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Connected!");
}

void updateLED(int id) {
  bool state = false;
  if (id == 1) state = led1State;
  if (id == 2) state = led2State;
  if (id == 3) state = led3State;

  int pin = (id == 1) ? LED1 : (id == 2) ? LED2 : LED3;
  digitalWrite(pin, state ? HIGH : LOW);
}

void publishState(int id, bool state) {
  const char* topic = (id == 1) ? "smartHome/led1" : (id == 2) ? "smartHome/led2" : "smartHome/led3";
  client.publish(topic, state ? "ON" : "OFF");
}

void toggleLED(int id) {
  if (id == 1) {
    led1State = !led1State;
    updateLED(1);
    publishState(1, led1State);
  } else if (id == 2) {
    led2State = !led2State;
    updateLED(2);
    publishState(2, led2State);
  } else if (id == 3) {
    led3State = !led3State;
    updateLED(3);
    publishState(3, led3State);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  if (String(topic) == "smartHome/led1") {
    led1State = (msg == "ON");
    updateLED(1);
  } else if (String(topic) == "smartHome/led2") {
    led2State = (msg == "ON");
    updateLED(2);
  } else if (String(topic) == "smartHome/led3") {
    led3State = (msg == "ON");
    updateLED(3);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("esp32_client")) {
      Serial.println(" connected!");
      client.subscribe("smartHome/led1");
      client.subscribe("smartHome/led2");
      client.subscribe("smartHome/led3");
    } else {
      Serial.print(" failed. Retry in 5 sec\n");
      delay(5000);
    }
  }
}

void checkManualButton() {
  int currBtnState = digitalRead(BTN_MANUAL);

  if (currBtnState != lastBtnState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currBtnState == LOW && lastBtnState == HIGH) {  // Button pressed (active LOW)
      toggleLED(3);
      delay(50); // small delay to avoid fast toggling
    }
  }

  lastBtnState = currBtnState;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(BTN_MANUAL, INPUT_PULLUP);  // Button with pull-up resistor

  // Initialize LEDs OFF
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  checkManualButton();
}



