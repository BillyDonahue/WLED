#pragma once

#include "wled.h"

/*
* PhilipsWho Usermod class that reads information
* from WLED and lights LED's as a result.
* LED 1: wifi connection status
* LED 2: WLED activity status
* Contact: chill@chillaspect.com
*/
class PhilipsWho : public Usermod {
  private:
    // strings to reduce flash memory usage (used more than twice)
    static const char PhilipsWho::_name[] PROGMEM = "PhilipsWho";
    static const char PhilipsWho::_enabled[] PROGMEM = "enabled";

  public:
    //How frequently to check for wifi
    const unsigned long poll_frequency_wifi = 5000; // every 5 second

    //Time when wifi was last checked
    unsigned long last_checked_wifi = 0;

    //How long the Activity LED remains on when activity detected
    unsigned long activity_led_on_period = 1000;

    //Default Pin Values
    int activity_led_pin = 13;
    int wifi_status_led_pin = 14;

    //Enabled Bool
    bool enabled = false;

    //The current time since ESP32 Started (in ms)
    unsigned long current_ms = 0L;

    //Time (in ms) when onStateChange was called
    unsigned long state_change_fired_ms = 0;

    /*
    * Check if wifi is connected
    * Light the LED appropriately
    */
    void check_wifi() {
      if (WLED_CONNECTED) {
          digitalWrite(wifi_status_led_pin, HIGH);
      }else{
          digitalWrite(wifi_status_led_pin, LOW);
      }
    }

    /**
     * Enable/Disable the usermod
     */
    void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    bool isEnabled() { return enabled; }

    /*
    * Callback function called when any change happens in WLED
    * This includes light and configuration changes
    */
    void onStateChange(uint8_t mode){
      digitalWrite(activity_led_pin, HIGH);
      state_change_fired_ms = millis();
    }

    /*
    * Add settings to the configuration file
    */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top["activity_led_pin"] = activity_led_pin;
      top["activity_led_on_period"] = activity_led_on_period;
      top["wifi_status_led_pin"] = wifi_status_led_pin;
      top["enabled"] = enabled;
    }

    /*
    * Read Settings from Configuration File
    */
    bool readFromConfig(JsonObject& root)
    {
      JsonObject top = root[FPSTR(_name)];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top["activity_led_pin"], activity_led_pin, 13);
      configComplete &= getJsonValue(top["activity_led_on_period"], activity_led_on_period, 1000);
      configComplete &= getJsonValue(top["wifi_status_led_pin"], wifi_status_led_pin, 14);
      configComplete &= getJsonValue(top["enabled"], enabled, false);

      // Turn off LED's if disabling
      if(!enabled){
          digitalWrite(wifi_status_led_pin, LOW);
          digitalWrite(activity_led_pin, LOW);
      }

      return configComplete;
    }

    /*
    * Initial Setup - Called once on boot
    */
    void setup() {
      Serial.println("PhilipsWho - Setup");
      pinMode(wifi_status_led_pin, OUTPUT);
      pinMode(activity_led_pin, OUTPUT);
      digitalWrite(wifi_status_led_pin, LOW);
      digitalWrite(activity_led_pin, LOW);
    }

    /*
    * Main Loop - Called continuously while ESP32 is running
    */
    void loop() {
      //If disabled, do nothing
      if (!enabled) return;

      //Get the current time in MS
      current_ms = millis();

      //Do we need to check for wifi yet?
      if(current_ms - last_checked_wifi >= poll_frequency_wifi){
          last_checked_wifi = current_ms;
          check_wifi();
      }

      if(current_ms - state_change_fired_ms >= activity_led_on_period){
          digitalWrite(activity_led_pin, LOW);
      }

      //Check for millis overflow - reset back to 0
      if(current_ms < last_checked_wifi){
          last_checked_wifi = 0;
          state_change_fired_ms = 0;
      }
    }
};
