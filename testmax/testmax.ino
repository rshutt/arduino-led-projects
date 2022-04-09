/*  MAX4466 Test Program
    Basic code to test the MAX4466 Microphone Preamplifier Module
    Connect to 3.3V, Gnd and Out goes to A0
    Connect 3.3V to AREF input to use 3.3V for ADC operation
    Open Serial Plotter window to observe the average audio amplitude.
*/
int const SAMPLE_WINDOW = 50; // Sample window width in mS (50 mS = 20Hz)
int const PREAMP_PIN = A0;      // Preamp output pin connected to A0 
unsigned int sample;
//===============================================================================
//  Initialization
//===============================================================================
void setup()
{
//   analogReference(EXTERNAL); // Connect 3.3V to AREF to provide reference voltage
   Serial.begin(9600);
}
//===============================================================================
//  Main
//=============================================================================== 
void loop()
{
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int amplitude = 0;   // peak-to-peak level
 
   unsigned int soundMax = 0;    
   unsigned int soundMin = 1024;
 
   // collect data for 50 mS and then plot data
   while (millis() - startMillis < SAMPLE_WINDOW)
   {
      sample = analogRead(PREAMP_PIN);
         if (sample > soundMax)
         {
            soundMax = sample;  // save the maximum levels
         }
         else if (sample < soundMin)
         {
            soundMin = sample;  // save the minimum levels
         }
      }
   amplitude = soundMax - soundMin;  // max - min = peak-peak amplitude
   Serial.print("Amplitude: ");
   Serial.print(amplitude);
   Serial.print(", Level: ");
   Serial.println(sample);

   }
