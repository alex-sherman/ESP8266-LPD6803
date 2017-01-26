#include <LPD6803.h>

using namespace LPD6803;
uint8_t NUM_LEDS  = 25;

LEDStrip strip = LEDStrip(NUM_LEDS, D1, D2);

void setup()
{
  Serial.begin(115200);
  Serial.println();
  strip.begin();
}
int i = 0;
void loop() {
  strip.setPixel(i, Color(0));
  i++; i %= NUM_LEDS;
  strip.setPixel(i, Color(0xFF, 0xFF, 0xFF));
  strip.show();

  delay(100);
}