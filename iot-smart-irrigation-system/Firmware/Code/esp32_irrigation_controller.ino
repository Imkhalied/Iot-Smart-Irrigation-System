// =====================================================
// SMART IRRIGATION – INSTANT PUMP, SLOW SERIAL
// =====================================================

#include <DHT.h>

// ------------------ PINS ------------------
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define RAIN_ANALOG 35
#define SOIL_ANALOG 34

#define TRIG_PIN 5
#define ECHO_PIN 18

#define PUMP_PIN 26

// ------------------ RELAY SETTINGS ------------------
// If pump works backward, swap HIGH/LOW here
#define RELAY_ON_LEVEL  LOW 
#define RELAY_OFF_LEVEL HIGH

// ------------------ TIMING ------------------
unsigned long lastSerialTime = 0;
const unsigned long serialDelay = 2000; // Update screen every 2 seconds

// ------------------ TANK VARS ------------------
float tankEmptyDistance = 0;
float tankFullDistance  = 0;
float tankHeightCM      = 0;
bool tankCalibrated     = false;

// ------------------ SOIL SETTINGS ------------------
const int soilDryRaw = 4095; 
const int soilWetRaw = 1800; 

// Tracks pump state for logic
bool isPumpOn = false; 
String pumpReason = "System Start";

// =====================================================
float readUltrasonicCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// =====================================================
void calibrateTank() {
  Serial.println("=== TANK CALIBRATION ===");
  Serial.println("Leave tank EMPTY...");
  delay(5000);
  tankEmptyDistance = readUltrasonicCM();
  Serial.println("Fill tank FULL...");
  delay(5000);
  tankFullDistance = readUltrasonicCM();
  tankHeightCM = tankEmptyDistance - tankFullDistance;
  if (tankHeightCM <= 0) tankHeightCM = 10; 
  Serial.print("Height: "); Serial.println(tankHeightCM);
  tankCalibrated = true;
  Serial.println("=== DONE ===\n");
}

// =====================================================
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  
  digitalWrite(PUMP_PIN, RELAY_OFF_LEVEL); // Start OFF

  dht.begin();
  calibrateTank(); 
}

// =====================================================
void loop() {
  // =================================================
  // PART 1: INSTANT PUMP LOGIC (Runs every millisecond)
  // =================================================
  
  int soilRaw = analogRead(SOIL_ANALOG);

  // LOGIC: 
  // > 3000: Turn ON Immediately
  // < 1800: Turn OFF Immediately
  
  if (soilRaw > 3000) {
    if (!isPumpOn) { // Only change if not already on
      isPumpOn = true;
      pumpReason = "Raw > 3000 (Dry)";
      digitalWrite(PUMP_PIN, RELAY_ON_LEVEL);
    }
  } 
  else if (soilRaw < 1800) {
    if (isPumpOn) { // Only change if not already off
      isPumpOn = false;
      pumpReason = "Raw < 1800 (Wet)";
      digitalWrite(PUMP_PIN, RELAY_OFF_LEVEL);
    }
  }
  // If between 1800 and 3000, do nothing (keep old state)

  // =================================================
  // PART 2: SLOW SERIAL UPDATE (Runs every 2 seconds)
  // =================================================
  
  if (millis() - lastSerialTime >= serialDelay) {
    lastSerialTime = millis();

    // Read other sensors just for display
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    
    float distance = readUltrasonicCM();
    float waterHeight = tankEmptyDistance - distance;
    waterHeight = constrain(waterHeight, 0, tankHeightCM);
    float tankPercent = (waterHeight / tankHeightCM) * 100.0;

    int rainVal = analogRead(RAIN_ANALOG);
    String rainStatus = (rainVal > 3800) ? "CLEAR" : (rainVal > 2500) ? "LIGHT RAIN" : "HEAVY RAIN";
    
    int soilPercent = map(soilRaw, soilDryRaw, soilWetRaw, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    Serial.println("================ STATUS ================");
    Serial.print("Soil Raw   : "); Serial.print(soilRaw); 
    Serial.print(" ("); Serial.print(soilPercent); Serial.println("%)");
    
    Serial.print("Pump State : "); Serial.println(isPumpOn ? "ON (Watering)" : "OFF (Idle)");
    Serial.print("Last Action: "); Serial.println(pumpReason);
    
    Serial.print("Tank Level : "); Serial.print(tankPercent, 0); Serial.println("%");
    Serial.print("Rain       : "); Serial.println(rainStatus);
    Serial.print("Temp/Hum   : "); Serial.print(temp, 1); Serial.print("C / "); Serial.print(hum, 0); Serial.println("%");
    Serial.println("========================================\n");
  }
}