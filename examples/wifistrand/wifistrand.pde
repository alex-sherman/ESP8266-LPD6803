#include <LPD6803.h>
#include <mrpc.h>

using namespace Json;
using namespace MRPC;
using namespace LPD6803;

uint16_t N = 26;

LEDStrip strip = LEDStrip(N, D6, D5);
Color color(0, 0, 0);
Color start_color(0, 0, 0);
enum State { single_color, animation, off };
State state = off;
bool new_input = false;
uint32_t steps_remaining;
uint32_t steps;

void show_color(Color c, uint32_t _steps) {
  steps = _steps;
  steps_remaining = _steps;
  start_color = color;
  color = c;
  state = single_color;
}

Value rgb(Service *self, Value &arg, bool &success) {
    if(arg.isFloat()) {
      float value = arg.asFloat();
      show_color(Color(value * 31, value * 31, value * 31), 1);
    }
    else if(arg.isArray()) {
      Array &value = arg.asArray();
      if(value.size() != 3) {
        success = false;
        return "Array must have 3 elements, RGB";
      }
      show_color(Color(value[0].asInt(), value[1].asInt(), value[2].asInt()), 1);
    }
    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    init(50123);            //Begin MRPC on UDP port 50123
    strip.begin();
    strip.show();
    create_service("rgb", &rgb);
}

void loop() {
    poll();
    if(steps_remaining > 0) {
      steps_remaining--;
      switch(state) {
        case off:
          for(int i = 0; i < N; i++) {
            strip.setPixel(i, Color(0,0,0));
          }
          strip.show();
        case single_color:
          for(int i = 0; i < N; i++) {
            strip.setPixel(i, Color::lerp(start_color, color, 1.0f - 1.0f * steps_remaining / steps));
          }
          strip.show();
          break;
        case animation:
          
          for(int i = 0; i < N; i++) {
            
          }
          break;
      }
    }
    delay(5);
}