#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"

const char* SSID = SECRET_SSID;
const char* PASSWORD = SECRET_PASS;
const String WEBHOOK_URL = SECRET_WEBHOOK_URL;

bool isConnectedToWifi() {
  return WiFi.status() == WL_CONNECTED;
}

void sendMessageToDiscord(const String& message) {

  if (isConnectedToWifi()) {

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
        String response = http.getString();
        Serial.println("Resposta do servidor:");
        Serial.println(response);
      }
    } else {
      Serial.printf("[HTTP] Falha na conexão com o servidor. Erro: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

  } else {
    Serial.println("Erro: WiFi não está conectado.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

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

  sendMessageToDiscord("Olá, Discord! O ESP32 está online e funcionando. :rocket:");
}

void loop() {
  delay(30000);
}
