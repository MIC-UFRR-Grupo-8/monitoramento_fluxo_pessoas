//#include <Wire.h>
#include <WiFi.h>
#include <math.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_PN532.h>
#include <ambiente.ino>  //#define WIFI_SSID WIFI_PASSWORD

// Objeto Firebase
FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;

// Definição dos pinos I2C do ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Configuração do PN532 para I2C
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Coordenadas dos pontos de referência
const float xx1 = 0.0, yy1 = 0.0;   // Ponto 1
const float xx2 = 5.0, yy2 = 0.0;   // Ponto 2
const float xx3 = 2.5, yy3 = 4.33;  // Ponto 3 (triângulo equilátero)

void getRSSI(const char* ssid, int& rssi) {
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    if (WiFi.SSID(i) == ssid) {
      rssi = WiFi.RSSI(i);
      return;
    }
  }
  rssi = -100;  // RSSI muito baixo se a rede não for encontrada
}

float calculateDistance(int rssi) {
  float A = -50;  // RSSI de referência a 1 metro
  float n = 2.5;  // Expoente de perda de sinal
  return pow(10, (A - rssi) / (10 * n));
}

void trilateration(float d1, float d2, float d3, float& xx, float& yy) {
  float A = 2 * (xx2 - xx1);
  float B = 2 * (yy2 - yy1);
  float C = pow(d1, 2) - pow(d2, 2) - pow(xx1, 2) + pow(xx2, 2) - pow(yy1, 2) + pow(yy2, 2);
  float D = 2 * (xx3 - xx1);
  float E = 2 * (yy3 - yy1);
  float F = pow(d1, 2) - pow(d3, 2) - pow(xx1, 2) + pow(xx3, 2) - pow(yy1, 2) + pow(yy3, 2);

  // Calcula as coordenadas
  xx = (C * E - F * B) / (E * A - B * D);
  yy = (C * D - A * F) / (B * D - A * E);

  // Envia as coordenadas ao Firebase
  String path = "/location";
  FirebaseJson json; 
  json.set("x", xx); 
  json.set("y", yy);
 // String json = String("{\"x\":") + String(xx) + ",\"y\":" + String(yy) + "}";
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("Coordenadas enviadas ao Firebase!");
  } else {
    Serial.print("Erro ao enviar dados: ");
    Serial.println(fbdo.errorReason());
  }
}

// Conexão Wi-Fi
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("WiFi conectado!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando o leitor RFID (PN532)...");

  // Iniciar comunicação com o PN532
  if (!nfc.begin()) {
    Serial.println("Não foi possível encontrar o PN532. Verifique as conexões.");
    while (1);
  }

  // Configurar o leitor para ler apenas tags ISO14443A (RFID/NFC 13.56MHz)
  nfc.SAMConfig();

  //---------------------
  connectWiFi();
  // Configurações do Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;  
  // Inicializa o Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(100);
}

void loop() {

  //TAG RFID
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer para o UID
  uint8_t uidLength;                        // Comprimento do UID

  // Tenta ler uma tag
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("Tag detectada!");
    Serial.print("UID da tag: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);  // Imprime o UID em hexadecimal
      Serial.print(" ");
    }
    Serial.println();
    Serial.println("-------------------------");
  } 

  // LOCALIZAÇÃO
  int rssi1 = 0, rssi2 = 0, rssi3 = 0;
  float d1 = 0, d2 = 0, d3 = 0;

  // Escaneia RSSIs dos três roteadores
  WiFi.scanNetworks();  // Requer chamado prévio para inicializar a varredura
  getRSSI(SSID1, rssi1);
  getRSSI(SSID2, rssi2);
  getRSSI(SSID3, rssi3);
  // Calcula distâncias
  d1 = calculateDistance(rssi1);
  d2 = calculateDistance(rssi2);
  d3 = calculateDistance(rssi3);
  // Calcula localização usando trilateração
  float xx = 0.0, yy = 0.0;
  trilateration(d1, d2, d3, xx, yy);
  // Exibe a localização no console
  Serial.print("Localização estimada: x = ");
  Serial.print(xx);
  Serial.print(", y = ");
  Serial.println(yy);

  delay(1000);  // Aguarda 1 segundo antes de tentar novamente
}
