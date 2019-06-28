#include <ESP8266WiFi.h>


const char* wifi_ad = "********";   // Wifi adı ve şifresini 
const char* wifi_sifre = "*******"; // değiştiriniz.

String header; // HTTP isteğini saklamak için
String lamba_durumu = "kapalı";
const int lamba = D0;

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
    delay(500);
    Serial.print(".");
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
              lamba_durumu = "açık";
              digitalWrite(lamba, HIGH);
            } else if (header.indexOf("GET /lamba/kapat") >= 0) {
              Serial.println("lamba off");
              lamba_durumu = "kapalı";
              digitalWrite(lamba, LOW);
            } 
            
            // Sayfayı görüntülemek için HTML kodu
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Cairo; display: inline; margin: 0px auto; text-align: center; background-color: #ccffb3;}");
            client.println(".button { background-color: #006699; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 35px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
        
            client.println("<svg width=\"300\" height=\"80\"><text fill=\"#00bfbf\" font-family=\"serif\" font-size=\"24\" id=\"svg_1\" stroke=\"#000000\" text-anchor=\"middle\" transform=\"matrix(1.35388 0 0 1.42308 -6.66283 -8.67308)\" x=\"86.5\" xml:space=\"preserve\" y=\"41.5\">Circuit Digest</text></svg>");
          
            // Heading kısmı
            client.println("<p>Lamba durumu;" + lamba_durumu + "</p>");   
            if (lamba_durumu=="kapalı") {
              client.println("<p><a href=\"/lamba/ac\"><button class=\"button\">AÇ</button></a></p>");
              client.println("<svg width=\"500\" height=\"300\"><ellipse cx=\"258.5\" cy=\"125.5\" fill=\"#ffffff\" rx=\"47\" ry=\"52\" stroke=\"#ffffaa\" stroke-width=\"5\"/><rect fill=\"#cccccc\" height=\"40\" stroke=\"#ffffaa\" stroke-width=\"5\" transform=\"rotate(-0.485546 261 187.5)\" width=\"39\" x=\"241.5\" y=\"167.5\"/></svg>");
            } else {
              client.println("<p><a href=\"/lamba/kapat\"><button class=\"button button2\">KAPAT</button></a></p>");
              client.println("<svg width=\"500\" height=\"300\"><ellipse cx=\"258.5\" cy=\"125.5\" fill=\"#ff7f00\" rx=\"47\" ry=\"52\" stroke=\"#ffffaa\" stroke-width=\"5\"/><rect fill=\"#cccccc\" height=\"40\" stroke=\"#ffffaa\" stroke-width=\"5\" transform=\"rotate(-0.485546 261 187.5)\" width=\"39\" x=\"241.5\" y=\"167.5\"/></svg>");
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

