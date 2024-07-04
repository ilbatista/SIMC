#include "WiFi.h"
#include "RTClib.h"
#include "LiquidCrystal_I2C.h"
#include "DHTesp.h"
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define POT_PIN 35 //Fixa o pino do potenciômetro que simula o detector de carbono
#define DHT_PIN 23 //Fixa o pino do sensor DHT22

#define TOKEN_TELEGRAM "7047714266:AAGrQOUfDmbzMF9ccp92DaMzE5mqTMosgYM"
#define ID_CHAT "129467599"

WiFiClientSecure cliente;
UniversalTelegramBot bot(TOKEN_TELEGRAM, cliente);

const char* SSID = "Wokwi-GUEST"; //SSID da rede Wi-fi
const char* SENHA =  ""; //Senha da rede Wi-fi

bool risco = false;
int potInicial, pot; //Instancia as variáveis que receberão os valores do potenciômetro
DHTesp sensor; //Instancia o objeto do sensor DHT22
TempAndHumidity dadosIniciais, dados; //Instancia os objetos que receberão as leituras do sensor
RTC_DS1307 rtc; //Instancia o objeto que receberá os dados do relógio
LiquidCrystal_I2C lcd(0x27,20,4); //Instancia o display LCD

unsigned long agora = 0; //Cria a variável a ser usada para controlar o timer interno

void setup() {
  Serial.begin(115200); //Monitor serial para debug

  pinMode(POT_PIN, INPUT); //Pino do potenciômetro
  lcd.init(); //Inicia o display LCD
  //lcd.backlight(); //Liga o backlight do display LCD

  sensor.setup(DHT_PIN, DHTesp::DHT22); //Inicia o sensor DHT22

  WiFi.begin(SSID, SENHA); //Inicia a conexão Wi-fi

  cliente.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  //Aguarda a conexão Wi-fi, enquanto exibe uma mensagem no display
  while (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.println("Conectando");
  }

  //Mostra uma mensagem de conexão bem-sucedida no display
  lcd.setCursor(0, 0);
  lcd.print("Conectado!");
  
  agora = millis(); //Timer interno recebe o momento atual
  while(millis() < agora + 1000); //Cria um delay "non-blocking" de 1 segundo

  printWifiData();

  agora = millis(); //Timer interno recebe o momento atual
  while(millis() < agora + 1000); //Cria um delay "non-blocking" de 1 segundo

  //Aborta a execução caso o relógio em tempo-real (RTC) não seja instanciado
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  dadosIniciais = sensor.getTempAndHumidity(); //Recebe os dados iniciais do sensor

  potInicial = map(analogRead(POT_PIN), 0, 4095, 0, 100); //Lê e mapeia o valor inicial do potenciômetro
  
  //Exibe no display as leituras dos sensores
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.println("Temperatura: " + String(dadosIniciais.temperature, 1) + (char)223 + "C");
  lcd.setCursor(0,1);
  lcd.println("Umidade: " + String(dadosIniciais.humidity, 1) + " %");
  lcd.setCursor(0,2);
  lcd.println("Monoxido: " + String(potInicial) + " ppm");
}

void loop() {
  //delay(10); //Apenas para o simulador

  agora = millis(); //Timer interno recebe o momento atual

  DateTime now = rtc.now(); //Instancia um objeto data/hora e recebe o valor do RTC

  dados = sensor.getTempAndHumidity(); //Recebe os dados atuais do sensor

  pot = map(analogRead(POT_PIN), 0, 4095, 0, 50); //Lê e mapeia o valor atual do potenciômetro

  //Condicional executado se houver alteração nos valores
  if(dados.temperature != dadosIniciais.temperature || dados.humidity != dadosIniciais.humidity || pot != potInicial){
    //Atualiza os dados iniciais para comparação posterior
    dadosIniciais = sensor.getTempAndHumidity();
    potInicial = pot;

    //Exibe no display as leituras dos sensores
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.println("Temperatura: " + String(dados.temperature, 1) + (char)223 + "C");
    lcd.setCursor(0,1);
    lcd.println("Umidade: " + String(dados.humidity, 1) + " %");
    lcd.setCursor(0,2);
    lcd.println("Monoxido: " + String(pot) + " ppm");

    //Condicional executado caso as leituras estejam dentro das margens de risco
    if((dados.temperature >= 30 && dados.humidity <= 30) || pot >= 15){
      //Exibe uma mensagem de alerta no display
      lcd.setCursor(0,3);
      lcd.print("Risco de incendio!");
      risco = true;
    }

    if(risco == true)
      bot.sendMessage(ID_CHAT, "Detectadas condições de risco para a ocorrência de incêndio!", "");
  }

  /*
  Serial.print("Data/hora atual: ");
  if(now.day() < 10)
    Serial.print('0');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  if(now.month() < 10)
    Serial.print('0');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" ");
  if(now.hour() < 10)
    Serial.print('0');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10)
    Serial.print('0');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10)
    Serial.print('0');
  Serial.print(now.second(), DEC);
  Serial.println();
  */

  while(millis() < agora + 1000); //Cria um delay "non-blocking" de 5 segundos
}

void printWifiData() {
  lcd.setCursor(0,0);
  IPAddress ip = WiFi.localIP();
  lcd.println("Endereco IP: ");
  lcd.setCursor(0,1);
  lcd.println(ip);

  byte mac[6];
  lcd.setCursor(0,2);
  WiFi.macAddress(mac);
  lcd.println("Endereco MAC: ");
  lcd.setCursor(0,3);
  lcd.print(mac[5], HEX);
  lcd.print(":");
  lcd.print(mac[4], HEX);
  lcd.print(":");
  lcd.print(mac[3], HEX);
  lcd.print(":");
  lcd.print(mac[2], HEX);
  lcd.print(":");
  lcd.print(mac[1], HEX);
  lcd.print(":");
  lcd.println(mac[0], HEX);
}
