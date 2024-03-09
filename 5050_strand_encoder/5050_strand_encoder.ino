
#include "Button2.h" //  https://github.com/LennartHennigs/Button2
#include "ESPRotary.h"

#define DT_PIN D0
#define CLK_PIN D6
#define SW_PIN D5

#define ROTARY_PIN1 DT_PIN
#define ROTARY_PIN2 CLK_PIN
#define BUTTON_PIN SW_PIN

#define MASTER_STRIP_PIN D1
#define RED_STRIP_PIN D8
#define GREEN_STRIP_PIN D4
#define BLUE_STRIP_PIN D7

#define CLICKS_PER_STEP 5
#define MAX_POS 255
#define MIN_POS 0
#define START_POS 0
#define INCREMENT 5

ESPRotary r;
Button2 b;
byte currentMode=0;

const char * modes[] = {
  "red",
  "green",
  "blue",
  "master"
};

byte settings[] = {
  125,
  125,
  125,
  125
};

byte stripPins[] = {
  RED_STRIP_PIN,
  GREEN_STRIP_PIN,
  BLUE_STRIP_PIN,
  MASTER_STRIP_PIN
};

void rotaryClockwise(ESPRotary& r) {
  settings[currentMode]=r.getPosition();

  Serial.print("Went right.  Setting ");

  setOnTurn();
}

void rotaryCounterClockwise(ESPRotary& r) {
  settings[currentMode]=r.getPosition();

  Serial.print("Went left.  Setting ");

  setOnTurn();
}

void setOnTurn() {
  Serial.print(modes[currentMode]);
  Serial.print(" to ");
  Serial.println(settings[currentMode]);

  analogWrite(stripPins[currentMode], settings[currentMode]);
}

// single click
void cycleMode(Button2& btn) {
  byte previousMode = currentMode;
  Serial.print("Switching from ");
  Serial.print(modes[previousMode]);

  if(previousMode == 3) {
    currentMode = 0;
  } else {
    currentMode = previousMode+1;
  }

  Serial.print(" to ");
  Serial.println(modes[currentMode]);

  r.resetPosition(settings[currentMode], false);

  Serial.println(settings[currentMode]);
}

// long click
void resetPosition(Button2& btn) {
    Serial.println("Resetting position");
    for(byte i=0;i<4;i++) {
      settings[i]=125;
      analogWrite(stripPins[i], settings[i]);
    }

    r.resetPosition(125, false);
}

void setup() {
  Serial.begin(115200);
  Serial.println("5050_strand_encoder starting up!");

  b.begin(BUTTON_PIN, INPUT_PULLUP, false);
  b.setLongClickHandler(cycleMode);
  b.setDoubleClickHandler(resetPosition);

  r.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS, START_POS, INCREMENT);
  r.resetPosition(settings[currentMode], false);
  r.setLeftRotationHandler(rotaryCounterClockwise);
  r.setRightRotationHandler(rotaryClockwise);

  pinMode(MASTER_STRIP_PIN, OUTPUT);
  pinMode(RED_STRIP_PIN, OUTPUT);
  pinMode(GREEN_STRIP_PIN, OUTPUT);
  pinMode(BLUE_STRIP_PIN, OUTPUT);

  for(byte i=0;i<4;i++) {
    analogWrite(stripPins[i],settings[i]);
  }
}

void loop() {
  r.loop();
  b.loop();
}
