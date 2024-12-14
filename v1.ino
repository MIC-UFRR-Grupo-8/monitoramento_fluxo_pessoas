#include <WiFi.h>
#include <ambiente.ino> //#define WIFI_SSID WIFI_PASSWORD 


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

  // Conecta ao Wi-Fi
  connectWiFi(SSID1, PASSWORD1);
}

void loop() {
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

  delay(5000); // Atraso de 5 segundos entre as varreduras
}
