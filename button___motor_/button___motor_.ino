#include <ESP32Encoder.h>
#define BIN_1 26
#define BIN_2 25
#define LED_PIN 13
#define BTN 15
#define PMP 12

ESP32Encoder encoder;

//Setup interrupt variables ----------------------------
volatile int count = 0; // encoder count
hw_timer_t * timer2 = NULL;

volatile bool buttonIsPressed = false; 
uint64_t time_val;

// setting PWM properties ----------------------------
const int freq = 5000;
const int ledChannel_1 = 1;
const int ledChannel_2 = 2;
const int resolution = 8;
int MAX_PWM_VOLTAGE = 255;
int motor_PWM;
int val = 0;

// encoder properties ------------------------------
int v = 0;


//Initialization ------------------------------------
void IRAM_ATTR isr() {  // the function to be called when interrupt is triggered
  buttonIsPressed = true;   
}



void setup() {
  pinMode(LED_PIN, OUTPUT); // configures the specified pin to behave either as an input or an output
  pinMode(PMP, OUTPUT);
  digitalWrite(LED_PIN, LOW); // sets the initial state of LED as turned-off

  pinMode(BTN, INPUT);  // configures the specified pin to behave either as an input or an output
  attachInterrupt(BTN, isr, RISING);  // set the "BTN" pin as the interrupt pin; call function named "isr" when the interrupt is triggered; "Rising" means triggering interrupt when the pin goes from LOW to HIGH
  timer2 = timerBegin(0, 80, true);
  
  Serial.begin(115200);
  ESP32Encoder::useInternalWeakPullResistors = UP; // Enable the weak pull up resistors
  encoder.attachHalfQuad(27, 33); // Attache pins for use as encoder pins
  encoder.setCount(0);  // set starting count value after attaching

  // configure LED PWM functionalitites
  ledcSetup(ledChannel_1, freq, resolution);
  ledcSetup(ledChannel_2, freq, resolution);

  // attach the channel to the GPIO to be controlled 
  ledcAttachPin(BIN_1, ledChannel_1);
  ledcAttachPin(BIN_2, ledChannel_2);
}

void loop() {
  
  val = digitalRead(BTN); 
  Serial.println(val);
  if (val ==1) {
    ButtonResponse();
    digitalWrite(PMP, LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
    ledcWrite(ledChannel_2, LOW);
    digitalWrite(PMP, HIGH);
}
}

bool CheckForButtonPress() {
  if (!timerStarted(timer2) and buttonIsPressed) {
    timerStart(timer2);
  } else if (timerStarted(timer2)) {
    time_val = timerReadMilis(timer2);
    if (time_val == 150) {
      timerStop(timer2);
      timerRestart(timer2);
      return true;
    }
  }
  return false;
  //return buttonIsPressed;
}

void ButtonResponse() {
  buttonIsPressed = false;
  digitalWrite(LED_PIN, HIGH);
  ledcWrite(ledChannel_1, LOW);
  ledcWrite(ledChannel_2, MAX_PWM_VOLTAGE);
}
