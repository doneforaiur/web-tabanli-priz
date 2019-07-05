#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#define lamba 14


const char* wifi_ad = "VodafoneNet-VHU2PP";   // Wifi adı ve şifresini 
const char* wifi_sifre = "432111ma"; // değiştiriniz.


String header; // HTTP isteğini saklamak için
int lamba_durumu = LOW;
int otomatik = LOW;
WiFiUDP ntpUDP;

const long utcOffsetInSeconds = 10800;   
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); 

WiFiServer server(8080);
// 80 portu Skype gibi uygulamalar tarafından kullanıldığı için
// hata alırsanız 8080 portunu deneyebilirsiniz.

void reply_state(WiFiClient client,int lamba_durumu, int otomatik) {

  String lamba_;
  String otomatik_;
  lamba_ = (lamba_durumu == 1) ? "acik" : "kapali";
  otomatik_ = (otomatik == 1) ? "acik" : "kapali";

  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnnection: close\r\n\r\n<!DOCTYPE HTML><html>";
  s +="<font size=\"5\">Lamba suan; "+lamba_;
  if(lamba_ == "acik")
    s+= "<br><a href=\"/lamba/kapat\"\">KAPAT</a></html>";
  else
    s+= "<br><a href=\"/lamba/ac\"\">AC</a></html>";
  s+="</font>";
  Serial.println(lamba_durumu);
  client.print(s);
  client.flush();
  delay(1);
}



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

}
const long interval =  500; 
unsigned long previousMillis = 0;   

const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(10) + 621;
DynamicJsonDocument doc(capacity);

void loop(){

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
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
        String sunset_time = results_sunset.substring(results_sunset.indexOf('T')+1,results_sunset.indexOf('+'));

        if(otomatik==HIGH && lamba_durumu == LOW && sunset_time < current_time){
          lamba_durumu = HIGH;
          digitalWrite(lamba, LOW);
        }
      }
      http.end(); 
    }
  }

    
  
  WiFiClient client = server.available();   

  if(client.available()){
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
    }
    int otomatik = LOW;
    if (request.indexOf("/otomatik/ac") != -1)  {
      otomatik = HIGH;
    }
    if (request.indexOf("/otomatik/kapat") != -1)  {
      otomatik = LOW;
    }
     reply_state(client,lamba_durumu,otomatik);
    }

}

