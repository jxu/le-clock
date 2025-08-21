/*
   Alarm clock
   3 modes (display, setting time, setting alarm)
*/

// Numeric constants
const char DIGITS   = 4;
const char SEGMENTS = 7;
const char BUTTONS  = 3;
const int  CYCLE_LEN = 1; // digit refresh

// Pin config
const char DIGIT_PINS[DIGITS]   = {2, 3, 4, 5};
const char SEG_PINS[SEGMENTS]   = {11, 10, 9, 8, 7, 13, 12}; // ABCDEFG seg
const char COLON_PIN            = 6;
const char SPEAKER_PIN          = A5;
const char BUTTON_PINS[BUTTONS] = {A0, A1, A2};

// Table of segments for each digit
// Display is active high (1 = on)
// For fun: store segment bits as bits in a char 0b0GFEDCBA
// saves some global variable memory compared to 2D bool array
const char DIGIT_SEG[10] = {
  0b0111111, // 0
  0b0000110, // 1
  0b1011011, // 2
  0b1001111, // 3
  0b1100110, // 4
  0b1101101, // 5
  0b1111101, // 6
  0b0000111, // 7
  0b1111111, // 8
  0b1101111, // 9
};


// Global variables
int                 digit_index      = 0; // 0-based for DIG1-DIG4
unsigned long       current_time     = 0; // ms time to convert into digits
unsigned long       time_offset      = 0; // for adjusting display time
unsigned long       alarm_time       = 6UL * 3600UL * 1000UL; // 06:00
unsigned long       cycle_start;
unsigned long       button_prev      = 0;

bool                pressed[BUTTONS];
const unsigned long MINUTE_MS        = 60 * 1000UL;
const unsigned long HOUR_MS          = 60 * MINUTE_MS;
const unsigned long DAY_MS           = 24 * HOUR_MS;
bool                debug_printed    = false;

const char MODES = 3;

enum Mode {
  MODE_DISPLAY = 0,
  MODE_SET_TIME = 1,
  MODE_SET_ALARM = 2,
};

Mode mode = MODE_SET_TIME;

// Calculate current digit from given time
int calc_current_digit(unsigned long t) {
  unsigned long current_time_sec = t / 1000;
  int current_min = (current_time_sec / 60) % 60;
  int current_hr = (current_time_sec / 3600) % 24;

  switch (digit_index) {
    case 0:
      return current_hr / 10;
    case 1:
      return current_hr % 10;
    case 2:
      return current_min / 10;
    case 3:
    default:
      return current_min % 10;
  }
}

// Write digit segments to the whichever digit the 7seg is on atm
void write_digit(int digit) {
  for (int i = 0; i < SEGMENTS; i++) {
    digitalWrite(SEG_PINS[i], (DIGIT_SEG[digit] >> i) & 1);
  }
}

void setup() {
  // Set display pins to output
  for (int i = 2; i <= 13; i++)
    pinMode(i, OUTPUT);

  // Use internal pull-up to not need external resistors
  for (int i = 0; i < BUTTONS; i++)
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);

  // set first snapshot
  cycle_start = millis();

  // Init speaker and serial debugging
  pinMode(SPEAKER_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // Every loop should call millis() only once to keep things in sync
  // Overflows after about 50 days
  // https://docs.arduino.cc/language-reference/en/functions/time/millis/
  
  unsigned long current_ms = millis();
  current_time = current_ms + time_offset;

  // DEBUG monitoring, but only once per 1000 ms

  if (!debug_printed && (current_time % 1000 == 0)) {
    Serial.print("millis ");
    Serial.print(current_ms);
    Serial.println();
    debug_printed = true;
  }

  // Reset flag
  if (current_time % 1000 > 500) {
    debug_printed = false;
  }

  // Read button presses every 50 ms for "debouncing"

  if (current_time - button_prev >= 50) {
    for (int i = 0; i < BUTTONS; i++) {
      // read active-low
      if (digitalRead(BUTTON_PINS[i]) == LOW)
      {
        // new press, as prev time was inactive
        if (pressed[i] == HIGH) {
          Serial.print("b");
          Serial.print(i);

          switch (i) {
            case 0: // mode button
              mode = Mode((mode + 1) % MODES);
              Serial.print("new mode ");
              Serial.print(mode);
              
              break;
            case 1:
              if (mode == MODE_SET_TIME) {
                // increment time by 1 hour
                time_offset = (time_offset + HOUR_MS) % DAY_MS;
              } 
              if (mode == MODE_SET_ALARM) {
                // increment alarm time by 1 hour
                alarm_time = (alarm_time + HOUR_MS) % DAY_MS;
              }
              break;

            case 2:
              if (mode == MODE_SET_TIME) {
                // increment time by 1 min
                time_offset = (time_offset + MINUTE_MS) % DAY_MS;  
              }
              if (mode == MODE_SET_ALARM) {
                // increment alarm time by 1 min
                alarm_time = (alarm_time + MINUTE_MS) % DAY_MS;
              }
              break;
          }
        }
        
        pressed[i] = LOW; // update button state
      } else {
        pressed[i] = HIGH;
      }
    }
    button_prev = current_time;
  }

  // Set alarm_playing if we've just reached alarm time
  // For one minute
  if ((current_time > alarm_time && 
       current_time < alarm_time + MINUTE_MS)) {
    if (current_time % 1000 > 500) // Alarm beep for half of a second
      tone(SPEAKER_PIN, 1000); // Hz
    else
      noTone(SPEAKER_PIN);
  } else {
    noTone(SPEAKER_PIN);
  }


  // Cycle digit if enough time has passed
  if (current_ms - cycle_start >= CYCLE_LEN) {

    // disable current digit index (active-low)
    digitalWrite(DIGIT_PINS[digit_index], HIGH);

    // change to next digit
    digit_index = (digit_index + 1) % DIGITS;

    // Based on mode, either write current or alarm time
    int digit;
    if (mode == MODE_SET_ALARM) {
      digit = calc_current_digit(alarm_time);
    } else {
      digit = calc_current_digit(current_time);
    }
    
    write_digit(digit);

    // enable digit (active-low)
    digitalWrite(DIGIT_PINS[digit_index], LOW);


    // Blink colon in display mode, otherwise leave on
    if (mode == MODE_DISPLAY) {
      bool colon = (current_ms % 1000 > 500) ? HIGH : LOW;
      digitalWrite(COLON_PIN, colon);
    } else {
      digitalWrite(COLON_PIN, HIGH);
    }

    cycle_start = current_ms; // reset cycle
  }
}
