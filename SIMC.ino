#include "Wire.h"
#include "Defines.h"
#include "DHTesp.h"
#include "LiquidCrystal_I2C.h"
#include "NTPClient.h"
#include "PMS.h"
#include "RTClib.h"
#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "WiFiUdp.h"
#include "ThingSpeak.h"
#include "UniversalTelegramBot.h"

PMS pms(Serial2);
PMS::DATA dadosPMS;
DHTesp sensor;
TempAndHumidity tempUmidade;
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiUDP udpClient;
NTPClient timeClient(udpClient, "pool.ntp.org", UTC_OFFSET);
WiFiClient apiClient;
WiFiClientSecure telegramClient;
UniversalTelegramBot bot(SECRET_TG_TOKEN, telegramClient);

unsigned long agora = 0;

String statusUpdate = "";
String statusRisco = "";

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(TESTE, OUTPUT);
  digitalWrite(TESTE, HIGH);

  aguardar(1);

  Serial.print("Conectando à rede Wi-Fi");

  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    aguardar(1);
  }

  Serial.println();

  separador();

  Serial.println("Conectado à rede Wi-Fi.");

  separador();

  rtc.begin();

  timeClient.begin();
  timeClient.update();
  rtc.adjust(DateTime(timeClient.getEpochTime()));

  if (!timeClient.isTimeSet())
    Serial.println("Erro ao sincronizar data/hora!");
  else
    Serial.println("Data/hora sincronizadas via NTP.");

  separador();

  ThingSpeak.begin(apiClient);

  telegramClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  sensor.setup(DHT_PIN, DHTesp::DHT11);
}

void loop() {
  pms.requestRead();

  aguardar(30);

  if (pms.readUntil(dadosPMS)) {
    DateTime now = rtc.now();

    char buf1[20];
    sprintf(buf1, "%02d/%02d/%02d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

    tempUmidade = sensor.getTempAndHumidity();

    if (digitalRead(TESTE) == LOW) {
      tempUmidade.temperature = 45;
      tempUmidade.humidity = 25;
      dadosPMS.PM_AE_UG_1_0 = 150;
      dadosPMS.PM_AE_UG_2_5 = 400;
      dadosPMS.PM_AE_UG_10_0 = 400;
    }

    Serial.println("-[ Data/hora atual ]-");
    Serial.println(String(buf1));

    separador();

    Serial.println("-[ Temperatura e umidade do ar ]-");
    Serial.println("Temperatura: " + String(tempUmidade.temperature, 1) + "°C");
    Serial.println("Umidade: " + String(tempUmidade.humidity, 1) + "%");

    separador();

    Serial.println("-[ Qualidade do ar ]-");
    Serial.println("PM1.0 : " + String(dadosPMS.PM_AE_UG_1_0) + "(μg/m³)");
    Serial.println("PM2.5 : " + String(dadosPMS.PM_AE_UG_2_5) + "(μg/m³)");
    Serial.println("PM10  : " + String(dadosPMS.PM_AE_UG_10_0) + "(μg/m³)");

    separador();

    ThingSpeak.setField(1, tempUmidade.temperature);
    ThingSpeak.setField(2, tempUmidade.humidity);
    ThingSpeak.setField(3, dadosPMS.PM_AE_UG_1_0);
    ThingSpeak.setField(4, dadosPMS.PM_AE_UG_2_5);
    ThingSpeak.setField(5, dadosPMS.PM_AE_UG_10_0);

    if (tempUmidade.temperature >= 30 && tempUmidade.humidity <= 30 && dadosPMS.PM_AE_UG_2_5 > 150) {
      statusRisco = "**Última leitura**:\n";
      statusRisco += String(buf1) + "\n\n";
      statusRisco += "**Coordenadas (lat,lon)**:\n";
      statusRisco += "```-8.7965008,-63.9479767```\n\n";
      statusRisco += "**Temperatura**:\n";
      statusRisco += "```" + String(tempUmidade.temperature, 1) + "°C```\n";
      statusRisco += "**Umidade**:\n";
      statusRisco += "```" + String(tempUmidade.humidity, 1) + "%```\n\n";
      statusRisco += "**Partículas 1μm**:\n";
      statusRisco += "```" + String(dadosPMS.PM_AE_UG_1_0) + "μg/m³```\n";
      statusRisco += "**Partículas 2,5μm**:\n";
      statusRisco += "```" + String(dadosPMS.PM_AE_UG_2_5) + "μg/m³```\n";
      statusRisco += "**Partículas 10μm**:\n";
      statusRisco += "```" + String(dadosPMS.PM_AE_UG_10_0) + "μg/m³```\n\n";
      statusRisco += "Status: Perigo! 🔴\n\n";
      statusRisco += "Despachar a equipe de campo para averiguação!\n\n";
      statusRisco += "[Acessar o monitoramento](https://thingspeak.com/channels/2647342)";
    }
    else {
      statusRisco = "**Última leitura**:\n";
      statusRisco += String(buf1) + "\n\n";
      statusRisco += "**Coordenadas (lat,lon)**:\n";
      statusRisco += "```-8.7965008,-63.9479767```\n\n";
      statusRisco += "**Temperatura**:\n";
      statusRisco += "```" + String(tempUmidade.temperature, 1) + "°C```\n";
      statusRisco += "**Umidade**:\n";
      statusRisco += "```" + String(tempUmidade.humidity, 1) + "%```\n\n";
      statusRisco += "**Partículas 1μm**:\n";
      statusRisco += "```" + String(dadosPMS.PM_AE_UG_1_0) + "μg/m³```\n";
      statusRisco += "**Partículas 2,5μm**:\n";
      statusRisco += "```" + String(dadosPMS.PM_AE_UG_2_5) + "μg/m³```\n";
      statusRisco += "**Partículas 10μm**:\n";
      statusRisco += "```" + String(dadosPMS.PM_AE_UG_10_0) + "μg/m³```\n\n";
      statusRisco += "Status: Seguro 🟢\n\n";
      statusRisco += "[Acessar o monitoramento](https://thingspeak.com/channels/2647342)";
    }

    bot.sendMessage(SECRET_TG_CHAT_ID, statusRisco, "Markdown");

    ThingSpeak.setStatus(statusRisco);

    int reqHTTP = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);

    if (reqHTTP == 200) {
      statusUpdate = "Canal atualizado com sucesso.";
      Serial.println(statusUpdate);
    } else {
      statusUpdate = "Problema na atualização do canal. Código HTTP: " + String(reqHTTP);
      Serial.println(statusUpdate);
    }

    separador();
  }
}

void aguardar(int segundos) {
  agora = millis();
  while (millis() < agora + (segundos * 1000));
}

void separador() {
  Serial.println();
  Serial.println("----------");
  Serial.println();
}