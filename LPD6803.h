#include "Arduino.h"

namespace LPD6803 {
    class Color {
    public:
        static uint8_t RGBOrder[3];
        uint16_t value;
        Color(uint8_t r, uint8_t g, uint8_t b) {
            RGB(r, g, b);
        }
        Color(uint16_t value) {
            this->value = 0x8000 | (value & 0x7FFF);
        }
        void RGB(uint8_t r, uint8_t g, uint8_t b) {
            value = 0;
            value |= (r & 0x1F) << 5 * RGBOrder[0];
            value |= (g & 0x1F) << 5 * RGBOrder[1];
            value |= (b & 0x1F) << 5 * RGBOrder[2];
            value |= 0x8000;
      }
    };
    class LEDStrip {
    private:
        uint8_t cpumax;

    public:
        LEDStrip(uint16_t n, uint8_t dpin, uint8_t cpin);
        void begin();
        void show();
        void doSwapBuffersAsap(uint16_t idx);
        void setPixel(uint16_t n, Color c);
        void setPixel(uint16_t n, uint16_t c);
        uint16_t getPixelColor(uint16_t n);
        void LerpRange(uint16_t start, uint16_t end, uint16_t *values);
        void setCPUmax(uint8_t m);
        uint16_t numPixels(void);
    };
}