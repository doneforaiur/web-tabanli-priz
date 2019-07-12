#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>


#define lamba 14

const char* wifi_ad = "******";   // Wifi adı ve şifresini
const char* wifi_sifre = "******"; // değiştiriniz.

const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(10) + 621;
DynamicJsonDocument doc(capacity);

String header; // HTTP isteğini saklamak için
int lamba_durumu = LOW;
int otomatik = HIGH;
WiFiUDP ntpUDP;

const long utcOffsetInSeconds = 10800;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiServer server(8080);
// 80 portu Skype gibi uygulamalar tarafından kullanıldığı için
// hata alırsanız 8080 portunu deneyebilirsiniz.

void reply_state(WiFiClient client, int lamba_durumu, int otomatik, String sunset_time) {

  String lamba_;
  String otomatik_;
  lamba_ = (lamba_durumu == 1) ? "acik" : "kapali";
  otomatik_ = (otomatik == 1) ? "acik" : "kapali";

  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnnection: close\r\n\r\n<!DOCTYPE HTML><html>";
  s += "<link rel=\"icon\" href=\"data:,\">"; // Gereksiz GET favicon.ico isteklerini engellemek için
  s += "<center><font size=\"8\">Lamba suan; " + lamba_;
  if (lamba_ == "acik")
    s += " <b><a href=\"/lamba/kapat\"\">KAPAT</a></b>";
  else
    s += " <b><a href=\"/lamba/ac\"\">AC</a></b>";
  s += "<br>Otomatik suan; " + otomatik_;
  if (otomatik_ == "acik")
    s += " <b><a href=\"/otomatik/kapat\"\">KAPAT</a></b>";
  else
    s += " <b><a href=\"/otomatik/ac\"\">AC</a></b>";
  s += "<br>Gunes batis saati; " + sunset_time + "</html></font>";
  Serial.print("    " + lamba_);
  client.print(s);
  client.flush();
}

String check_automatic_time() {
  timeClient.update();
  String current_time = timeClient.getFormattedTime();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://api.sunrise-sunset.org/json?lat=39.921329648&lng=32.884663128&date=today&formatted=0"); // Ankara
    int httpCode = http.GET();
    String http1 = http.getString();
    deserializeJson(doc, http1);
    if (httpCode > 0) {

      JsonObject results = doc["results"];
      String results_sunset = results["astronomical_twilight_end"];
      String sunset_time = results_sunset.substring(results_sunset.indexOf('T') + 1, results_sunset.indexOf('+'));
      return sunset_time;
    }
    http.end();
  }
}

String sunset_time = "18:00:00";

void setup() {
  Serial.begin(115200);
  pinMode(lamba, OUTPUT);
  digitalWrite(lamba, HIGH);

  Serial.print(wifi_ad);
  Serial.println("'na bağlanılıyor.");

  WiFi.begin(wifi_ad, wifi_sifre);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(WiFi.status());
    // Hata kodu 4 yanlış şifre girildiğini belirtir.
  }
  Serial.println("Bağlandı.");
  Serial.println("Web serverin yerel IP'si;");
  // Modemin arayüzünden NodeMCU'ya sabit bir IP atamanızı öneririm.
  // Böylelikle DDNS ve Port ayarlarınızı yaptıktan sonra NodeMCU'ya
  // internet erişimi olan her yerden bağlanabilirsiniz.

  Serial.println(WiFi.localIP());
  server.begin();
  timeClient.begin();
  sunset_time = check_automatic_time();
  ArduinoOTA.setHostname("nodemcu-v3");
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
  });
  ArduinoOTA.onEnd([]() {
    ESP.restart();
  });
  ArduinoOTA.begin();
}
const long interval = 1000 * 60 * 60 * 24; // Günde 1 defa
unsigned long previousMillis = 0;



void loop() {
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    sunset_time = check_automatic_time();
    previousMillis = currentMillis;
  }
  timeClient.update();
  String current_time = timeClient.getFormattedTime();
  delay(1000);
  if (otomatik == HIGH && lamba_durumu == LOW && sunset_time < current_time) {
    lamba_durumu = HIGH;
    digitalWrite(lamba, LOW);
  }
  WiFiClient client = server.available();
  if (client.available()) {
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();
    if (request.indexOf("/lamba/ac") != -1)  {
      digitalWrite(lamba, LOW);
      lamba_durumu = HIGH;
    }
    if (request.indexOf("/lamba/kapat") != -1)  {
      digitalWrite(lamba, HIGH);
      lamba_durumu = LOW;
      if (otomatik == HIGH && sunset_time < current_time)
        otomatik = LOW;
    }
    if (request.indexOf("/otomatik/ac") != -1)
      otomatik = HIGH;
    if (request.indexOf("/otomatik/kapat") != -1)
      otomatik = LOW;

    reply_state(client, lamba_durumu, otomatik, sunset_time);
  }
}
