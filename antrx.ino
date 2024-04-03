#include <esp_now.h>
#include <WiFi.h>

const int AIN1 = 0;
const int AIN2 = 1;
const int V_SENSE = 3;
const int BIN1 = 4;
const int BIN2 = 5;
const int servoPin = 10;

uint8_t broadcastAddress[] = {0xF4, 0x12, 0xFA, 0x59, 0xDD, 0x50};

int incomingA;
int incomingB;
int sesrvo;

int outgoingbat;

unsigned long currentMillis = millis();

typedef struct MotorData {
  int a;
  int b;
  int c;
} MotorData;

typedef struct BatteryCheck {
  int batt;
} BatteryCheck;

MotorData motors;
BatteryCheck battery;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&motors, incomingData, sizeof(motors));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingA = motors.a;
  incomingB = motors.b;
  sesrvo = motors.c;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(V_SENSE, INPUT);
  pinMode(servoPin, OUTPUT);
  Serial.begin(115200);
  ledcSetup(0, 2000, 12);
  ledcSetup(1, 2000, 12);
  ledcSetup(2, 2000, 12);
  ledcSetup(3, 2000, 12);
  ledcSetup(4, 330, 8);
  ledcAttachPin(AIN1, 0);
  ledcAttachPin(AIN2, 1);
  ledcAttachPin(BIN1, 2);
  ledcAttachPin(BIN2, 3);
  ledcAttachPin(10, 4);
  //servo likes the range 87 - 140.
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  int A_speed = map(incomingA, 0, 16000, 3200, -3200);
  if (A_speed < 500 && A_speed > -500) {
    A_speed = 0;
  }
  if (A_speed > 4095) {
    A_speed = 4095;
  }
  if (A_speed < -4095) {
    A_speed = -4095;
  }
  if (A_speed > 0) {
    ledcWrite(0, abs(A_speed));
    ledcWrite(1, 0);
  } else {
    ledcWrite(1, abs(A_speed));
    ledcWrite(0, 0);  
  }

  int B_speed = map(incomingB, 0, 16000, 4095, -4095);
  if (B_speed < 500 && B_speed > -500) {
    B_speed = 0;
  }
  if (B_speed > 4095) {
    B_speed = 4095;
  }
  if (B_speed < -4095) {
    B_speed = -4095;
  }
  if (B_speed > 0) {
    ledcWrite(2, abs(B_speed));
    ledcWrite(3, 0);
  } else {
    ledcWrite(3, abs(B_speed));
    ledcWrite(2, 0);  
  }

  if (sesrvo == 0) {
    ledcWrite(4, 140);
  } else {
    ledcWrite(4, 89);
  }

  if (millis() - currentMillis > 1000) {
    battery.batt = map(analogRead(V_SENSE), 1673, 2067, 0, 100);

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &battery, sizeof(battery));
    
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    currentMillis = millis();
  }
}