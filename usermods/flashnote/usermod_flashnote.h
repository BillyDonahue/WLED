#pragma once

#include "wled.h"

namespace {

const char name[] = "flashnote";

const auto colorWhite = RGBW32(255, 255, 255, 0);

template <typename T>
T clamp(T x, T lo, T hi) {
    return min(max(x, lo), hi);
}

/*
 * Flashnote flashes a segment in response to a musical note.
 * Notifications arrive via JSON.
 */
class FlashnoteUsermod : public Usermod {
public:
  void setup() override { setup_ = true; }

  void loop() override {
    if (fakePeriod_) {
      auto now = millis();
      if (now >= impulseAlarm_) {
        impulseStart_ = now;
        impulseAlarm_ = impulseStart_ + fakePeriod_;
        colorUpdated(CALL_MODE_NOTIFICATION);
      }
    }
  }

  void addToJsonInfo(JsonObject &root) override {
    JsonObject user = root["u"];
    if (user.isNull())
      user = root.createNestedObject("u");
  }

  void addToJsonState(JsonObject &root) override {
    if (!setup_)
      return;
    JsonObject usermod = root[FPSTR(name)];
    if (usermod.isNull())
      usermod = root.createNestedObject(FPSTR(name));
    usermod["user0"] = userVar0;
  }

  void readFromJsonState(JsonObject &root) {
    if (!setup_)
      return; // prevent crash on boot applyPreset()

    JsonObject usermod = root[FPSTR(name)];
    if (!usermod.isNull()) {
      // expect JSON usermod data in usermod name object:
      // {"ExampleUsermod:{"user0":10}"}
      userVar0 = usermod["user0"] | userVar0; // if "user0" key exists in JSON,
                                              // update, else keep old value
    }
    // you can as well check WLED state JSON keys
    // if (root["bri"] == 255) Serial.println(F("Don't burn down your
    // garage!"));
  }

  void addToConfig(JsonObject &root) override {
    JsonObject top = root.createNestedObject(FPSTR(name));
    top["impulseDuration"] = impulseDuration_;
    top["flashColor"] = flashColor_;
    top["flashIntensity"] = flashIntensity_;
    top["fakePeriod"] = fakePeriod_;
  }

  bool readFromConfig(JsonObject &root) override {
    JsonObject top = root[FPSTR(name)];
    bool ok = !top.isNull();
    ok &= getJsonValue(top["impulseDuration"], impulseDuration_, 500);
    ok &= getJsonValue(top["flashColor"], flashColor_, colorWhite);
    ok &= getJsonValue(top["flashIntensity"], flashIntensity_, 100);
    ok &= getJsonValue(top["fakePeriod"], fakePeriod_, 5000);
    return ok;
  }

  void handleOverlayDraw() override {
    auto now = millis();
    if (now < impulseStart_ + impulseDuration_) {
      // Parametrically fit t=[start,dur] to color=[flash,oc]
      // phase is how far along [0,1.0] in the impulse we are.
      auto phase = (now - impulseStart_) * 1. / impulseDuration_;
      phase = clamp(phase, 0.0, 1.0);
      double flash[4]{
          static_cast<double>(R(flashColor_)),
          static_cast<double>(G(flashColor_)),
          static_cast<double>(B(flashColor_)),
          static_cast<double>(W(flashColor_)),
      };
      for (uint16_t px = 0; px < strip.getLengthTotal(); ++px) {
        auto oc32 = strip.getPixelColor(px);
        double oc[4]{
            static_cast<double>(R(oc32)),
            static_cast<double>(G(oc32)),
            static_cast<double>(B(oc32)),
            static_cast<double>(W(oc32)),
        };
        for (int i = 0; i != 4; ++i) {
          oc[i] += 1e-2 * flashIntensity_ * (1.0 - phase) * (flash[i] - oc[i]);
        }
        strip.setPixelColor(px, RGBW32(oc[0], oc[1], oc[2], oc[3]));
      }
      colorUpdated(CALL_MODE_NOTIFICATION);
    }
  }

  void onStateChange(uint8_t mode) override {}

private:
  bool setup_ = false;

  // config
  unsigned long impulseDuration_ = 1000;
  unsigned long flashColor_ = RGBW32(255, 255, 255, 0);
  unsigned long fakePeriod_ = 5000; // periodically simulate a hit
  uint8_t flashIntensity_ = 100;  // %

  // runtime state
  unsigned long impulseAlarm_ = 0; // next trigger millis
  unsigned long impulseStart_ = 0; // impulseStarted
};

} // namespace
