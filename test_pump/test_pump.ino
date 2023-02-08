#include <ESP32Encoder.h>
#include <IRremote.h>
#include <ezButton.h>

#define PMP 12
#define POT 32
#define IR 14
#define TRIG 18   //ultrasonic transmitter pin
#define ECHO 19  //ultrasonic receiver pin

#define RPWM 5 // define pin 3 for RPWM pin (output)
#define R_EN 25 // define pin 2 for R_EN pin (input)

#define LPWM 4 // define pin 6 for LPWM pin (output)
#define L_EN 26 // define pin 7 for L_EN pin (input)

ezButton SW1(27);
ezButton SW2(33);

hw_timer_t * timer = NULL; 
uint64_t time_val;


int pot = 0;  
int pot_pwm = 0; 
int timing = 0;

//Setup IR receiver ----------------------------
IRrecv irrecv(IR);
decode_results results;
unsigned long last_signal = 0;

const float sound_speed = 0.034;
long duration = 0;
float distance = 0; //cm
int count = 0;


void setup() {
  // put your setup code here, to run once:
  pinMode(PMP, OUTPUT); // declares pin 12 as output
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  
  pinMode(POT, INPUT);  // declares pin A0 as input
  pinMode(IR, INPUT);

  pinMode(TRIG, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO, INPUT);

  
  irrecv.enableIRIn();
  timer = timerBegin(0, 80, true); //timer for water pump
  SW1.setDebounceTime(50);
  SW2.setDebounceTime(50);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  pot = analogRead(POT);
  pot_pwm = map(pot, 0, 4095, 0, 255);
  timing = millis();
//
//  if (irrecv.decode(&results)){
//    Serial.print("Signal: ");
//    Serial.println(results.value, HEX);
//
//    irrecv.resume();  
//  }
//
//  current_dist();
////
//  analogWrite(PMP, pot/4) lp=;

//  if (timing <= 5000) {
//    Serial.println("watering");
//    
//    digitalWrite(PMP, HIGH);
//  } else if (timing >= 5000)                     {
//    
//    digitalWrite(PMP, LOW
//    );
//    
//  } 
//  Serial.println(digitalRead(PMP));
//  Serial.println(timing);
//  Serial.print("motor_PWM: ");
//  Serial.println(pot_pwm);
  //rotate(50, 0);
//  Serial.print("L_PWM: ");
//  Serial.println(digitalRead(LPWM));
//  Serial.print("R_PWM: ");
//  Serial.println(digitalRead(RPWM));
//  Serial.print("L_EN: ");
//  Serial.println(digitalRead(L_EN));
//  Serial.print("R_EN: ");
//  Serial.println(digitalRead(R_EN));
//  
  SW1.loop(); // MUST call the loop() function first
  SW2.loop();
  int front_end = SW1.getState();
  int back_end = SW2.getState();
  if(front_end == HIGH) {
    Serial.println("SW1: UNTOUCHED");
  } else {
    Serial.println("SW1: TOUCHED");
  }

  if(back_end == HIGH) {
    Serial.println("SW2: UNTOUCHED");
  } else {
    Serial.println("SW2: TOUCHED");
  }
//  current_dist();
//  digitalWrite(PMP, HIGH);
//}
}

void rotate(int value, int dir) {

  digitalWrite(L_EN, HIGH);
  digitalWrite(R_EN, HIGH);
  int pwm1Pin, pwm2Pin;
  if(dir == 1){
    analogWrite(LPWM, toPWM(value));
    analogWrite(RPWM, 0);
  }else{
    analogWrite(LPWM, 0);
    analogWrite(RPWM, toPWM(value));  
  }
  digitalWrite(pwm1Pin, LOW);
  Serial.print("PWM1: ");
  Serial.print(pwm1Pin);
  Serial.print(" PWM2: ");
  Serial.print(pwm2Pin);
  Serial.print(" PWM value: ");
  Serial.print(value);
  Serial.println("%");    
     
  if(value >=0 && value <=100 ){
    analogWrite(pwm2Pin, toPWM(value));
  }
}

void stop_motor(){
  digitalWrite(L_EN, LOW);
  digitalWrite(R_EN, LOW);
  //Serial.println("Motor stopped");
}

int toPWM(int v){
  return map(v, 0,100,0,255);
}

int current_dist() {
  digitalWrite(TRIG, LOW); //set trigger signal low for 2us
  delayMicroseconds(2);

   /*send 10 microsecond pulse to trigger pin of HC-SR04 */
  digitalWrite(TRIG, HIGH);  // make trigger pin active high
  delayMicroseconds(10);            // wait for 10 microseconds
  digitalWrite(TRIG, LOW);   // make trigger pin active low

  /*Measure the Echo output signal duration or pulss width */
  duration = pulseIn(ECHO, HIGH); // save time duration value in "duration variable
  distance= duration*0.034/2; //Convert pulse duration into distance
  if (distance > 10) { //limit sensor range to 20cm so not mistaken other objects
    distance = 0;
  }  
//  if (distance != 0) {
//    if (count < 50) {
//      count = count + 1;
//      Serial.print("Count: ");
//      Serial.print(count);
//      return 0;
//    } else {
//      count == 0;
//    }
//  }
  Serial.print("Distance: ");
  Serial.println(distance);
  return distance;
}

void countDownWater() {
  if (!timerStarted(timer)) {
    timerStart(timer);
    digitalWrite(PMP, HIGH);
    Serial.println("Starting watering");
  } else {
    time_val = timerReadSeconds(timer);
    Serial.print("Watering time (s): ");
    Serial.println(time_val);
    if (time_val > 5) {
      timerStop(timer);
      timerRestart(timer);
      digitalWrite(PMP, LOW);
      Serial.println("Finished watering" );
    }
  }
}
