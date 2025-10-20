#define BLYNK_TEMPLATE_ID "TMPL33drr8YQ2"
#define BLYNK_AUTH_TOKEN "UOoAK1rcL6GV786oPWnp30Rq37xQdkZl"
#define BLYNK_TEMPLATE_NAME "Smart Water Pump"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// ---- Pin definitions ----
#define TRIG_PIN D5
#define ECHO_PIN D6
#define RELAY_PIN D4

// ---- WiFi credentials ----
char ssid[] = "";
char pass[] = "";

// ---- Global variables ----
long duration;
float distance;
int levelPercent;
bool pumpState = false;     // Pump OFF initially
bool autoMode = false;      // V3 = OFF means manual mode

BlynkTimer timer;

// ---- Tank calibration ----
const float FULL_DISTANCE = 4.3;   // 4.3 cm = Full tank
const float EMPTY_DISTANCE = 9.5;  // 9.5 cm = Empty tank

// ---------- Measure water level ----------
void readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);  // Timeout 30 ms
  if (duration == 0) {
    Serial.println("Out of range");
    return;
  }

  // Convert duration to distance (cm)
  distance = (duration * 0.0343) / 2.0;

  // Map distance to 0–100%
  levelPercent = map(distance, EMPTY_DISTANCE, FULL_DISTANCE, 0, 100);
  levelPercent = constrain(levelPercent, 0, 100);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Level: ");
  Serial.print(levelPercent);
  Serial.println(" %");

  // Send level to Blynk gauge
  Blynk.virtualWrite(V0, levelPercent);

  // AUTO mode logic
  if (autoMode) {
    if (levelPercent < 30 && !pumpState) {   // Tank low → Turn ON pump
      pumpState = true;
      digitalWrite(RELAY_PIN, LOW);  // Active LOW relay
      Blynk.virtualWrite(V2, 1);
      Serial.println("AUTO MODE: Pump ON (Low water)");
    } 
    else if (levelPercent > 80 && pumpState) { // Tank full → Turn OFF pump
      pumpState = false;
      digitalWrite(RELAY_PIN, HIGH);
      Blynk.virtualWrite(V2, 0);
      Serial.println("AUTO MODE: Pump OFF (Tank full)");
    }
  }
}

// ---------- Manual Pump Control (V1) ----------
BLYNK_WRITE(V1) {
  if (!autoMode) { // Only works in manual mode
    int value = param.asInt(); // 1 = ON, 0 = OFF
    pumpState = value;
    digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH);
    Blynk.virtualWrite(V2, pumpState);
    Serial.println(pumpState ? "Manual: Pump ON" : "Manual: Pump OFF");
  } else {
    Serial.println("Ignored manual command (AUTO mode active)");
  }
}

// ---------- Auto Mode Switch (V3) ----------
BLYNK_WRITE(V3) {
  int mode = param.asInt(); // 1 = AUTO, 0 = MANUAL
  autoMode = (mode == 1);

  if (autoMode) {
    Serial.println("Mode: AUTO");
  } else {
    Serial.println("Mode: MANUAL");
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pump OFF initially

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, readWaterLevel); // every 2 sec
}

// ---------- Loop ----------
void loop() {
  Blynk.run();
  timer.run();
}


