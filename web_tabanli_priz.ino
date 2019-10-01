#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>


#define lamba 14

const char* wifi_ad = "*******";   // Wifi adı ve şifresini
const char* wifi_sifre = "*******"; // değiştiriniz.

const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(10) + 621;
DynamicJsonDocument doc(capacity);

String header; // HTTP isteğini saklamak için
int lamba_durumu = LOW;
int otomatik = HIGH;
int alarm = HIGH;
String alarm_time = "06:00:00";
WiFiUDP ntpUDP;

const long utcOffsetInSeconds = 10800;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiServer server(80);
// 80 portu Skype gibi uygulamalar tarafından kullanıldığı için
// hata alırsanız 8080 portunu deneyebilirsiniz.

void reply_state(WiFiClient client, int lamba_durumu, int otomatik, String sunset_time,int alarm ,String alarm_time) {

  String lamba_;
  String otomatik_;
  String alarm_;
  lamba_ = (lamba_durumu == 1) ? "acik" : "kapali";
  otomatik_ = (otomatik == 1) ? "acik" : "kapali";
  alarm_ = (alarm == 1) ? "acik" : "kapali";

  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnnection: close\r\n\r\n<!DOCTYPE HTML><html>";
  s += "<style type=\"text/javascript\">"
  "var slider = document.getElementById(\"alarmTime\");"
  "var output = document.getElementById(\"aTime\");"
  "output.innerHTML = slider.value;"
  "slider.oninput = function() {"
    "output.innerHTML = this.value;"
    "sendSlider(this.id, this.value);"
  "}"

  "function sendSlider(slider, value){"
  "  var xhr = new XMLHttpRequest();"
  "  var url = \"/\" + slider + \"=\" + value;"
  "  xhr.open(\"GET\", url, true);"
  "  xhr.send();"
  "}"
  "</style>";
  s+="<body>";
  s += "<link rel=\"icon\" href=\"data:,\">"; // Gereksiz GET favicon.ico isteklerini engellemek için
  s += "<br><center><font size=\"8\">Lamba suan; " + lamba_;
  if (lamba_ == "acik")
    s += " <b><a href=\"/lamba/kapat\"\">KAPAT</a></b>";
  else
    s += " <b><a href=\"/lamba/ac\"\">AC</a></b>";
    
  s += "<br><br>Alarm suan; " + alarm_;
  if (alarm_ == "acik")
    s += " <b><a href=\"/alarm/kapat\"\">KAPAT</a></b>";
  else
    s += " <b><a href=\"/alarm/ac\"\">AC</a></b>";

  s += "<br><br>Otomatik suan; " + otomatik_;
  if (otomatik_ == "acik")
    s += " <b><a href=\"/otomatik/kapat\"\">KAPAT</a></b>";
  else
    s += " <b><a href=\"/otomatik/ac\"\">AC</a></b>";

  s+="<br><br><form action=\"/alarm_set\">"
  "Alarm saati; <input type=\"range\" name=\"alarmTime\" min=\"6\" max=\"9\">"
  "<input type=\"submit\">"
  "</form>";
  s += "<br><br><br><br>Gunes batis saati; " + sunset_time + "<br>Alarm saati; "+ alarm_time +"</body></html></font>";
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



//######################

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


//############################


void loop() {
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    sunset_time = check_automatic_time();
    previousMillis = currentMillis;
  }
  delay(500);
  timeClient.update();
  String current_time = timeClient.getFormattedTime();

  if (otomatik == HIGH && lamba_durumu == LOW && sunset_time < current_time) {
    lamba_durumu = HIGH;
    digitalWrite(lamba, LOW);
  }

  if(lamba_durumu == LOW && alarm == HIGH && current_time > alarm_time && current_time < sunset_time){
    lamba_durumu = HIGH;
    alarm = LOW;
    digitalWrite(lamba,LOW);
  }
  WiFiClient client = server.available();
  if (client.available()) {
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();
    if(request.indexOf("alarmTime")!=-1){
    int index = request.indexOf("alarmTime");
    char aTime = request[index+10];
    //sprintf(aTime, "%i", index);
    alarm_time[1] = aTime;
    }
    if (request.indexOf("/lamba/ac") != -1)  {
      digitalWrite(lamba, LOW);
      lamba_durumu = HIGH;
    }
    if (request.indexOf("/lamba/kapat") != -1)  {
      digitalWrite(lamba, HIGH);
      lamba_durumu = LOW;
      if (otomatik == HIGH && sunset_time < current_time)
        otomatik = LOW;
      if (alarm == HIGH && alarm_time < current_time && current_time < sunset_time)
        alarm = LOW;
    }
    if (request.indexOf("/otomatik/ac") != -1)
      otomatik = HIGH;
    if (request.indexOf("/otomatik/kapat") != -1)
      otomatik = LOW;
    if (request.indexOf("/alarm/ac") != -1)
      alarm = HIGH;
    if (request.indexOf("/alarm/kapat") != -1)
      alarm = LOW;
      
    reply_state(client, lamba_durumu, otomatik, sunset_time, alarm, alarm_time);
  }
}
