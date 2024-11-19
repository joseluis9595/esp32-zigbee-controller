#include "Zigbee.h"
#include <Adafruit_NeoPixel.h>


#ifndef ZIGBEE_MODE_ED
#error "Zigbee ED(end device) mode is not selected in Tools->Zigbee mode"
#endif


/************************ Onboard LED *****************************/
struct RGB {
    uint8_t r, g, b;
};
constexpr RGB COLOR_OFF   = {0, 0, 0};
constexpr RGB COLOR_RED   = {255, 0, 0};
constexpr RGB COLOR_GREEN = {0, 255, 0};
constexpr RGB COLOR_BLUE  = {0, 0, 255};
constexpr RGB COLOR_WHITE = {255, 255, 255};
constexpr RGB CUSTOM_COLOR = {255, 0, 255}; 
Adafruit_NeoPixel rgbLed(1, 8, NEO_GRB + NEO_KHZ800);


/************************ GPIO *****************************/
// Structure for button configuration
struct ButtonConfig {
  int pin;            // GPIO pin for the button
  uint8_t endpoint;   // Zigbee endpoint number
};

// Button configurations
ButtonConfig buttons[] = {
  {13, 1},
  {12, 2},
  {11, 3},
};

// Number of buttons
const int numButtons = sizeof(buttons) / sizeof(buttons[0]);

// Zigbee switch objects
ZigbeeSwitch* zigbeeSwitches[numButtons];


/************************ Button press *****************************/

// Send Zigbee commands
void sendZigbeeToggleCommand(uint8_t endpoint) {
  esp_zb_zcl_on_off_cmd_t cmd_req;
  cmd_req.zcl_basic_cmd.src_endpoint = endpoint;
  cmd_req.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  cmd_req.on_off_cmd_id = ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID;

  log_i("Sending 'toggle' command for Endpoint: %d", endpoint);
  esp_zb_zcl_on_off_cmd_req(&cmd_req);
}

// Setup GPIO pins and create ZigbeeSwitch instances
void setupButtonsAndEndpoints() {
  for (int i = 0; i < numButtons; i++) {
    // Initialize button pin as input with internal pull-up resistor
    pinMode(buttons[i].pin, INPUT_PULLUP);

    // Create and configure Zigbee switch
    zigbeeSwitches[i] = new ZigbeeSwitch(buttons[i].endpoint);
    zigbeeSwitches[i]->setManufacturerAndModel("Espressif", "RoomController");
    Zigbee.addEndpoint(zigbeeSwitches[i]);
  }
}

// Debounce button press for each pin
void handleButtonPress(const ButtonConfig& button) {
  if (digitalRead(button.pin) == LOW) { // Button pressed
    delay(50); // Debounce delay
    if (digitalRead(button.pin) == LOW) { // Confirm still pressed
      sendZigbeeToggleCommand(button.endpoint);

      // Wait for button release to avoid multiple toggles
      while (digitalRead(button.pin) == LOW) {
        delay(10);
      }
    }
  }
}


/********************* Arduino functions **************************/
void changeLEDColor(const RGB& color) {
  rgbLed.setPixelColor(0, rgbLed.Color(color.r, color.g, color.b));
  rgbLed.show();
}



/********************* Arduino functions **************************/
void setup() {
  // Serial configuration
  Serial.begin(9600);

  // Initialize onboard LED
  rgbLed.begin(); 
  rgbLed.show(); 
  changeLEDColor({20,0,0});

  // Initialize Zigbee and buttons
  setupButtonsAndEndpoints();

  // Create a custom Zigbee configuration for End Device with keep alive 10s to avoid interference with reporting data
  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 10000;

  // When all EPs are registered, start Zigbee in End Device mode
  Zigbee.begin(&zigbeeConfig, false);

  // Wait for Zigbee to start
  while (!Zigbee.isStarted()) {
    delay(100);
  }

  Serial.println("Zigbee initialized");
  changeLEDColor({0,5,0});
}


void loop() {
    // Poll each button for presses
  for (int i = 0; i < numButtons; i++) {
    handleButtonPress(buttons[i]);
  }
}





