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

const char flashEffectSpec[] PROGMEM = "Flash@Duration;!,!;!;01";

struct FlashEffect : Usermod {
  void setup() override {
    strip.addEffect(255, &flashEffect_, flashEffectSpec);
    Serial.println("flash setup");
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

private:
  static uint16_t flashEffect_() {
    if (SEGMENT.call == 0) {
      needs_restart_ = true;
    }

    if (needs_restart_) {
      Serial.printf("flashEffect starting\n");
      uint32_t maxDuration = 1000;
      endCall_ = SEGMENT.call + SEGMENT.speed * maxDuration / FRAMETIME / 256;
      needs_restart_ = false;
    }

    // Which call do we shut off at?

    //Serial.printf("flashEffect duration=%d\n", duration);
    if (SEGMENT.call < endCall_) {
      uint32_t color = SEGCOLOR(0);
      Serial.printf("flashEffect dt=%d, now=%d, t0=%d, %x\n",
                    strip.now - SEGMENT.aux0,
                    strip.now,
                    SEGMENT.aux0,
                    color);
      SEGMENT.fill(color);
    } else if (SEGMENT.call == duration) {
      uint32_t color = SEGCOLOR(1);
      Serial.printf("flashEffect ending %d %x\n", SEGMENT.call, color);
      SEGMENT.fill(color);
    }
    return FRAMETIME;
  }
};
