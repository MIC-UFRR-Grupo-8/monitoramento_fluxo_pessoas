#include <WiFi.h>
#include <math.h>
//#include <ambiente.ino> //#define WIFI_SSID WIFI_PASSWORD 

// Coordenadas dos pontos de referência
const float xx1 = 0.0, yy1 = 0.0;   // Ponto 1
const float xx2 = 5.0, yy2 = 0.0;   // Ponto 2
const float xx3 = 2.5, yy3 = 4.33;  // Ponto 3 (triângulo equilátero)

// Dados da rede Wi-Fi (para escanear os pontos de referência)
const char* SSID1 = "OUTRAREDE";
const char* SSID2 = "OUTRAREDE2";
const char* SSID3 = "OUTRAREDE3";

void getRSSI(const char* ssid, int& rssi) {
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    if (WiFi.SSID(i) == ssid) {
      rssi = WiFi.RSSI(i);
      return;
    }
  }
  rssi = -100; // RSSI muito baixo se a rede não for encontrada
}

float calculateDistance(int rssi) {
  float A = -50;  // RSSI de referência a 1 metro
  float n = 2.5;  // Expoente de perda de sinal
  return pow(10, (A - rssi) / (10 * n));
}

void trilateration(float d1, float d2, float d3, float& x, float& y) {
  float A = 2 * (xx2 - xx1);
  float B = 2 * (yy2 - yy1);
  float C = pow(d1, 2) - pow(d2, 2) - pow(xx1, 2) + pow(xx2, 2) - pow(yy1, 2) + pow(yy2, 2);
  float D = 2 * (xx3 - xx1);
  float E = 2 * (yy3 - yy1);
  float F = pow(d1, 2) - pow(d3, 2) - pow(xx1, 2) + pow(xx3, 2) - pow(yy1, 2) + pow(yy3, 2);

  x = (C * E - F * B) / (E * A - B * D);
  y = (C * D - A * F) / (B * D - A * E);
}

void setup() {
  Serial.begin(115200);

  // Conecte-se ao Wi-Fi, se necessário
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Desconecta para iniciar o escaneamento
  delay(100);
}

void loop() {
  int rssi1 = 0, rssi2 = 0, rssi3 = 0;

  // Escaneia RSSIs dos três roteadores
  WiFi.scanNetworks(); // Requer chamado prévio para inicializar a varredura
  getRSSI(SSID1, rssi1);
  getRSSI(SSID2, rssi2);
  getRSSI(SSID3, rssi3);

  // Calcula distâncias
  float d1 = calculateDistance(rssi1);
  float d2 = calculateDistance(rssi2);
  float d3 = calculateDistance(rssi3);

  // Calcula localização usando trilateração
  float xx = 0.0, yy = 0.0;
  trilateration(d1, d2, d3, xx, yy);

  // Exibe a localização no console
  Serial.print("Localização estimada: x = ");
  Serial.print(xx);
  Serial.print(", y = ");
  Serial.println(yy);

  delay(2000);
}
