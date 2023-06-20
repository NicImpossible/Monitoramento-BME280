#include <HTTPClient.h>
#include <WiFi.h>
#include "string.h"

// Configuração das entradas do LoRa
#define RXD2 16  // Entra no TX
#define TXD2 17  // Entra no RX

// Configuração do WiFi
const char* ssid     = "BrasilNet_Nicole";     // Nome do wifi
const char* password = "@SuperPOWER3265!";    // Senha

//Configuração da planilha
char *server = "script.google.com";         // Server URL
char *GScriptId = "AKfycbyj1vnFOUeUc1kXy9HLIO5emUwvaLCkre6zKFIeUWdY8VW9g-K9W_99rTmp-4cuUY8d";
const int httpsPort = 443;

WiFiClientSecure client;

// Inicialize variáveis para obter e salvar dados LoRa
String temperatura;
String humidade;
String pressao;
String LoRaData;
unsigned char teste;

// Substitui o espaço reservado por valores DHT
String processor(const String& var) {

  if (var == "TEMPERATURA") {
    return temperatura;
  }
  else if (var == "HUMIDADE") {
    return humidade;
  }
  else if (var == "PRESSAO") {
    return pressao;
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  //connect_wifi();

  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);
}

void loop() {
  disableCore0WDT();

  getLoRaData();
/*
  if (teste == 1){
    enviarMedicao();
    teste = 0;
  }*/
}

void connect_wifi(void){

  Serial.print("Conectando-se ao wi-fi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {

    delay(500);
    Serial.print(WiFi.status());
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wi-Fi conectado");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());
}


// Leia o pacote LoRa e obtenha as leituras do sensor
int getLoRaData() {

  if (Serial2.available() > 0) {

    String LoRaData = Serial2.readString();
    Serial.println(LoRaData);

    // Obtenha pressao, temperatura e umidade
    int pos1 = LoRaData.indexOf('/');
    int pos2 = LoRaData.indexOf('&');
    int pos3 = LoRaData.indexOf('#');
    pressao = LoRaData.substring(0, pos1);
    temperatura = LoRaData.substring(pos1 + 1, pos2);
    humidade = LoRaData.substring(pos2 + 1, pos3);

    teste = 1;
  }
}

void enviarMedicao() {
  HTTPClient http;

  String url = String("https://script.google.com") + "/macros/s/" + GScriptId + "/exec?" + "value1=" + pressao + "&value2=" + temperatura + "&value3=" + humidade;

  http.begin(url.c_str()); //Especifica o URL e o certificado
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();   // Verificar erro

  Serial.println("Enviando para dashboard");

  String payload;
  if (httpCode > 0) { //Verifica o código de retorno
    payload = http.getString();

    //Serial.println(httpCode);
    //Serial.println(payload);
  }
  else {
    Serial.println("Erro na solicitação HTTP");
  }

  http.end();

}
