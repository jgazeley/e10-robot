// === Nano sketch additions ===
#include <TFMPlus.h>
#include <AltSoftSerial.h>

AltSoftSerial VEX(8, 9);    // RX=D8, TX=D9 (to VEX)
TFMPlus tfmP;

const int LED_R = A1;
const int LED_G = A3;
const int LED_Y = A5;
const int BUZZ  = 4;   // tone-capable pin

#define NOTE_A5   880
#define NOTE_GS5  831
#define NOTE_E6   1319
#define NOTE_FS6  1480
#define NOTE_G6   1568
#define NOTE_GS6  1661
#define NOTE_C7   2093
#define NOTE_DS6  1245
#define REST 0
#define END -1

// Blink state for "DONE" flashing
bool doneBlink = false;
unsigned long lastBlink = 0;
bool blinkOn = false;

// Your secret tune (quick version)
// ---- secret melody as (note, divider) pairs; END terminates ----
const int16_t secret[] = {
  NOTE_G6, 16,  NOTE_FS6, 16,  NOTE_DS6, 16,  NOTE_A5, 16,
  NOTE_GS5, 16, NOTE_E6,  16,  NOTE_GS6, 16,  NOTE_C7, 16,
  END
};

// Generic tone player (your preferred style)
void playTune(const int16_t *seq, uint16_t tempoBPM, uint8_t pin) {
  const uint32_t wholeMs = (60000UL * 4) / tempoBPM; // whole note length

  for (size_t i = 0; ; ) {
    int16_t note = seq[i++];
    if (note == END) break;

    int16_t denom = seq[i++];
    if (denom <= 0) denom = 4;       // default quarter

    uint32_t dur    = wholeMs / (uint16_t)denom;
    uint32_t playMs = (dur * 9UL) / 10UL; // 90% on
    uint32_t gapMs  = dur - playMs;       // 10% gap

    if (note == REST) {
      noTone(pin);
      delay(dur);
    } else {
      tone(pin, (unsigned)note, (unsigned)playMs);
      delay(playMs);
      noTone(pin);
      if (gapMs) delay(gapMs);
    }
  }
}

void r2ChirpPack(uint8_t pin) {
  const int K = 2000;
  for (int i = 0; i < 6; i++) {
    tone(pin, K + random(-1700, 2000), random(70, 170));
    delay(random(80, 190));   // includes tone time
    noTone(pin);
    delay(random(0, 30));
  }
}


void ledsOff() { digitalWrite(LED_R,LOW); digitalWrite(LED_G,LOW); digitalWrite(LED_Y,LOW); }
void setRGB(bool r, bool g, bool y) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_Y, y ? HIGH : LOW);
}

void robotInit() {
  ledsOff();
  randomSeed(analogRead(A6));   // any floating analog ok
  r2ChirpPack(BUZZ);            // <<< play once at boot
  setRGB(1,0,0); delay(250);
  setRGB(0,1,0); delay(250);
  setRGB(0,0,1); delay(250);
  ledsOff();
}

void handleVexCmds() {
  while (VEX.available()) {
    char c = VEX.read();
    switch (c) {
      case 'I': robotInit(); break;              // init LED sequence
      case 'R': doneBlink=false; setRGB(1,0,0); break; // RED state
      case 'G': doneBlink=false; setRGB(0,1,0); break; // GREEN state
      case 'Y': doneBlink=false; setRGB(0,0,1); break; // YELLOW state
      case 'O': doneBlink=false; ledsOff(); break;     // all off
      case 'F': doneBlink=true;  break;          // start DONE flashing
      case 'f': doneBlink=false; ledsOff(); break;     // stop flashing
      case 'S': playTune(secret, 120, BUZZ); break;            // play secret tune
      default: break;
    }
  }
}

void updateBlink() {
  if (!doneBlink) return;
  unsigned long now = millis();
  if (now - lastBlink >= 200) {
    lastBlink = now;
    static int phase = 0;
    phase = (phase + 1) % 3;

    switch (phase) {
      case 0: setRGB(1,0,0); break; // red
      case 1: setRGB(0,1,0); break; // green
      case 2: setRGB(0,0,1); break; // yellow
    }
  }
}

void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(BUZZ,  OUTPUT);
  robotInit();

  VEX.begin(9600);      // To VEX
  Serial.begin(115200); // To TF-Luna (sensor TX -> Nano RX0 only is OK)
  tfmP.begin(&Serial);
}

void loop() {
  // 1) Read LiDAR and stream to VEX (unchanged behavior)
  int16_t dist, flux, temp;
  if (tfmP.getData(dist, flux, temp)) {
    VEX.print(dist);
    VEX.print('\n');
  } else {
    VEX.print("-1\n");
  }

  // 2) Handle any commands from VEX (LEDs/buzzer)
  handleVexCmds();

  // 3) Maintain non-blocking blink pattern
  updateBlink();

  delay(50); // ~20 Hz distance stream
}
