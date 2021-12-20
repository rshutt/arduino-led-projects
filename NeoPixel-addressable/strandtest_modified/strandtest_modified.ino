#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <WM_Configer.h>

#if !(defined(ESP8266)) 
#error This code is intended to be run on the ESP8266
#endif

#define LED_PIN    D8
#define MIC_PIN    A0
#define LED_COUNT 300
#define BASE_BPM 174.0
#define MAX_EFFECT_MODE 14

int bpm;
float beatMillis;
int effectMode=0;

// This is for the color walking routines. Adding many LEDs has made this a much longer cycle.

int firstPixelHue = 0;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define HTTP_REST_PORT 8080
ESP8266WebServer server(HTTP_REST_PORT);

void restServerRouting() {
  server.on("/effectMode", HTTP_GET, []() {
    DynamicJsonDocument doc(512);

    doc["effectMode"] = effectMode;

    String buf;
    serializeJson(doc, buf);
    server.send(200, F("application/json"), buf);
  });
  server.on("/bpm", HTTP_GET, []() {
    DynamicJsonDocument doc(512);
    String buf;
    
    doc["bpm"] = bpm;
    serializeJson(doc, buf);
    server.send(200, F("application/json"), buf);
  });
  server.on("/bpm", HTTP_POST, []() {
    String postBody = server.arg("plain");

    DynamicJsonDocument doc(512);

    DeserializationError error = deserializeJson(doc, postBody);
    if(error) {
      Serial.print(F("Error parsing JSON "));
      Serial.println(error.c_str());

      String msg = error.c_str();

      server.send(400, F("text/html"),
        "Error parsing json body!<br>\n" + msg);
    } else {
     JsonObject postObj = doc.as<JsonObject>();
     String foo;
     serializeJson(postObj, foo);
     Serial.println(foo);

     DynamicJsonDocument responseDoc(512);

     if(postObj.containsKey("bpm")) {
      
     }
    }
  });
  server.on("/effectMode", HTTP_POST, []() {
    String postBody = server.arg("plain");

    DynamicJsonDocument doc(512);

    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
      Serial.print(F("Error parsing JSON "));
      Serial.println(error.c_str());
 
      String msg = error.c_str();
 
      server.send(400, F("text/html"),
        "Error parsing json body!<br>\n" + msg);
    } else {
      JsonObject postObj = doc.as<JsonObject>();
      String foo;
      serializeJson(postObj, foo);
      Serial.println(foo);

      DynamicJsonDocument responseDoc(512);
         
      if(postObj.containsKey("effectMode")) {
        int newEffectMode = postObj["effectMode"];

        if(newEffectMode >= 0 && newEffectMode <= MAX_EFFECT_MODE) {
          Serial.println("Setting effect mode to: " + String(newEffectMode));

          effectMode = newEffectMode;
          
          responseDoc["status"] = "OK";
          responseDoc["effectMode"] = newEffectMode;
          String buf;
          serializeJson(responseDoc, buf);
          server.send(201, F("application/json"), buf);
        } else {
          Serial.println("Bad newEffectMode received: " + String(newEffectMode));
          responseDoc["status"] = "FAIL";
          responseDoc["reason"] = "No such effect mode: " + String(newEffectMode);
          String buf;
          serializeJson(responseDoc, buf);
          server.send(400, F("application/json"), buf);         
        }
      } else {
        responseDoc["status"] = "FAIL";
        responseDoc["reason"] = "No effectMode argument";
        String buf;
        serializeJson(responseDoc, buf);
        server.send(400, F("application/json"),  buf);
      }
    }
  });
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

class WM_Configer * myWMConfiger;

void setup() {
  pinMode(MIC_PIN, INPUT);
  
  Serial.begin(115200);

  while(!Serial) {
    ;
  }

  Serial.print("Serial Ready!\n");

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS  

  myWMConfiger = new WM_Configer();

  myWMConfiger->config_tree();
  bpm = BASE_BPM;
  beatMillis = (1.0 / (BASE_BPM / 60.0)) * 1000.0;

  beatMillis = beatMillis/8;
  
  Serial.print("Beat should be every: ");
  Serial.print(beatMillis);
  Serial.print(" milliseconds\n");

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void handleSerialInput(char input) {
  Serial.print("Serial Received '" + String(input) + "'\n");

  if(input != -1) {
    if(input == '+' && effectMode <  MAX_EFFECT_MODE) {
      effectMode++; 
    } else if(input == '-' && effectMode > 0) {
      effectMode--;
    }
      
    Serial.print("Switching to effect mode ");
    Serial.print(effectMode);
    Serial.print("\n");
  }
}

void loop() {
  int micSensor = 0;

  myWMConfiger->dodrdloop();
  
  server.handleClient();
  
  micSensor = analogRead(MIC_PIN);
 
  if (Serial.available()) {
    handleSerialInput(Serial.read());
  }

  switch(effectMode) {
    case 0:
      color_walk_loop();
      break;
    case 1:
      greyscale_walk_loop();
      break;
    case 2:
      greyscale_random_loop();
      break;
    case 3:
      solid_color_walk_loop(0, 0, 255);
      break;
    case 4:
      solid_color_walk_loop(0, 255, 0);
      break;
    case 5:
      solid_color_walk_loop(255, 0, 0);
      break;
    case 6:
      nearly_solid_color(192, 192, 192);
      break;
    case 7:
      nearly_solid_color(192, 0, 0);
      break;
    case 8:
      nearly_solid_color(0, 192, 0);
      break;
    case 9:
      nearly_solid_color(0, 0, 192);
      break;
    case 10:
      solid_color(255, 255, 255);
      break; 
    case 11:
      breathe(255, 255, 255);
      break;
    case 12:
      breathe(255, 0, 0);
      break;
    case 13:
      breathe(0, 255, 0);
      break;
    case 14:
      breathe(0, 0, 255);
      break;
    default:
      // noop
      strip.clear();
      strip.show();
      delay(beatMillis * 4);
      break;
  }
}

struct loop { 
  unsigned long targetStartMillis;
  unsigned long actualStartMillis;
  unsigned long targetEndMillis;
  unsigned long actualEndMillis;
  unsigned long delayMillis;
};

void breathe(int red, int green, int blue) {
  Serial.print("Entering breathe with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n"); 

  strip.clear();
  float max_brightness = 200;
  float smoothness_pts = 192;
  float gamma = 0.18;
  float beta = 0.5;

  for(int a=0; a<smoothness_pts; a++) {
    unsigned long startMillis = millis();
    
    float intensity = (max_brightness - 20) * (exp(-(pow(((a/smoothness_pts)-beta)/gamma,2.0))/2.0));
    strip.setBrightness(intensity + 20);

    // Serial.print("Intensity is: ");
    // Serial.print(intensity);
    // Serial.print("\n");
      
    for(int c=0; c<strip.numPixels();c++) {
      strip.setPixelColor(c, red, green, blue);
    }

    strip.show();
    unsigned long endMillis = millis();
    float durationMillis = endMillis - startMillis;
    // Serial.println(durationMillis);
    float leftoverMillis = beatMillis - durationMillis;
    // Serial.println(leftoverMillis);
    
    // Serial.println(beatMillis);
    
    if(leftoverMillis > 0) {     
      delay(round(leftoverMillis));
    }
  }

  // Reset the max brightness!
  
  strip.setBrightness(255);
   
}


void solid_color(int red, int green, int blue) {
  Serial.print("Entering solid color with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n");

  if(red != 255 && green != 255 && blue != 255) {
    // Power can go crazy here!!! 
    if(red > 200) {
      red = 200;
    }

    if(green > 200) {
      green = 200;
    }

    if(blue > 200) {
      blue = 200;
    }
  }
  
  strip.clear();
  for(int c=0;c<strip.numPixels();c++) {
    strip.setPixelColor(c, red, green, blue);
  }
  strip.show();
  delay(beatMillis * 16);
}


void nearly_solid_color(int red, int green, int blue) {
  Serial.print("Entering nearly solid color with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n");

  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=0; c<strip.numPixels();c++) {
        if(random(3) != 0) {
          strip.setPixelColor(c+1, red, green, blue);
          c++;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, red, green, blue);
        }
      }
      strip.show();
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;
      
      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }
}

void solid_color_walk_loop(int red, int green, int blue) {
  Serial.print("Entering solid color walk loop with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n");
  
  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 1) {
        // Every 3rd pixel will be shifted forward 1
        // or maybe they just don't show at all?
        
        if(random(3) != 0) {
          strip.setPixelColor(c+1, red, green, blue);
          c+=4;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, red, green, blue);
        }
      }
      strip.show();
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;
      
      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }
}

void color_walk_loop() {
  Serial.println("Entering color_walk_loop");

  struct loop loops[16];

  unsigned long startRoutineMillis = millis();

  for(int i=0; i<16;i++) {
    loops[i].targetStartMillis = startRoutineMillis + (beatMillis * i);
    loops[i].targetEndMillis = loops[i].targetStartMillis + beatMillis;
  }

  int loopCtr=0;
  
  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++,loopCtr++) {
      if(millis() < loops[loopCtr].targetStartMillis) {
        Serial.println(String(millis()) + " < " + String(loops[loopCtr].targetStartMillis));
        delay(1);
      }

      loops[loopCtr].actualStartMillis = millis();
      
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 1) {
        int hue = (firstPixelHue + c * 65536L / strip.numPixels()) % 65536;
        // Serial.print("Pixel hue: ");
        // Serial.println(hue);
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); 

        // Every 3rd pixel will be shifted forward 1
        // or maybe they just don't show at all?
        
        if(random(3) != 0) {
          strip.setPixelColor(c+1, color);
          c+=4;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, color);
        }
      }
      strip.show();

      // This is what causes the overall "color walk" wherein it
      // walks the starting color through the entire strip, one 
      // LED at a time.
      
      firstPixelHue += 65536 / (LED_COUNT - 1);
      if(firstPixelHue >= 65536) {
        firstPixelHue = firstPixelHue % 65536;
      }

      loops[loopCtr].actualEndMillis = millis();

      if(loops[loopCtr].actualEndMillis > loops[loopCtr].targetEndMillis) {
        Serial.println("Falling behind: " + String(loops[loopCtr].actualEndMillis) + " > " + String(loops[loopCtr].targetEndMillis));
      }

      while(millis() < loops[loopCtr].targetEndMillis) {
        delay(1);
      } 
    }
  }

  Serial.println("Expected Loops: " + String((millis() - startRoutineMillis) / beatMillis) + " Actual Loops: " + String(loopCtr));
}

void greyscale_walk_loop() {
  Serial.println("Entering greyscale_walk_loop");
  int firstPixelHue = 0;
  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 1) {
        // Every 3rd pixel will be shifted forward 1
        // or maybe they just don't show at all?
        
        if(random(3) != 0) {
          strip.setPixelColor(c+1, 255, 255, 255);
          c+=4;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, 255, 255, 255);
        }
      }
      strip.show();

      // This is what causes the overall "color walk" wherein it
      // walks the starting color through the entire strip, one 
      // LED at a time.
      
      firstPixelHue += 65536 / (LED_COUNT - 1);
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;

      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      }     
    }
  }  
}

void greyscale_random_loop() {
  Serial.println("Entering greyscale_random_loop");

  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=0; c<strip.numPixels(); c += 1) {
        int greyscale = random(5);
        
        strip.setPixelColor(c, greyscale * (256/4), greyscale * (256/4), greyscale * (256/4));
      }
      strip.show();

      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;

      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }
}
