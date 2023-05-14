#pragma once

#include "wled.h"

namespace {

const char name[] = "flashnote";

/*
 * Flashnote flashes a segment in response to a musical note.
 * Notifications arrive via JSON.
 */
class FlashnoteUsermod : public Usermod {
public:
  void setup() override { initDone_ = true; }

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
    if (!initDone_)
      return;
    JsonObject usermod = root[FPSTR(name)];
    if (usermod.isNull())
      usermod = root.createNestedObject(FPSTR(name));
    usermod["user0"] = userVar0;
  }

  void readFromJsonState(JsonObject &root) {
    if (!initDone_)
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
    top["fakePeriod"] = fakePeriod_;
  }

  bool readFromConfig(JsonObject &root) override {
    JsonObject top = root[FPSTR(name)];
    bool ok = !top.isNull();
    ok &= getJsonValue(top["impulseDuration"], impulseDuration_, 500);
    ok &= getJsonValue(top["fakePeriod"], fakePeriod_, 5000);
    return ok;
  }

  void handleOverlayDraw() override {
    auto now = millis();
    auto end = impulseAlarm_ + impulseDuration_;
    if (now >= end)
      return;
    int rel = static_cast<int>(255 * (end - now) / impulseDuration_);
    auto flashColor = RGBW32(rel, rel, rel, rel);
    for (uint16_t i = 0; i < strip.getLengthTotal(); ++i)
      strip.setPixelColor(i, flashColor);
  }

  void onStateChange(uint8_t mode) override {}

private:
  bool initDone_ = false;

  // config
  unsigned long impulseDuration_ = 1000;
  unsigned long fakePeriod_ = 5000; // periodically simulate a hit

  // runtime state
  unsigned long impulseAlarm_ = 0; // next trigger millis
  unsigned long impulseStart_ = 0; // impulseStarted
};

} // namespace
