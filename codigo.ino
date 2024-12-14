#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_PN532.h>
#include <ambiente.ino> //#define WIFI_SSID WIFI_PASSWORD 

// Definição dos pinos I2C do ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Configuração do PN532 para I2C
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Função para conectar a uma rede Wi-Fi
void connectWiFi(const char* ssid, const char* password) {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // Aguarda a conexão
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) { // 20 tentativas (20 segundos)
    delay(1000);
    Serial.print(".");
    attempt++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao Wi-Fi com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar ao Wi-Fi.");
  }
}
// Função para escanear e obter os valores de RSSI para os SSIDs especificados
void getRSSI(int& rssi1, int& rssi2) {
  rssi1 = 0; // Valores padrão para RSSI
  rssi2 = 0;
  int numNetworks = WiFi.scanNetworks();
  Serial.print("Número de redes encontradas: ");
  Serial.println(numNetworks);
  for (int i = 0; i < numNetworks; ++i) {
    String currentSSID = WiFi.SSID(i);
    int currentRSSI = WiFi.RSSI(i);
    if (currentSSID == SSID1) {
      rssi1 = currentRSSI;
    } else if (currentSSID == SSID2) {
      rssi2 = currentRSSI;
    }
  }
  if (rssi1 == 0 && rssi2 == 0) {
    Serial.println("Nenhuma das redes especificadas foi encontrada no escaneamento.");
  }
}
// Função para calcular a distância com base no RSSI
float calculateDistance(int rssi) {
  if (rssi == 0) {
    return -1; // Retorna -1 para indicar erro (sem valor de RSSI)
  }
  const float A = -50; // RSSI de referência a 1 metro
  const float n = 2.0; // Expoente de perda de caminho (valor típico entre 2 e 4)
  float distance = pow(10, (A - rssi) / (10 * n));
  return distance;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando o leitor RFID (PN532)...");

  // Iniciar comunicação com o PN532
  if (!nfc.begin()) {
    Serial.println("Não foi possível encontrar o PN532. Verifique as conexões.");
    while (1);
  }

    // Conecta ao Wi-Fi
  connectWiFi(SSID1, PASSWORD1);

  // Configurar o leitor para ler apenas tags ISO14443A (RFID/NFC 13.56MHz)
  nfc.SAMConfig();
  Serial.println("PN532 pronto para leitura!");
}

void loop() {

  //TAG RFID
  Serial.println("Aproxime uma tag RFID ou NFC...");

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; // Buffer para o UID
  uint8_t uidLength;                      // Comprimento do UID

  // Tenta ler uma tag
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("Tag detectada!");
    Serial.print("UID da tag: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX); // Imprime o UID em hexadecimal
      Serial.print(" ");
    }
    Serial.println();
    Serial.println("-------------------------");
  } else {
    Serial.println("Nenhuma tag detectada.");
  }

// LOCALIZAÇÃO

    int rssi1 = 0, rssi2 = 0;
  // Escaneia para obter os valores de RSSI
  getRSSI(rssi1, rssi2);
  // Calcula as distâncias
  float distance1 = calculateDistance(rssi1);
  float distance2 = calculateDistance(rssi2);
  // Exibe os resultados
  Serial.println("---- Resultados de RSSI e Distância ----");
  if (rssi1 != 0) {
    Serial.print("RSSI do Roteador 1: ");
    Serial.print(rssi1);
    Serial.print(" dBm, Distância: ");
    Serial.print(distance1);
    Serial.println(" metros");
  } else {
    Serial.println("Roteador 1: Não detectado.");
  }
  if (rssi2 != 0) {
    Serial.print("RSSI do Roteador 2: ");
    Serial.print(rssi2);
    Serial.print(" dBm, Distância: ");
    Serial.print(distance2);
    Serial.println(" metros");
  } else {
    Serial.println("Roteador 2: Não detectado.");
  }

  delay(1000); // Aguarda 1 segundo antes de tentar novamente
}
