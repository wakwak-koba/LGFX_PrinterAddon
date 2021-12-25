/*
    https://twitter.com/wakwak_koba/
*/
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

# define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <NetworkPrinter.hpp>
#include <printer/escpos.hpp>

static WiFiClient tcpSocket;
static lgfx_addon::NetworkPrinter<lgfx_addon::epson::ESCPOS> printer(&tcpSocket, 384, 192);

# define printer_ip "192.168.1.100"
# define printer_port 9100

void setup() {
  Serial.begin(115200);

  Serial.println("WiFi");
#if defined (wifi_ssid) && defined (wifi_pass)
  WiFi.begin(wifi_ssid, wifi_pass);
#else
  WiFi.begin();
#endif  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

 if(tcpSocket.connect(printer_ip, printer_port))
 {
    printer.init();
    printer.clear(TFT_WHITE);
   
    printer.drawPngUrl("http://t.wakwak-koba.jp/garakuta/monodora.png");

    printer.setTextColor(TFT_BLACK);
    printer.setFont(&fonts::Font4);
    printer.setTextDatum(textdatum_t::top_right);
    printer.drawString("HELLO", printer.width() - 1, 0);
    printer.drawString("DORAEMON", printer.width() - 1, 30);
    printer.display();

    tcpSocket.println();
    tcpSocket.println();

    tcpSocket.stop();
  } 
}

void loop() {
}
