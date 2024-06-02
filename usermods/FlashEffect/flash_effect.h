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
  bool start_requested;
};

FlashData flash_data[256]{};
const char flash_effect_name[] = "FlashEffect";

struct FlashEffect : Usermod {
  bool initDone = false;
  uint16_t default_duration = 200;
  uint32_t default_color = WHITE;
  bool enabled = true;
  uint8_t drum_lengths[10];

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
    top["enabled"] = enabled;

    return configComplete;
  }

  /**
    * onStateChanged() is used to detect WLED state change
    * @mode parameter is CALL_MODE_... parameter used for notifications
    */
  void onStateChange(uint8_t mode) override {
    Usermod::onStateChange(mode);
    // do something if WLED state changed (color, brightness, effect, preset, etc)
    Serial.printf("onStateChange mode=%d\n", mode);
  }

  void readFromJsonState(JsonObject& root) override {
    if (!initDone) return;
    if(!root["flash_enable"].isNull()){
      flash_enable = (root["flash_enable"] == 1);
      Serial.printf("flash_effect enable=%d\n", flash_enable);
    }
    JsonObject flash = root["flash"];
    if(flash.isNull()) return;
    auto seg_obj = flash["seg_id"];
    if(seg_obj.isNull()) return;
    uint16_t seg_id = seg_obj;
    if (seg_id >= 256) return;
    flash_data[seg_id].start_ms = millis();
    flash_data[seg_id].start_requested = true;
    Serial.printf("flash_effect seg_id=%d\n", seg_id);
  }

  /*
  * Called right before show() Last chance to alter leds before they are shown
  */
  void handleOverlayDraw() override {
    if(!flash_enable) return;
    
    flash_now = millis();
    uint32_t impulse;

    //Loop through drums and see if they need to be flashed
    int drumStart = 0;
    for (uint8_t i = 0; i < 10; ++i) {
      auto len = drum_lengths[i];
      if (len == 0)
        break;
      drumStart += len;

      if(flash_data[i].start_ms + default_duration <= flash_now) continue;

      if(flash_data[i].start_requested){
        // Take snapshot of segment colors before flashing, array of colors same size as segment
        // Then pass the snapshot to flash_segment as parameter
        flash_data[i].start_requested = false;
      }

      impulse = impulseResponse(flash_now - flash_data[i].start_ms);
      flash_pixel_range(drumStart - len, drumStart, impulse);
    }

    //Force the strip to update asap
    strip.trigger();
  }
  
  /*
  * Flash the wled segment.
  * impulse is a value between 255-0 that indicates how far into the flash we are
  * 255 = just started, 0 = ending
  * To Do: Add snap shot of colors as 2nd param, fade to that instead of pallet color
  */
  void flash_pixel_range(int startPixel, int endPixel, uint32_t impulse){
    // bool pallet_solid_wrap = (strip.paletteBlend == 1 || strip.paletteBlend == 3);
    for (int i = startPixel; i < endPixel; i++) {
      strip.setPixelColor(i, color_blend(BLACK, WHITE, impulse));
      // strip.color_from_palette(i, true, pallet_solid_wrap, 0),  TBD
    }
  }

  uint32_t impulseResponse(uint32_t elapsed_ms){
    uint32_t response = 256 * (default_duration-elapsed_ms) / default_duration;
    if (response > 255) response = 255;
    if (response <= 0) response = 1;
    return response;
  }
};
