#include <LPD6803.h>
#include <mrpc.h>

using namespace Json;
using namespace MRPC;
using namespace LPD6803;

#define N 26
Color lerp_start[N];
Color lerp_end[N];
LEDStrip strip = LEDStrip(N, D6, D5);
Color color(0, 0, 0);
Color color_max(0, 0, 0);
bool b_animation = false;
uint32_t steps_remaining = 0;
uint32_t steps = 0;
int animation_length = 0;
float light_value = 1.0f;

bool isColor(Json::Value value) {
  if(!value.isArray()) return false;
  Array &c = value.asArray();
  return c.size() == 3 && c[0].isFloat() && c[1].isFloat() && c[2].isFloat();
}

Color toColor(Json::Value value) {
  Array &c = value.asArray();
  return Color(c[0].asFloat() * 31, c[1].asFloat() * 31, c[2].asFloat() * 31);
}

int glowy(Color *colors) {
  randomSeed((int)RANDOM_REG32);
  for(int i = 0; i < N; i++) {
    colors[i] = Color(random(color.r(), color_max.r()), random(color.g(), color_max.g()), random(color.b(), color_max.b()));
  }
  return animation_length;
}

void show_color(Color c, uint32_t _steps) {
  b_animation = false;
  steps = _steps;
  steps_remaining = _steps;
  for(int i = 0; i < N; i++) {
    lerp_start[i] = Color(strip.getPixelColor(i));
    lerp_end[i] = c;
  }
}

Value light(Value &arg, bool &success) {
  if(arg.isBool()) {
    light_value = arg.asBool() ? 1.0f : 0.0f;
  }
  if(arg.isFloat()) {
    light_value = fmax(0.0f, fmin(1.0f, arg.asFloat()));
  }
  return light_value;
}

Value rgb(Value &arg, bool &success) {
    if(arg.isFloat()) {
      float value = arg.asFloat();
      show_color(Color(value * 31, value * 31, value * 31), 10);
    }
    else if(arg.isArray()) {
      if(!isColor(arg)) {
        success = false;
        return "Array must have 3 elements, RGB";
      }
      show_color(toColor(arg), 10);
    }
    return true;
}

Value animation(Value &arg, bool &success) {
  if(arg.isArray()) {
    Array &top = arg.asArray();
    if(top.size() == 3 && isColor(top[0]) && isColor(top[1]) && top[2].isInt()) {
      color = toColor(top[0]);
      color_max = toColor(top[1]);
      animation_length = top[2].asInt();
    }
    b_animation = true;
  }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    init(50123);            //Begin MRPC on UDP port 50123
    strip.begin();
    strip.show();
    create_service("rgb", &rgb);
    create_service("light", &light);
    create_service("animation", &animation);
}
unsigned long last_anim_update = 0;
void loop() {
    poll();
    if(steps_remaining > 0) {
      steps_remaining--;
    }

    if(steps_remaining <= 0 && b_animation) {
      memcpy(lerp_start, lerp_end, sizeof(Color) * N);
      steps = glowy(lerp_end);
      steps_remaining = steps;
    }

    for(int i = 0; i < N; i++) {
      strip.setPixel(i, Color::lerp(lerp_start[i], Color::lerp(Color(), lerp_end[i], light_value), 1.0f - 1.0f * steps_remaining / steps));
    }
    strip.show();
    delay(5);
}