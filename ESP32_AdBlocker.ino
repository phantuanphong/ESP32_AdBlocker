/*
   ESP32_AdBlocker acts as a DNS Sinkhole by returning 0.0.0.0 for any domain names in its blocked list, 
   else forwards to an external DNS server to resolve IP addresses. This prevents content being retrieved 
   from or sent to blocked domains. Checks generally take <100us.

   To use ESP32_AdBlocker, enter its IP address in place of the DNS server IPs in your router/devices.
   Currently it does not have an IPv6 address and some devices use IPv6 by default, so disable IPv6 DNS on 
   your device / router to force it to use IPv4 DNS.

   Blocklist files can be downloaded from hosting sites and should either be in HOSTS format 
   or Adblock format (only domain name entries processed)

   arduino-esp32 library DNSServer.cpp modified as custom AdBlockerDNSServer.cpp so that DNSServer::processNextRequest()
   calls checkBlocklist() in ESP32_AdBlocker to check if domain blocked, which returns the relevant IP. 
   Based on idea from https://github.com/rubfi/esphole

   s60sc 2020, 2023
*/

#include "appGlobals.h"

unsigned long upTimeMillis = 0;

// Task handles
TaskHandle_t indicatorTaskHandle;


void setup() { 
  upTimeMillis = millis();
  logSetup();
  // prep data storage
  if (startStorage()) {
    // Load saved user configuration
    if (loadConfig()) {
      if (psramFound()) {
        LOG_INF("PSRAM size: %s", fmtSize(ESP.getPsramSize()));
        LOG_INF("PSRAM free: %s", fmtSize(ESP.getFreePsram()));
        if (ESP.getPsramSize() < 3 * ONEMEG) 
          snprintf(startupFailure, SF_LEN, STARTUP_FAIL "Insufficient PSRAM for app");
      }
      else snprintf(startupFailure, SF_LEN, STARTUP_FAIL "Need PSRAM to be enabled");
    }
  }

#ifdef DEV_ONLY
  devSetup();
#endif

  // connect wifi or start config AP if router details not available
  startWifi(); 
  
  startWebServer();
  if (strlen(startupFailure)) LOG_ERR("%s", startupFailure);
  else {
    // start rest of services
    appSetup();
    checkMemory();
  }

  if (ledIndicator){
    startLedIndicator();
  }
}

void loop() {
  // confirm not blocked in setup
  LOG_INF("=============== Total tasks: %u ===============\n", uxTaskGetNumberOfTasks() - 1);
  delay(1000);
  vTaskDelete(NULL); // free 8k ram
}

void startLedIndicator(){
  if (indicatorTaskHandle == NULL){
    LOG_INF("Start led indicator");
    xTaskCreate(blinkRed, "Blink Red", 2048, NULL, 1, &indicatorTaskHandle);
  }
}

// Function to delete the red task
void stopLedIndicator() {
  if (indicatorTaskHandle != NULL) {
    LOG_INF("Stop led indicator");
    vTaskDelete(indicatorTaskHandle); // Delete the red blinking task
    indicatorTaskHandle = NULL; // Clear the handle
  }
}


// Function to blink the red LED
void blinkRed(void *parameter) {
#ifdef RGB_BUILTIN
  while (true) {
    neopixelWrite(RGB_BUILTIN,5,0,0);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    neopixelWrite(RGB_BUILTIN,0,0,0);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
#endif
}