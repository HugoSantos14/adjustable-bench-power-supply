#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>

#include "secrets.h"
#include "DHT.h"

#define DHT_PIN 22
#define DHT_TYPE DHT22
#define MQ2_PIN 35

const float TEMP_THRESHOLD = 55.0;
const int SMOKE_THRESHOLD = 700;

const char* SSID = SECRET_SSID;
const char* PASSWORD = SECRET_PASS;
const String WEBHOOK_URL = SECRET_WEBHOOK_URL;

DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastCheckTime = 0;
const long CHECK_INTERVAL = 5000;

bool tempAlertSent = false;
bool smokeAlertSent = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando Fonte de Bancada IoT...");

  dht.begin();
  pinMode(MQ2_PIN, INPUT);

  connectToWifi();
  sendMessageToDiscord("✅ Fonte de Bancada INICIADA. Monitoramento de sensores ativado.");
}

void loop() {
  if (!isConnectedToWifi()) {
    Serial.println("Conexão Wi-Fi perdida. Tentando reconectar...");
    connectToWifi();
  }

  if (millis() - lastCheckTime >= checkInterval) {
    lastCheckTime = millis(); // Reseta o contador de tempo
    checkSensors();
  }
}

void connectToWifi() {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);

  while (!isConnectedToWifi()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

bool isConnectedToWifi() {
  return WiFi.status() == WL_CONNECTED;
}

void checkSensors() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // --- Leitura do DHT (Temp. e umidade) ---
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Falha ao ler o sensor DHT22!");
  } else {
    Serial.printf("[Sensor] Temperatura: %.1f C | Umidade: %.1f %%\n", temp, hum);
    
    if (temp > TEMP_THRESHOLD && !tempAlertSent) {
      String msg = "🔥 ALERTA DE SUPERQUECIMENTO! 🔥\nTemperatura atingiu " + String(temp, 1) + " C. Verifique a fonte!";
      sendMessageToDiscord(msg);
      tempAlertSent = true;
    } else if (temp < (TEMP_THRESHOLD - 5) && tempAlertSent) {
      String msg = "❄️ Temperatura Normalizada.\nNível atual: " + String(temp, 1) + " C.";
      sendMessageToDiscord(msg);
      tempAlertSent = false;
    }
  }

  // --- Leitura do MQ-2 (Fumaça) ---
  int smokeValue = analogRead(MQ2_PIN);
  Serial.printf("[Sensor] Nível de Fumaça (Analógico): %d\n", smokeValue);

  if (smokeValue > SMOKE_THRESHOLD && !smokeAlertSent) {
    String msg = "💨 ALERTA DE FUMAÇA! 💨\nNível do sensor: " + String(smokeValue) + ". Risco de curto-circuito ou incêndio!";
    sendMessageToDiscord(msg);
    smokeAlertSent = true;

  } else if (smokeValue < (SMOKE_THRESHOLD - 100) && smokeAlertSent) {
    String msg = "👍 Nível de fumaça normalizado.\nNível atual: " + String(smokeValue) + ".";
    sendMessageToDiscord(msg);
    smokeAlertSent = false;
  }
}

void sendMessageToDiscord(const String& message) {
  if (!isConnectedToWifi()) {
    Serial.println("Erro: WiFi não conectado. Mensagem não pode ser enviada.");
    return;
  }

  HTTPClient http;
  http.begin(WEBHOOK_URL);
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{\"content\": \"" + message + "\"}";

  Serial.println("Enviando POST para o Discord...");
  Serial.println(jsonPayload);
  int httpCode = http.POST(jsonPayload);

  if (httpCode > 0) {   // significa que recebemos uma resposta do servidor
    if (httpCode == HTTP_CODE_NO_CONTENT) {   // 204
      Serial.println("Mensagem enviada com sucesso ao Discord!");
    } else {
      Serial.printf("[HTTP] Falha no envio, código de resposta: %d\n", httpCode);
      Serial.println("Resposta do servidor: " + http.getString());
    }
  } else {
    Serial.printf("[HTTP] Falha na conexão com o servidor. Erro: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}