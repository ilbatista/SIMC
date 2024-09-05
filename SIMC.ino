#include "PMS.h"
#include "WiFi.h"
#include "NTPClient.h"
#include "WiFiUdp.h"
#include "LiquidCrystal_I2C.h"
#include "Wire.h"
#include "RTClib.h"
#include "DHTesp.h"
#include "Chaves.h"
#include "ThingSpeak.h"

#define RXD2 16
#define TXD2 17
#define DHT_PIN 23 //Fixa o pino do sensor DHT22

const char* ssid = SECRET_SSID; //SSID da rede Wi-fi
const char* senha =  SECRET_PASS; //Senha da rede Wi-fi
const long utcOffsetInSeconds = -4 * 3600;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

PMS pms(Serial2);
PMS::DATA data;
DHTesp sensor; //Instancia o objeto do sensor DHT22
TempAndHumidity dadosIniciais, dados; //Instancia os objetos que receberão as leituras do sensor
RTC_DS1307 rtc; //Instancia o objeto que receberá os dados do relógio
LiquidCrystal_I2C lcd(0x27,16,2); //Instancia o display LCD
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
WiFiClient client;

unsigned long agora = 0; //Cria a variável a ser usada para controlar o timer interno

void setup()
{
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.begin(115200);
  Serial.println("Aquecendo");

  agora = millis(); //Timer interno recebe o momento atual
  while(millis() < agora + 5000); //Cria um delay "non-blocking" de 1 segundo

  lcd.init(); //Inicia o display LCD
  lcd.backlight(); //Liga o backlight do display LCD

  sensor.setup(DHT_PIN, DHTesp::DHT11); //Inicia o sensor DHT11

  WiFi.begin(ssid, senha); //Inicia a conexão Wi-fi

  //Aguarda a conexão Wi-fi, enquanto exibe uma mensagem no display
  while (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(0,0);
    lcd.print("Conectando");
  }

  //Mostra uma mensagem de conexão bem-sucedida no display
  lcd.setCursor(0,0);
  lcd.print("Conectado!");
  
  ThingSpeak.begin(client);

  agora = millis(); //Timer interno recebe o momento atual
  while(millis() < agora + 1000); //Cria um delay "non-blocking" de 1 segundo

  //Aborta a execução caso o relógio em tempo-real (RTC) não seja instanciado
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  timeClient.begin();
  timeClient.update();

  rtc.adjust(DateTime(timeClient.getEpochTime()));

  dadosIniciais = sensor.getTempAndHumidity(); //Recebe os dados iniciais do sensor

  //Exibe no display as leituras dos sensores
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temp: " + String(dadosIniciais.temperature, 1) + (char)223 + "C");
  lcd.setCursor(0,1);
  lcd.print("Umid: " + String(dadosIniciais.humidity, 1) + "%");
}
 
void loop()
{
  if (pms.read(data))
  {
    agora = millis(); //Timer interno recebe o momento atual
    while(millis() < agora + 1000); //Cria um delay "non-blocking" de 1 segundo

    DateTime now = rtc.now(); //Instancia um objeto data/hora e recebe o valor do RTC

    dados = sensor.getTempAndHumidity(); //Recebe os dados atuais do sensor

    //Condicional executado se houver alteração nos valores
    if(dados.temperature != dadosIniciais.temperature || dados.humidity != dadosIniciais.humidity){
      //Atualiza os dados iniciais para comparação posterior
      dadosIniciais = sensor.getTempAndHumidity();

      //Exibe no display as leituras dos sensores
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Temp: " + String(dados.temperature, 1) + (char)223 + "C");
      lcd.setCursor(0,1);
      lcd.print("Umid: " + String(dados.humidity, 1) + "%");

      agora = millis(); //Timer interno recebe o momento atual
      while(millis() < agora + 1000); //Cria um delay "non-blocking" de 1 segundo

      lcd.setCursor(0,0);
      lcd.print("PM2.5: " + String(data.PM_AE_UG_2_5) + "ug/m3");
      lcd.setCursor(0,1);
      lcd.print("PM10:  " + String(data.PM_AE_UG_10_0) + "ug/m3");
    }

    Serial.println("Data/hora atual");
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
    Serial.println();

    Serial.println("Temperatura e umidade do ar");
    Serial.println("Temperatura: " + String(dados.temperature, 1) + "°C");
    Serial.println("Umidade: " + String(dados.humidity, 1) + " %");
    Serial.println();

    Serial.println("Qualidade do ar");
    Serial.println("PM1.0 : " + String(data.PM_AE_UG_1_0) + "(μg/m³)");
    Serial.println("PM2.5 : " + String(data.PM_AE_UG_2_5) + "(μg/m³)");
    Serial.println("PM10  : " + String(data.PM_AE_UG_10_0) + "(μg/m³)");
    Serial.println();
    Serial.println("----------");
    Serial.println();

    ThingSpeak.setField(1, dados.temperature);
    ThingSpeak.setField(2, dados.humidity);
    ThingSpeak.setField(3, data.PM_AE_UG_1_0);
    ThingSpeak.setField(4, data.PM_AE_UG_2_5);
    ThingSpeak.setField(5, data.PM_AE_UG_10_0);

    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if(x == 200)
      Serial.println("Channel update successful.");
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("HTTP error" + String(x));
    }

    agora = millis(); //Timer interno recebe o momento atual
    while(millis() < agora + 10000); //Cria um delay "non-blocking" de 10 segundos
  }
}
