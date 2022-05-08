
#include "Button2.h" //  https://github.com/LennartHennigs/Button2
#include "ESPRotary.h"

#define DT_PIN D7
#define CLK_PIN D6
#define SW_PIN D5

#define ROTARY_PIN1 CLK_PIN
#define ROTARY_PIN2 DT_PIN
#define BUTTON_PIN SW_PIN

#define CLICKS_PER_STEP 1
#define MAX_POS 255
#define START_POS 0
#define INCREMENT 1

ESPRotary r;
Button2 b;

void setup() {
  Serial.begin(9600);
  Serial.println("5050_strand_encoder starting up!");

  b.begin(BUTTON_PIN);
  b.setTapHandler(buttonPress);
  b.setLongClickHandler(resetPosition);

  r.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS, START_POS, INCREMENT)
  r.setChangedHandler(encoderRotate);
  r.setLeftRotationHandler(showDirection);
  r.setRightRotationHandler(showDirection);
}

void loop() {
  r.loop;
  b.loop;
}

// on change
void encoderRotate(ESPRotary& r) {
   Serial.println(r.getPosition());
}

// on left or right rotation
void showDirection(ESPRotary& r) {
  Serial.println(r.directionToString(r.getDirection()));
}

// single click
void buttonPress(Button2& btn) {
  Serial.println("Click!");
}

// long click
void resetPosition(Button2& btn) {
  r.resetPosition();
  Serial.println("Reset!");
}
