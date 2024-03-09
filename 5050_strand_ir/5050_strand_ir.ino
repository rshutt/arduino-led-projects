#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define IR_INPUT_PIN D1
#define RED_STRIP_PIN D5
#define GREEN_STRIP_PIN D6
#define BLUE_STRIP_PIN D7

#define CLICKS_PER_STEP 5
#define MAX_POS 255
#define MIN_POS 0
#define START_POS 0
#define INCREMENT 5

#define REMOTE_UP 0xFF18E7
#define REMOTE_DOWN 0xFF4AB5
#define REMOTE_RIGHT 0xFF5AA5
#define REMOTE_LEFT 0xFF10EF
#define REMOTE_OK 0xFF38C7
#define REMOTE_ONE 0xFFA25D
#define REMOTE_TWO 0xFF629D
#define REMOTE_THREE 0xFFE21D
#define REMOTE_FOUR 0xFF22DD


#define VISUAL_SOLID 1
#define VISUAL_BREATHE 2
#define VISUAL_FASTBLINKING 3
#define VISUAL_BLINKING 4

byte currentVisual=VISUAL_SOLID;
byte currentMode=0;
IRrecv irrecv(IR_INPUT_PIN);
decode_results irresults;
uint64_t irlast=0;
bool debug=true;
bool off=false;
float beatMillis;
float barStartMillis = 0;
int beats_per_measure = 4;
int measures_per_bar = 8;

const char * visuals[] = {
  "",
  "solid",
  "breathing",
  "fast_blinking",
  "blinking",
  "rotating"
};

const char * modes[] = {
  "red",
  "green",
  "blue"
};

byte color_settings[] = {
  125,
  125,
  125
};

byte stripPins[] = {
  RED_STRIP_PIN,
  GREEN_STRIP_PIN,
  BLUE_STRIP_PIN,
};

int curBeat=0;

void setup() {
  Serial.begin(74880);
  Serial.println("5050_strand_ir starting up!");

  irrecv.enableIRIn();
  Serial.print("Started IR Receiver on pin ");
  Serial.print(IR_INPUT_PIN);
  Serial.print("\n");

  pinMode(RED_STRIP_PIN, OUTPUT);
  pinMode(GREEN_STRIP_PIN, OUTPUT);
  pinMode(BLUE_STRIP_PIN, OUTPUT);

  for(byte i=0;i<3;i++) {
    Serial.print("Writing pin: ");
    Serial.println(stripPins[i]);
    analogWrite(stripPins[i],color_settings[i]);
  }

  beatMillis = 60.0/174.0 * 1000.0;
  Serial.print("Millis per beat is: ");
  Serial.print(beatMillis);
  Serial.println("ms");
  Serial.println("Setup finished - loop starting");
}

void loop() {
  uint64_t command;
  bool changed;
  bool dup=false;

  float durationMillis=beatMillis;
  float targetEndMillis = 0;
  float beatStartMillis = millis();

  if(curBeat == 0) {
    barStartMillis = beatStartMillis;
  }

  changed=false;
  // Serial.println("Starting loop");
  if (irrecv.decode(&irresults)) {
    // print() & println() can't handle printing long longs. (uint64_t)
    if(irresults.value == 0xFFFFFFFFFFFFFFFF) {
      if(debug) {
        Serial.print("Received repeat for last command: ");
        serialPrintUint64(irlast, HEX);
        Serial.println("");
      }
      command = irlast;
      dup=true;
    } else {
      if(debug) {
        Serial.print("Received command: ");
        serialPrintUint64(irresults.value, HEX);
        Serial.println("");
      }
      command = irresults.value;
      irlast = command;
    }

    switch(command) {
      int incr;

      case REMOTE_ONE:
        if(currentVisual != 1) {
          currentVisual = 1;
          Serial.print("Changing visual to ");
          Serial.print(visuals[currentVisual]);
          Serial.println("");

          changed = true;
        }

        break;

      case REMOTE_TWO:
        if(currentVisual != 2) {
          currentVisual = 2;
          Serial.print("Changing visual to ");
          Serial.print(visuals[currentVisual]);
          Serial.println("");

          changed = true;
        }

        break;

      case REMOTE_THREE:
        if(currentVisual != 3) {
          currentVisual = 3;
          Serial.print("changing visual to ");
          Serial.println(visuals[currentVisual]);
          changed = true;
        }

      case REMOTE_FOUR:
        if(currentVisual != 4) {
          currentVisual = 4;
          Serial.print("changing visual to ");
          Serial.println(visuals[currentVisual]);
          changed = true;
        }

        break;

      case REMOTE_OK:
        if(off) {
          off = false;
          changed = true;
          Serial.println("Remote OK pressed.  Toggling on/off state to on");
        } else {
          off = true;
          changed = true;
          Serial.println("Remote OK pressed.  Toggling on/off state to off");
        }

        break;

      case REMOTE_UP:
        incr = dup ? 10 : 1;

        if(color_settings[currentMode] + incr <= 255) {
          color_settings[currentMode]+=incr;
          changed=true;
        } else {
          if(color_settings[currentMode] != 255) {
            color_settings[currentMode]=255;
            changed=true;
          }
        }

        Serial.print("Remote UP pressed.  color_settings[");
        Serial.print(modes[currentMode]);
        Serial.print("] now ");
        Serial.print(color_settings[currentMode]);
        Serial.println("");

        break;

      case REMOTE_DOWN:
        incr = dup ? 10 : 1;
        if(color_settings[currentMode] - incr >= 0) {
          color_settings[currentMode]-=incr;
          changed=true;
        } else {
          if(color_settings[currentMode] != 0) {
            color_settings[currentMode]=0;
            changed=true;
          }
        }

        Serial.print("Remote DOWN pressed.  color_settings[");
        Serial.print(modes[currentMode]);
        Serial.print("] now ");
        Serial.print(color_settings[currentMode]);
        Serial.println("");

        break;

      case REMOTE_RIGHT:
        if(currentMode < 2) {
          currentMode++;
        } else {
          currentMode = 0;
        }
        Serial.print("Remote RIGHT pressed.  Current mode now: ");
        Serial.print(modes[currentMode]);
        Serial.println("");

        break;

      case REMOTE_LEFT:
        if(currentMode > 0) {
          currentMode--;
        } else {
          currentMode = 2;
        }
        Serial.print("Remote LEFT pressed.  Current mode now: ");
        Serial.print(modes[currentMode]);
        Serial.println("");

        break;

      default:
        Serial.print("Unknown command recceived: ");
        serialPrintUint64(command, HEX);
        Serial.println("");
        break;
    }
    irrecv.resume();  // Receive the next value
  }

 if (currentVisual == VISUAL_SOLID) {
    if(changed) {
      for(int colorNum=0;colorNum<=2;colorNum++) {
        int target = off ? 0 : color_settings[colorNum];
        if(debug) {
          Serial.print("Setting stripPin[");
          Serial.print(colorNum);
          Serial.print("] to: ");
          Serial.print(target);
          Serial.println("");
        }
        analogWrite(stripPins[colorNum], target);
      }
    }

    curBeat++;

  } else if(currentVisual == VISUAL_BREATHE) {
    float subBeatFraction = 32.0;
    float smoothness_pts = beats_per_measure * (measures_per_bar) * subBeatFraction;

    float gamma = 0.20;
    float beta = 0.5;

    for(int subBeat=0;subBeat<subBeatFraction;subBeat++) {
      int smoother = (curBeat*subBeatFraction)+subBeat;
      float intensity = (exp(-(pow(((smoother/smoothness_pts)-beta)/gamma,2.0))/2.0));

      //if(debug) {
        Serial.print("Current Beat: ");
        Serial.print(curBeat);
        Serial.print(" subBeat: ");
        Serial.print(subBeat);
        Serial.print(" Desired intensity: ");
        Serial.println(intensity);
      //}

      for(int colorNum=0;colorNum<=2;colorNum++) {
        int target = off ? 0 : color_settings[colorNum]*intensity;
        if(debug) {
          Serial.print("Setting stripPin[");
          Serial.print(colorNum);
          Serial.print("] to: ");
          Serial.print(target);
          Serial.println("");
        }

        analogWrite(stripPins[colorNum], target);
      }
      targetEndMillis=barStartMillis + (((curBeat*subBeatFraction)+subBeat) *
          (durationMillis/subBeatFraction));

      float now=millis();

      float leftOver=targetEndMillis-now;

      if(leftOver > 0) {
       // if(debug) {
          Serial.print("Delaying ");
          Serial.print(leftOver);
          Serial.println("ms to catch subBeat");
        //}
        delay(leftOver);
      }
    }

    // don't forget to track the beat we ate with the subbeat.
    curBeat++;
  } else if (currentVisual == VISUAL_BLINKING || currentVisual == VISUAL_FASTBLINKING) {
    int evensonly;

    if(currentVisual == VISUAL_FASTBLINKING) {
      evensonly=1;
    } else {
      evensonly=2;
    }

    for(int loopBeat=0;loopBeat<beats_per_measure*measures_per_bar;loopBeat++) {
      if(loopBeat % evensonly == 0) {
      // Toggle
        if(off == true) {
          off = false;
          if(debug) {
            Serial.println("Blink on");
          }
        } else {
          off = true;

          if(debug) {
            Serial.println("Blink off");
          }
        }

        for(int colorNum=0;colorNum<=2;colorNum++) {
          int target = off ? 0 : color_settings[colorNum];
          if(debug) {
            Serial.print("Setting stripPin[");
            Serial.print(colorNum);
            Serial.print("] to: ");
            Serial.print(target);
            Serial.println("");
          }
          analogWrite(stripPins[colorNum], target);
        }

      }

      curBeat++;

      targetEndMillis=barStartMillis + (curBeat*durationMillis);

      float now=millis();

      float leftOver=targetEndMillis-now;

      if(leftOver > 0) {
        // if(debug) {
        Serial.print("Delaying ");
        Serial.print(leftOver);
        Serial.println("ms to catch beat");
        //}
      delay(leftOver);
      }
    }
  }


  if(curBeat >= beats_per_measure * measures_per_bar) {
    curBeat=0;
  }

  //if(debug) {
    Serial.print("barStartMillis: ");
    Serial.print(barStartMillis);
    Serial.print(" curBeat: ");
    Serial.print(curBeat);
    Serial.print(" durationMillis: ");
    Serial.println(durationMillis);
  //}

  targetEndMillis=barStartMillis + (curBeat*durationMillis);

  float now=millis();

  float leftOver=targetEndMillis-now;

  Serial.print("Final delay of: ");
  Serial.print(leftOver);
  Serial.println("ms to catch beat");

  if(leftOver > 0) {
    delay(leftOver);
  }

  now=millis();
  Serial.print("Total beat took: ");
  Serial.print(now-beatStartMillis);
  Serial.println(" ms");
}
