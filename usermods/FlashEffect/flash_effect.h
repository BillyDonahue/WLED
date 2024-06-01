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

struct FlashEffect : Usermod {
  bool initDone = false;
  uint16_t default_duration = 200;
  uint32_t default_color = WHITE;

  void setup() override {
    Serial.println("flash setup");
    flash_now = millis();
    initDone = true;
  }

  void loop() override {
    return;
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

    //Loop through segments and see if they need to be flashed
    uint8_t seg_size = strip.getSegmentsNum();
    auto segments = strip.getSegments();
    for(uint8_t i=0; i < seg_size; i++){
      if(flash_data[i].start_ms + default_duration <= flash_now) continue;

      if(flash_data[i].start_requested){
        // Take snapshot of segment colors before flashing, array of colors same size as segment
        // Then pass the snapshot to flash_segment as parameter
        flash_data[i].start_requested = false;
      }

      impulse = impulseResponse(flash_now - flash_data[i].start_ms);
      flash_segment(segments[i], impulse);
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
  void flash_segment(Segment segment, uint32_t impulse){
    bool pallet_solid_wrap = (strip.paletteBlend == 1 || strip.paletteBlend == 3);
    for (int i = 0; i < segment.length(); i++) {
      segment.setPixelColor(i, color_blend(segment.color_from_palette(i, true, pallet_solid_wrap, 0), WHITE, impulse));
    }
  }

  uint32_t impulseResponse(uint32_t elapsed_ms){
    uint32_t response = 256 * (default_duration-elapsed_ms) / default_duration;
    if (response > 255) response = 255;
    if (response <= 0) response = 1;
    return response;
  }
};