#include <ESP8266WiFi.h>
#define lamba 2

const char* wifi_ad = "*******";   // Wifi adı ve şifresini 
const char* wifi_sifre = "*******"; // değiştiriniz.

String header; // HTTP isteğini saklamak için
String lamba_durumu = "kapali";


WiFiServer server(80);
// 80 portu Skype gibi uygulamalar tarafından kullanıldığı için
// hata alırsanız 8080 portunu deneyebilirsiniz.

void setup() {
  Serial.begin(115200);
  pinMode(lamba, OUTPUT);
  digitalWrite(lamba, LOW);
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
}

void loop(){
  WiFiClient client = server.available();   
  if (client) {                                
    String kullanici_girdisi = "";
    while (client.connected())
      if (client.available()) {      // Kullanici bilgi aktariyorsa
        char c = client.read();             
        Serial.write(c);                    
        header += c;
        if (c == '\n') {                    // Aktarilan bilgi yeni satır karakteriyse
          if (kullanici_girdisi.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /lamba/ac") >= 0) {
              Serial.println("Röle aktif.");
              lamba_durumu = "acik";
              digitalWrite(lamba, LOW);
            } else if (header.indexOf("GET /lamba/kapat") >= 0) {
              Serial.println("Röle aktif değil.");
              lamba_durumu = "kapali";
              digitalWrite(lamba, HIGH);
            } 
            
            // Sayfayı görüntülemek için HTML kodu
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Cairo; display: inline; margin: 0px auto; text-align: center; background-color: #FFFFFF;}");
            client.println(".button { background-color: #006699; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 35px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
                 
            // Heading kısmı
            client.println("<p>Lamba durumu;" + lamba_durumu + "</p>");   
            if (lamba_durumu=="kapali") {
              client.println("<p><a href=\"/lamba/ac\"><button class=\"button\">Ac</button></a></p>");
            } else {
              client.println("<p><a href=\"/lamba/kapat\"><button class=\"button button2\">KAPAT</button></a></p>");
            }     
            client.println("</body></html>");
            client.println();
            break;
          } else { 
            kullanici_girdisi = "";
          }
        } else if (c != '\r') {  
          kullanici_girdisi += c;
        }
      }
    }
    header = "";
    client.stop();
  }

