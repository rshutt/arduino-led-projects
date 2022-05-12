
#include "Button2.h" //  https://github.com/LennartHennigs/Button2
#include "ESPRotary.h"

#define DT_PIN D0
#define CLK_PIN D6
#define SW_PIN D5

#define ROTARY_PIN1 DT_PIN
#define ROTARY_PIN2 CLK_PIN
#define BUTTON_PIN SW_PIN

#define RED_REF_PIN D3
#define GREEN_REF_PIN D1
#define BLUE_REF_PIN D2

#define RED_STRIP_PIN D8
#define GREEN_STRIP_PIN D4
#define BLUE_STRIP_PIN D7

#define CLICKS_PER_STEP 1
#define MAX_POS 255
#define MIN_POS 0
#define START_POS 0
#define INCREMENT 4

ESPRotary r;
Button2 b;
int currentMode=0;

const char * modes[] = {
  "red",
  "green",
  "blue"
};

int settings[] = {
  127,
  127,
  127
};

byte refPins[] = {
  RED_REF_PIN,
  GREEN_REF_PIN,
  BLUE_REF_PIN
};

byte stripPins[] = {
  RED_STRIP_PIN,
  GREEN_STRIP_PIN,
  BLUE_STRIP_PIN
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

  analogWrite(refPins[currentMode], settings[currentMode]);
  analogWrite(stripPins[currentMode], settings[currentMode]);
}

// single click
void cycleMode(Button2& btn) {
  int previousMode = currentMode;
  Serial.print("Switching from ");
  Serial.print(modes[previousMode]);

  analogWrite(refPins[previousMode], 0);

  if(previousMode == 2) {
    currentMode = 0;
  } else {
    currentMode = previousMode+1;
  }

  r.resetPosition(settings[currentMode], false);
  analogWrite(refPins[currentMode], settings[currentMode]);

  Serial.print(" to ");
  Serial.println(modes[currentMode]);
}

// long click
void resetPosition(Button2& btn) {
    Serial.println("Resetting position");
    for(int i=0;i<3;i++) {
      settings[i]=127;
      analogWrite(stripPins[i], settings[i]);
    }

    r.resetPosition(127, false);
    analogWrite(refPins[currentMode], settings[currentMode]);
}

void setup() {
  Serial.begin(9600);
  Serial.println("5050_strand_encoder starting up!");

  b.begin(BUTTON_PIN, INPUT_PULLUP, false, false);
  b.setTapHandler(cycleMode);
  b.setLongClickHandler(resetPosition);

  r.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS, START_POS, INCREMENT);
  r.resetPosition(settings[currentMode], false);
  r.setLeftRotationHandler(rotaryCounterClockwise);
  r.setRightRotationHandler(rotaryClockwise);

  pinMode(RED_REF_PIN, OUTPUT);
  pinMode(GREEN_REF_PIN, OUTPUT);
  pinMode(BLUE_REF_PIN, OUTPUT);

  analogWrite(refPins[currentMode],settings[currentMode]);
}

void loop() {
  r.loop();
  b.loop();
}
