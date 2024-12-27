#include <CapacitiveSensor.h>

// Debug mode (set to 1 to enable serial debugging, 0 to disable)
#define DEBUG_MODE 0

#if DEBUG_MODE
  #include <SoftwareSerial.h>
  #define DEBUG_TX_PIN PB4         // Software Serial TX pin (physical pin 3)
  // Create software serial object
  SoftwareSerial mySerial(255, DEBUG_TX_PIN); // RX (not used), TX
  #define DEBUG_PRINT(x) mySerial.print(x)
  #define DEBUG_PRINTLN(x) mySerial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// Pin definitions
#define SEND_PIN PB1       // Capacitive sensor send pin (physical pin 6)
#define RECEIVE_PIN PB2    // Capacitive sensor receive pin (physical pin 7)
#define MOSFET_PIN PB0     // PWM output pin for MOSFET (physical pin 5)

// Constants
#define TOUCH_THRESHOLD 150     // Adjust this value based on your setup
#define RELEASE_THRESHOLD 50    // Threshold to detect release
#define FADE_SPEED 1          // Lower number = slower fade
#define DEBOUNCE_DELAY 500     // Delay to prevent multiple triggers
#define MAX_BRIGHTNESS 255     // Maximum LED brightness
#define AUTO_OFF_TIME 600000   // Auto off after 10 minutes (600000ms)

// Create capacitive sensor object
CapacitiveSensor touchSensor = CapacitiveSensor(SEND_PIN, RECEIVE_PIN);

// Variables
bool isOn = false;
bool wasTouched = false;      // Track if sensor was touched
int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastTouchTime = 0;
unsigned long turnOnTime = 0;    // When the light was turned on

void setup() {
  pinMode(MOSFET_PIN, OUTPUT);
  
  #if DEBUG_MODE
    // Initialize software serial
    mySerial.begin(9600);
    DEBUG_PRINTLN(F("Starting up..."));
  #endif
  
  // Initialize the capacitive sensor with lower timeout
  touchSensor.set_CS_Timeout_Millis(100);    // Add timeout to prevent hanging
  touchSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);  // Turn off autocalibration
  
  #if DEBUG_MODE
    DEBUG_PRINTLN(F("Setup complete"));
  #endif
}

void loop() {
  // Read capacitive sensor
  long value = touchSensor.capacitiveSensor(30);
  
  // Check auto-off timer
  if (isOn && (millis() - turnOnTime >= AUTO_OFF_TIME)) {
    isOn = false;
    wasTouched = false;  // Reset touch state when auto-off triggers
    targetBrightness = 0;
    DEBUG_PRINTLN(F("Auto-off triggered"));
  }
  
  #if DEBUG_MODE
    // Debug output
    DEBUG_PRINT(F("Raw Sensor: "));
    DEBUG_PRINT(value);
    DEBUG_PRINT(F(" State: "));
    DEBUG_PRINT(isOn ? "ON" : "OFF");
    DEBUG_PRINT(F(" Touched: "));
    DEBUG_PRINT(wasTouched ? "YES" : "NO");
    DEBUG_PRINT(F(" Brightness: "));
    DEBUG_PRINTLN(currentBrightness);
  #endif
  
  // Check for touch and release
  if (value > TOUCH_THRESHOLD) {
    wasTouched = true;  // Mark as touched
  } 
  else if (value < RELEASE_THRESHOLD && wasTouched && (millis() - lastTouchTime > DEBOUNCE_DELAY)) {
    // Toggle state only on release
    isOn = !isOn;
    targetBrightness = isOn ? MAX_BRIGHTNESS : 0;
    lastTouchTime = millis();
    if (isOn) {
      turnOnTime = millis();  // Reset the auto-off timer when turned on
    }
    wasTouched = false;  // Reset touch state
  }
  else if (value < RELEASE_THRESHOLD) {
    wasTouched = false;  // Reset touch state if value is below threshold
  }

  // Fade handling
  if (currentBrightness < targetBrightness) {
    currentBrightness = min(currentBrightness + FADE_SPEED, targetBrightness);
  } else if (currentBrightness > targetBrightness) {
    currentBrightness = max(currentBrightness - FADE_SPEED, targetBrightness);
  }

  // Output to MOSFET
  analogWrite(MOSFET_PIN, currentBrightness);

  delay(1);
}
