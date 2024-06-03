#pragma once

#include "../../wled00/wled.h"

/**
platformio_override.ini example to enable this mod:

    [platformio]
    default_envs = xiao32C3_with_flash_effect

    [env:xiao32C3_with_flash_effect]
    ;; ESP32s3dev_8MB but adding flash effect UserMod
    extends = env:esp32s3dev_8MB
    build_flags = ${env:esp32s3dev_8MB.build_flags} -DUSERMOD_FLASH_EFFECT
*/

bool flash_reset[256];
uint32_t flash_now;

//TODO: set to false before push to prod
bool flash_enable = true;

struct FlashData {
  uint32_t start_ms;
  uint8_t velocity;
  bool start_requested;
};

FlashData flash_data[256]{};
const char flash_effect_name[] = "FlashEffect";

struct FlashEffect : Usermod {
  bool initDone = false;
  uint16_t default_duration_ms = 200;
  bool enabled = true;
  uint8_t drum_lengths[10];
  uint32_t pixel_colors[1024];

  void setup() override {
    Serial.println("flash setup");
    flash_now = millis();
    initDone = true;
  }

  void loop() override {
    return;
  }

  /*
  * Add settings to the configuration file
  */
  void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(flash_effect_name);
    top["default_duration_ms"] = default_duration_ms;
    top["drum_length_0"] = drum_lengths[0];
    top["drum_length_1"] = drum_lengths[1];
    top["drum_length_2"] = drum_lengths[2];
    top["drum_length_3"] = drum_lengths[3];
    top["drum_length_4"] = drum_lengths[4];
    top["drum_length_5"] = drum_lengths[5];
    top["drum_length_6"] = drum_lengths[6];
    top["drum_length_7"] = drum_lengths[7];
    top["drum_length_8"] = drum_lengths[8];
    top["drum_length_9"] = drum_lengths[9];
    top["enabled"] = enabled;
  }

  /*
  * Read Settings from Configuration File
  */
  bool readFromConfig(JsonObject& root) override {
    JsonObject top = root[flash_effect_name];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top["default_duration_ms"], default_duration_ms, 200);
    configComplete &= getJsonValue(top["drum_length_0"], drum_lengths[0], 0);
    configComplete &= getJsonValue(top["drum_length_1"], drum_lengths[1], 0);
    configComplete &= getJsonValue(top["drum_length_2"], drum_lengths[2], 0);
    configComplete &= getJsonValue(top["drum_length_3"], drum_lengths[3], 0);
    configComplete &= getJsonValue(top["drum_length_4"], drum_lengths[4], 0);
    configComplete &= getJsonValue(top["drum_length_5"], drum_lengths[5], 0);
    configComplete &= getJsonValue(top["drum_length_6"], drum_lengths[6], 0);
    configComplete &= getJsonValue(top["drum_length_7"], drum_lengths[7], 0);
    configComplete &= getJsonValue(top["drum_length_8"], drum_lengths[8], 0);
    configComplete &= getJsonValue(top["drum_length_9"], drum_lengths[9], 0);
    configComplete &= getJsonValue(top["enabled"], enabled, true);
    return configComplete;
  }

  /**
    * onStateChanged() is used to detect WLED state change
    * @mode parameter is CALL_MODE_... parameter used for notifications
    */
  void onStateChange(uint8_t mode) override {
    Usermod::onStateChange(mode);
    Serial.printf("onStateChange mode=%d\n", mode);
  }

  void readFromJsonState(JsonObject& root) override {
    if (!initDone || !enabled) return;
    if(!root["flash_enable"].isNull()){
      flash_enable = (root["flash_enable"] == 1);
      Serial.printf("flash_effect enable=%d\n", flash_enable);
    }
    JsonObject flash = root["flash"];
    if(flash.isNull()) return;
    uint8_t velocity = 64;
    if(!flash["vel"].isNull()){
      velocity = flash["vel"];
    }
    JsonArray seg_arr = flash["seg"];
    if(seg_arr.isNull()) return;
    uint8_t seg_id;
    for(uint8_t i = 0; i < seg_arr.size(); i++){
      seg_id = flash["seg"][i];
      if(seg_id>=256) continue;
      flash_data[seg_id].start_ms = millis();
      flash_data[seg_id].velocity = velocity;
      flash_data[seg_id].start_requested = true;
      Serial.printf("flash_effect seg_id=%d vel=%d\n", seg_id, velocity);
    }
  }

  /*
  * Called right before show() Last chance to alter leds before they are shown
  */
  void handleOverlayDraw() override {
    if(!initDone || !enabled || !flash_enable) return;
    
    flash_now = millis();
    uint32_t impulse;

    //Loop through drums and see if they need to be flashed
    int drumEnd = 0;
    for (uint8_t i = 0; i < 10; ++i) {
      auto len = drum_lengths[i];
      if (len == 0) break;
      int drumStart = drumEnd;
      drumEnd += len;
      
      //If this segment is not being flashed, skip it
      if(flash_data[i].start_ms + default_duration_ms <= flash_now) continue;

      //On the first iteration of a flash, take snapshot of the segment's colors
      if(flash_data[i].start_requested){
        for(uint32_t j = drumStart; j < drumEnd; j++){
          pixel_colors[j] = strip.getPixelColor(j);
        }
        flash_data[i].start_requested = false;
      }

      impulse = impulseResponse(flash_now - flash_data[i].start_ms);
      flash_pixel_range(drumStart, drumEnd, impulse, flash_data[i].velocity);
    }

    //Force the strip to update asap
    strip.trigger();
  }
  
  /*
  * Flash the wled segment.
  * impulse is a value between 255-0 that indicates how far into the flash we are
  *   255 = just started, 0 = ending
  * velocity is a value between 0-127 indicating how hard the drum was struck
  */
  void flash_pixel_range(int startPixel, int endPixel, uint32_t impulse, uint8_t velocity){
    uint32_t flash_color;
    for (int i = startPixel; i < endPixel; i++) {
      flash_color = scaleColor(pixel_colors[i], velocity);
      strip.setPixelColor(i, color_blend(pixel_colors[i], flash_color, impulse));
    }
  }


  uint32_t impulseResponse(uint32_t elapsed_ms){
    uint32_t response = 256 * (default_duration_ms-elapsed_ms) / default_duration_ms;
    if (response > 255) response = 255;
    if (response <= 0) response = 1;
    return response;
  }

  /*
  * Scale a color's brightness up by velocity (0-127)
  * Max brightness 255 will be white
  */
  uint32_t scaleColor(uint32_t color, uint32_t velocity){
    uint8_t r = R(color);
    uint8_t g = G(color);
    uint8_t b = B(color);
    uint8_t w = W(color);
    r = constrain(velocity+r, 0, 255);
    g = constrain(velocity+g, 0, 255);
    b = constrain(velocity+b, 0, 255);
    return RGBW32(r,g,b,w);
  }

  /*
  * Returns avg brightness of the color (0-255)
  */
  uint8_t getColorBrightness(uint32_t color){
    uint8_t r = R(color);
    uint8_t g = G(color);
    uint8_t b = B(color);
    uint8_t avg = (r + g + b) / 3;
    return avg;
  }
};
