/*
   Alarm clock
   3 modes (display, setting time, setting alarm)
   Goes off at 06:00 for a minute
*/

// TODO:
// - Blinking colon
// - user-friendly Adjust time and alarm

// Numeric constants
const char DIGITS   = 4;
const char SEGMENTS = 7;
const char BUTTONS  = 3;
const int  CYCLE_LEN = 1; // digit refresh

// Pin config
const char DIGIT_PINS[DIGITS]   = {2, 3, 4, 5};
const char SEG_PINS[SEGMENTS]   = {11, 10, 9, 8, 7, 13, 12};
const char COLON_PIN            = 6;
const char BUZZER_PIN           = A5;
const char BUTTON_PINS[BUTTONS] = {A0, A1, A2};

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
  { 1, 1, 1, 1, 0, 1, 1 }
};

// Globals for displaying time
int                 digit_index      = 0; // 0-based for DIG1-DIG4
unsigned long       current_time     = 0;
unsigned long       time_offset      = 0; // for adjusting display time
unsigned long       alarm_time       = 6UL * 3600UL * 1000UL; // 06:00
unsigned long       cycle_start;
bool                alarm_set        = true;
bool                alarm_playing    = false;
// TODO: remove or clean up this
unsigned long       button_interval  = 0;
const long DAY_MS                    = 1000L * 3600L * 24L;
bool                debug_printed    = false;

const char MODES = 3;

enum Mode {
  MODE_DISPLAY = 0,
  MODE_SET_TIME = 1,
  MODE_SET_ALARM = 2,
};

Mode mode = MODE_DISPLAY;

// Calculate current digit from time
int calc_current_digit() {
  unsigned long current_time_sec = current_time / 1000;
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
    digitalWrite(SEG_PINS[i], DIGIT_SEG[digit][i]);
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

  // Init buzzer and serial debugging
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // Every loop should call millis() only once to keep things in sync
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

  // Read button presses

  for (int i = 0; i < BUTTONS; i++) {
    // Debug
    if ((millis() - button_interval > 300ul)) {


      /*
        if (dread) {
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
          mode = Mode((mode + 1) % MODES);
          Serial.print("Mode ");
          Serial.println(mode);
        }

        button_interval = millis();
        }
      */
    }
  }



  // Set alarm_playing if we've just reached alarm time
  // For one minute
  if (alarm_set && 
      (current_time > alarm_time && current_time < alarm_time + 60000UL))
    alarm_playing = true;
  else alarm_playing = false;


  if (alarm_playing) {
    if (current_time % 1000 > 500) // Alarm beep for half of a second
      tone(BUZZER_PIN, 1000); // Hz
    else
      noTone(BUZZER_PIN);
  }

  // Cycle digit if enough time has passed
  if (current_ms - cycle_start >= CYCLE_LEN) {

    // disable current digit index (active-low)
    digitalWrite(DIGIT_PINS[digit_index], HIGH);

    // change to next digit
    digit_index = (digit_index + 1) % DIGITS;

    int digit = calc_current_digit();
    write_digit(digit);

    // enable digit (active-low)
    digitalWrite(DIGIT_PINS[digit_index], LOW);

    cycle_start = current_ms; // reset cycle start
  }

  // Always show colon for now
  digitalWrite(COLON_PIN, HIGH);
}
