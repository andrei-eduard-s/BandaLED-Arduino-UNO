#include <FastLED.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#define NUM_LEDS 30 // Numarul de LED-uri din banda LED RGB
#define DATA_PIN 8 // Data pin-ul la care este conectata banda LED
#define potPin A2 // Potentiometrul conectat la pinul analog A2
#define photoresistorPin A3 // Fotorezistenta conectata la pinul analog A3
#define buttonPin 2 // Butonul conectat la pinul digital 2
SoftwareSerial bluetooth(12, 13); // Definim pinii software serial (TX, RX) 
char sir[8]; // sirul hex pe care il primimi prin bluetooth 
int counter = 0;  // numarator pentru sirul primit
int adc_value = 0; //  valoarea citită din conversia analogic-digitală (ADC)
uint32_t hexvalue = 0; // valoarea hexazecimală compusă
int brightness = 0; // variabila globala pentru luminozitate 

// Funcție pentru compunerea unei valori hexazecimale din caractere
uint32_t composeHexFromChars(char * chars) {
   return strtol(chars, NULL, 16);
}

// Definirea instanțelor pentru banda LED și senzorul de temperatură
CRGB leds[NUM_LEDS]; // Definim OneWire si DallasTemperature
OneWire oneWire(4);
DallasTemperature sensors( & oneWire);

// Funcția de inițializare a configurării
void setup() {
   Serial.begin(9600);
   FastLED.addLeds < WS2812, DATA_PIN, GRB > (leds, NUM_LEDS);
   FastLED.setBrightness(255); // valorea initiala de luminozitate a benzii LED
   pinMode(potPin, INPUT);
   pinMode(photoresistorPin, INPUT);
   pinMode(buttonPin, INPUT_PULLUP);
   sensors.begin(); // Initializam senzorul de temperatura
   bluetooth.begin(9600); // Initializam comunicatia  software serial 
   delay(2000); // Asteptam un timp pentru a initializa modulul bluetooth HC-05
   ADMUX = 0x00;
   ADMUX |= (1 << REFS0); // selectie referinta ca fiind Vcc
   ADMUX |= (1 << MUX1); // selectie canal 2
   ADCSRA |= (1 << ADIE); // pornire intreruperi
   ADCSRA |= 0b00000111; // prescaler 128
   ADCSRA |= (1 << ADEN); // selectare ADC
}

// Funcția de întrerupere pentru conversia ADC
ISR(ADC_vect) {
   adc_value = ADC;
   Serial.println(adc_value);
}

// Functia principala
void loop() {
    // Recepționarea datelor de pe bluetooth și compunerea valorii hexazecimale
   if (bluetooth.available()) {
      char receivedChar =
         bluetooth.read();
      sir[counter++] = receivedChar;
   } else {
      counter = 0;
      hexvalue = composeHexFromChars(sir);
   }

   // Citim starea butonului: 
   bool buttonState = digitalRead(buttonPin);
   int brightness;


   if (buttonState) {
      ADMUX &= ~(1 << MUX0);
      ADCSRA |= (1 << ADSC);
      brightness
         = map(adc_value, 0, 1023, 0, 255);
      if (adc_value == 0.5) {
         FastLED.clear();
         FastLED.show();
      }
      sensors.requestTemperatures(); // Obtinem temperatura de la senzor 
      float temperature = sensors.getTempCByIndex(0);
      // Setarea culorii LED-urilor în funcție de temperatură
      if (temperature < 18) {
         fill_solid(leds, NUM_LEDS, CRGB::Purple);
      } else if (temperature >= 18 && temperature < 20) {
         fill_solid(leds,
            NUM_LEDS, CRGB::Blue);
      } else if (temperature >= 20 && temperature < 22) {
         fill_solid(leds,
            NUM_LEDS, CRGB::Green);
      } else if (temperature >= 22 && temperature < 24) {
         fill_solid(leds,
            NUM_LEDS, CRGB::Gold);
      } else if (temperature >= 24 && temperature < 26) {
         fill_solid(leds,
            NUM_LEDS, CRGB::Orange);
      } else if (temperature >= 26) {
         fill_solid(leds, NUM_LEDS, CRGB::Red);
      }
   } else {
    // Setarea ADC pentru fotorezistor
      ADMUX |= (1 << MUX0);
      ADCSRA |= (1 << ADSC);

      // Setarea luminozității în funcție de valoarea citită de la fotorezistor
      // (logica pare să fie invers proporțională cu luminozitatea)
      if (adc_value < 70) {
         brightness = 255; // Luminozitate maxima
      } else if (adc_value >= 70 && adc_value < 190) {
         brightness = map(adc_value, 70, 190, 255, 128); // Interval 70-190: 100% -> 50% luminozitate
      } else if (adc_value >= 190 && adc_value < 280) {
         brightness = map(adc_value, 190, 280, 128, 64); // Interval 190-280: 50% -> 25% luminozitate
      } else if (adc_value >= 280 && adc_value < 400) {
         brightness = map(adc_value, 280, 400, 64, 0); // Interval 280-400: 25%-> 0% luminozitate
      } else {
         brightness = 0; // Luminozitate minima pentru valori peste 400
      } 
      
      // Verificare și setare a culorilor LED-urilor în absența datelor de la Bluetooth
      if (hexvalue >= 5) fill_solid(leds, NUM_LEDS, hexvalue);
      else 
        {
          static bool alternateColors = false;
          static unsigned long previousMillis = 0;
          const long interval = 500; // Intervalul de timp în milisecunde între schimbarea culorilor
          unsigned long currentMillis = millis();

          if (currentMillis - previousMillis >= interval) {
          previousMillis = currentMillis;

          // Schimbarea între culori
          if (alternateColors) {
            // Culorile pentru LED-uri pare
            for (int i = 0; i < NUM_LEDS; i += 2) {
                leds[i] = CRGB::Green;
            }

            // Culorile pentru LED-uri impare
            for (int i = 1; i < NUM_LEDS; i += 2) {
                leds[i] = CRGB::Red;
            }
        } else {
            // Culorile inverse pentru LED-uri pare
            for (int i = 0; i < NUM_LEDS; i += 2) {
                leds[i] = CRGB::Red;
            }

            // Culorile inverse pentru LED-uri impare
            for (int i = 1; i < NUM_LEDS; i += 2) {
                leds[i] = CRGB::Green;
            }
        }

        // Schimbăm starea pentru următoarea iterație
        alternateColors = !alternateColors;
        }
      }
   }
   // Aprindem banda LED RGB:
   FastLED.setBrightness(brightness);
   FastLED.show();
   delay(10);
}