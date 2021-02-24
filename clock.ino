/*
 * Alarm clock
 * 3 modes (display, setting time, setting alarm)
 * Goes off at 06:00 for a minute 
 * 
 * Tone library from https://github.com/bhagman/Tone
 */ 

#include <Tone.h>

#define A_THRESH      500
#define BUTTONS       3
#define MODES         3
#define BUZZER_PIN    6


const int DIGIT_PINS[4]        = {2, 3, 4, 5};
const int SEG_PINS[7]          = {11, 10, 9, 8, 7, 13, 12};
const int BUTTON_PINS[BUTTONS] = {A0, A1, A2};
const unsigned long DAY_MS     = (1000 * 3600 * 24);


// Table of segments for each digit
// Display is active high (1 = on) 
const bool DIGIT_SEG[10][7] = {{ 1, 1, 1, 1, 1, 1, 0 },
                               { 0, 1, 1, 0, 0, 0, 0 },
                               { 1, 1, 0, 1, 1, 0, 1 },
                               { 1, 1, 1, 1, 0, 0, 1 },
                               { 0, 1, 1, 0, 0, 1, 1 },
                               { 1, 0, 1, 1, 0, 1, 1 },
                               { 1, 0, 1, 1, 1, 1, 1 },
                               { 1, 1, 1, 0, 0, 0, 0 },
                               { 1, 1, 1, 1, 1, 1, 1 },
                               { 1, 1, 1, 1, 0, 1, 1 }};

// Globals for displaying time
// TODO: change current_digit to not be global!
int                 current_digit;
unsigned long       current_time     = 0;
unsigned long       current_time_day = 0;
unsigned long       time_offset      = 0;
unsigned long       alarm_time       = 6UL * 3600UL * 1000UL; // 06:00 
unsigned long       intervalStart;
const unsigned long intervalLength   = 1; // for digit refresh
bool                alarm_set        = true;
bool                alarm_playing    = false;
unsigned long       button_interval  = 0;

Tone tone1;

typedef enum {
  MODE_DISPLAY,
  MODE_SET_TIME,
  MODE_SET_ALARM,
} mode_t;

mode_t mode = MODE_DISPLAY; 


void setup() {
  // Set display pins, buzzer pin to output
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }

  for (int i = 0; i < BUTTONS; i++) 
    pinMode(BUTTON_PINS[i], INPUT);
  
  current_digit = 3;
  intervalStart = millis();

  // Init buzzer and serial debugging
  tone1.begin(BUZZER_PIN);
  Serial.begin(115200);
}

void loop() {
  // Write millis, but not too often 
  // DEBUG
  
  if (millis() % 1000 == 0) {
    Serial.println(millis());
    Serial.print("Time offset: ");
    Serial.println(time_offset);

    Serial.println(button_interval);
    Serial.println(millis() - button_interval);
  }

  // Read button presses
  for (int i=0; i<BUTTONS; i++) {
    
    // Debug
    if ((millis() - button_interval > 300ul)) {
      int aread = analogRead(BUTTON_PINS[i]);
      
      if (aread > A_THRESH) {
        Serial.print("Button pressed: ");
        Serial.println(i);

        if (i == 0) {       
          
          if (mode == MODE_SET_TIME) 
            time_offset += 60000ul;
        }
        else if (i == 1) { 
            if (mode == MODE_SET_TIME)
              time_offset += 3600000ul;
        }
        else if (i == 2) {
          mode = (mode + 1) % MODES; 
          Serial.print("Mode ");
          Serial.println(mode);
        }

        button_interval = millis();
      }
    } 
  }


  current_time = millis() + time_offset;
  current_time_day = current_time ;

  // Set alarm_playing if we've just reached alarm time 
  // For one minute
  if (alarm_set && (current_time_day > alarm_time && current_time_day < alarm_time + 60000UL))
    alarm_playing = true;
  else alarm_playing = false;

  if (alarm_playing) {
    tone1.play(NOTE_A5);
  } else {
    tone1.stop();
  }

  // Always display time (non-blocking)
  display_time();

}

/* Helper functions */

// Non-blocking function to display the time
// Uses millis instead of delay
void display_time() {
  if ((millis() - intervalStart) >= intervalLength) {

    current_digit = (current_digit + 1) % 4;
    unset_digit_pins();
    write_current_digit(current_time);
    digitalWrite(DIGIT_PINS[current_digit], LOW);
    intervalStart = millis();
  }
}

// Set all digit pins to high (active-low)
void unset_digit_pins() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(DIGIT_PINS[i], HIGH);
  }
}

// Writes current digit
void write_current_digit(unsigned long current_time) {
  int digit;
  unsigned long current_time_sec = current_time / 1000;
  int current_min = (current_time_sec / 60) % 60;
  int current_hr = (current_time_sec / 3600) % 24;
  
  switch (current_digit) {
    case 0:
      digit = (current_hr / 10) ;
      break;
    case 1:
      digit = current_hr % 10;
      break;
    case 2:
      digit = current_min / 10;
      break;
    default:
      digit = current_min % 10;
  }
  write_digit(digit);
}

// Writes a digit to the seven-segment display
void write_digit(int digit) {
  for (int i = 0; i < 7; i++) {
    digitalWrite(SEG_PINS[i], DIGIT_SEG[digit][i]);
  }
}
