#pragma once

#include "wled.h"

namespace {

/*
 * Flashnote flashes a segment in response to a musical note.
 * Notifications arrive via JSON.
 */
class FlashnoteUsermod : public Usermod {
public:
  void setup() override;
  void loop() override;
  void addToJsonInfo(JsonObject &root) override;
  void addToJsonState(JsonObject &root) override;
  void readFromJsonState(JsonObject &root) override;
  void addToConfig(JsonObject &root) override;
  bool readFromConfig(JsonObject &root) override;
  void handleOverlayDraw() override;
  void onStateChange(uint8_t mode) override;

private:
  bool initDone_ = false;
  unsigned long lastTime_ = 0;
  unsigned long fakePeriod_ = 1000; // simulate a hit every fakePeriod msec.
  unsigned long impulseDuration_ = 500;
};

const char extName_[] = "flashnote";

void FlashnoteUsermod::setup() { initDone_ = true; }

void FlashnoteUsermod::loop() {
  if (fakePeriod_) {
    auto now = millis();
    if (now > lastTime_ + fakePeriod_) {
      lastTime_ = now;
      colorUpdated(CALL_MODE_NOTIFICATION);
    }
  }
}

void FlashnoteUsermod::addToJsonInfo(JsonObject &root) {
  JsonObject user = root["u"];
  if (user.isNull())
    user = root.createNestedObject("u");
}

void FlashnoteUsermod::addToJsonState(JsonObject &root) {
  if (!initDone_)
    return;
  JsonObject usermod = root[FPSTR(extName_)];
  if (usermod.isNull())
    usermod = root.createNestedObject(FPSTR(extName_));
  usermod["user0"] = userVar0;
}

/*
 * readFromJsonState() can be used to receive data clients send to the
 * /json/state part of the JSON API (state object). Values in the state object
 * may be modified by connected clients
 */
void FlashnoteUsermod::readFromJsonState(JsonObject &root) {
  if (!initDone_)
    return; // prevent crash on boot applyPreset()

  JsonObject usermod = root[FPSTR(extName_)];
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

void FlashnoteUsermod::addToConfig(JsonObject &root) {
  JsonObject top = root.createNestedObject(FPSTR(extName_));
  top["impulseDuration"] = impulseDuration_;
  top["fakePeriod"] = fakePeriod_;
}

bool FlashnoteUsermod::readFromConfig(JsonObject &root) {
  JsonObject top = root[FPSTR(extName_)];
  bool ok = !top.isNull();
  ok &= getJsonValue(top["impulseDuration"], impulseDuration_, 500);
  ok &= getJsonValue(top["fakePeriod"], fakePeriod_, 5000);
  return ok;
}

void FlashnoteUsermod::handleOverlayDraw() {
  auto now = millis();
  auto end = lastTime_ + impulseDuration_;
  if (now >= end)
    return;
  int rel = static_cast<int>(255 * (end - now) / impulseDuration_);
  auto flashColor = RGBW32(rel, rel, rel, rel);
  for (uint16_t i = 0; i < strip.getLengthTotal(); ++i)
    strip.setPixelColor(i, flashColor);
}

void FlashnoteUsermod::onStateChange(uint8_t mode) {
}

} // namespace
