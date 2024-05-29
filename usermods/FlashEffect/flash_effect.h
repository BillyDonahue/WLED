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
//static const char _data_FX_MODE_SCAN[] PROGMEM = "Scan@!,# of dots,,,,,Overlay;!,!,!;!";
//const char flashEffectSpec[] PROGMEM = "Flash@Duration;!,!;!;01";
const char flashEffectSpec[] PROGMEM = "Flash@Duration,,,,,,Overlay;!,!;!;01";
bool flash_reset[256];

struct FlashEffect : Usermod {
  bool initDone = false;
  
  void setup() override {
    strip.addEffect(255, &flashEffect_, flashEffectSpec);
    Serial.println("flash setup");
    initDone = true;
  }

  void loop() override {
    // Serial.println("flash loop");
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
    Serial.printf("Event Read JSON State");
    if (!initDone) return;
    JsonObject flash = root["flash"];
    if(flash.isNull()) return;
    auto seg_obj = flash["seg_id"];
    if(seg_obj.isNull()) return;
    uint16_t seg_id = seg_obj;
    if (seg_id >= 256) return;
    flash_reset[seg_id] = true;
    Serial.printf("reset flash_effect seg_id=%d\n", seg_id);
  }

private:
  static uint16_t flashEffect_() {
    uint8_t seg_id = strip.getCurrSegmentId();
    if(flash_reset[seg_id]){
      uint32_t maxDuration = 1000;
      flash_reset[seg_id] = false;
      SEGMENT.aux0 = SEGMENT.call + SEGMENT.speed * maxDuration / FRAMETIME / 256;
      
      Serial.printf("Start flashEffect segment_id=%d \n", seg_id);
    }

    if (SEGMENT.call < SEGMENT.aux0) {
      uint32_t amount = (SEGMENT.aux0 - SEGMENT.call) * 20;
      if (amount > 255) {
        amount = 255;
      }
      uint32_t color = color_fade(SEGCOLOR(0), amount, false);
      SEGMENT.fill(color);
    } else if (SEGMENT.call == SEGMENT.aux0) {
      uint32_t color = SEGCOLOR(1);
      Serial.printf("flashEffect ending %d %x\n", SEGMENT.call, color);
      SEGMENT.fill(color);
    }
    return FRAMETIME;
  }
};
