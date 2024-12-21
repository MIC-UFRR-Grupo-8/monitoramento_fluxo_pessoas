#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_PN532.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>  // Biblioteca para manipular tempo

FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;

#define WIFI_SSID "OUTRAREDE"
#define WIFI_PASSWORD "12345678"

#define FIREBASE_AUTH "AIzaSyA4bWr1rJAmBmUhR66Pbbnj4wocw16T4n8"
#define FIREBASE_HOST "iot2024-c31d1-default-rtdb.firebaseio.com"

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// NTP Setup
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000);  // Servidor NTP e intervalo de atualização (em milissegundos)

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
  float xx1 = 0.0, yy1 = 0.0;
  float xx2 = 5.0, yy2 = 0.0;
  float xx3 = 2.5, yy3 = 4.33;

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
  if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("Coordenadas enviadas ao Firebase!");
  } else {
    Serial.print("Erro ao enviar dados: ");
    Serial.println(fbdo.errorReason());
  }
}

// Função para obter a data e hora no formato desejado
String getDateTime() {
  // Atualiza o tempo
  timeClient.update();

  // Definir o tempo do NTP na biblioteca TimeLib
  setTime(timeClient.getEpochTime());

  // Ajuste o horário para o fuso horário de Roraima (UTC-4)
  adjustTime(-4 * 3600);  // Subtrai 4 horas (Roraima está UTC-4)

  // Formata a data e hora como "dd/MM/yyyy HH:mm:ss"
  char dateTimeStr[20];
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d/%02d/%04d %02d:%02d:%02d", 
           day(), month(), year(),
           hour(), minute(), second());
  
  return String(dateTimeStr);
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
    while (1)
      ;
  }

  nfc.SAMConfig();

  //---------------------
  connectWiFi();
  // Configurações do Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  // Inicializa o Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Iniciar o cliente NTP
  timeClient.begin();

  if (!Firebase.ready()) {
    Serial.println("Falha ao conectar ao Firebase!");
  } else {
    Serial.println("Conectado ao Firebase!");
  }

  delay(100);
}

void loop() {
  //TAG RFID
  Serial.println("Aproxime uma tag RFID...");

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer para o UID
  uint8_t uidLength;                        // Comprimento do UID
  String tagUID = "";

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("Tag detectada!");
    Serial.print("UID da tag: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) tagUID += "0";      // Adiciona zero à esquerda para números menores que 16
      tagUID += String(uid[i], HEX);         // Converte o byte para hexadecimal
      if (i < uidLength - 1) tagUID += ":";  // Adiciona separador ":" entre os bytes
    }
    Serial.print("UID para enviar: ");
    Serial.println(tagUID);
    Serial.println();
    Serial.println("-------------------------");

    // Obtemos a data e hora ajustada para o horário de Roraima
    String dateTime = getDateTime();

    // Envia o UID e a data/hora para o Firebase (usando push para não sobrescrever)
    FirebaseJson json;
    json.set("uid", tagUID);
    json.set("timestamp", dateTime);
    
    // Usando push para adicionar um novo nó em vez de sobrescrever
    if (Firebase.RTDB.pushJSON(&fbdo, "/rfid_tag", &json)) {
      Serial.println("UID e Data/Hora enviados ao Firebase com sucesso!");
    } else {
      Serial.print("Erro ao enviar dados ao Firebase: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // LOCALIZAÇÃO
  int rssi1 = 0, rssi2 = 0, rssi3 = 0;
  float d1 = 0, d2 = 0, d3 = 0;

  // Escaneia RSSIs dos três roteadores
  WiFi.scanNetworks();  // Requer chamado prévio para inicializar a varredura
  getRSSI("OUTRAREDE1", rssi1);
  getRSSI("OUTRAREDE2", rssi2);
  getRSSI("OUTRAREDE3", rssi3);
}
