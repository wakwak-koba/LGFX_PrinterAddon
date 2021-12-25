/*
    https://twitter.com/wakwak_koba/
*/
#include <WiFi.h>
#include <HTTPClient.h>

# define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <SerialPrinter.hpp>
#include <printer/aiebcy/EM5820.hpp>

static lgfx_addon::SerialPrinter<lgfx_addon::aiebcy::EM5820> printer(&Serial1, 192, 19);

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
  
  Serial1.begin(9600, SERIAL_8N1, 33, 23);
  printer.init();
  printer.clear(TFT_WHITE);
  
  printer.drawPngUrl("http://t.wakwak-koba.jp/garakuta/monodora.png");

  printer.setTextColor(TFT_BLACK);
  printer.setFont(&fonts::Font4);
  printer.setTextDatum(textdatum_t::top_right);
  printer.drawString("HELLO", printer.width() - 1, 0);
  printer.drawString("DORAEMON", printer.width() - 1, 30);
  printer.display();

  Serial1.println();
  Serial1.println();
}

void loop() {
}
