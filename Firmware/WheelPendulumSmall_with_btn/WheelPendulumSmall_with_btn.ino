#define K1 53.53
#define K2 4.33
#define K3 0.38
#define K4 0.23

#include <VBCoreG4_arduino_system.h>
#include <Wire.h>
#include <AS5600.h>
#include <SimpleFOC.h>

#define pinSDA PB_7_ALT1
#define pinSCL PC6
#define pinNSLEEP PB3


// HardwareTimer *timer_read_request = new HardwareTimer(TIM7);
// HardwareTimer *timer_move = new HardwareTimer(TIM5);
HardwareTimer *timer_send_response = new HardwareTimer(TIM15);
HardwareTimer *timer_move = new HardwareTimer(TIM7);
HardwareTimer *timer_foc = new HardwareTimer(TIM2);

float target_angle, pendulum_angle, prev_angle, motor_angle, motor_velocity;
float pendulum_vel; 

int stop_flag = 0, swing_up_flag = 0, balance_flag = 0;

unsigned long t, time_dot;
float offset;
float angle_tmp;
float u = 0;

// float k_pa =40;//120;
// float k_pv =0.8;// 15;
// float k_ma =0;// 0.008;
// float k_mv = 0.000015;//0.41;

float k_pa = K1; //54.9062;//120;
float k_pv = K2; //7.3934;// 15;
float k_ma = K3; //0.008717;//0.8716;// 0.008;
float k_mv = K4; //0.013016;//0.41;
float k_balance = 35;//3;

float M = 0.014;
float l = 0.12;
float m = 0.063;
float J = M*l*l/3;
float Jm = 0.0003385;
float g = 9.81;

float E0 = (M/2+m)*g*l;
float E;
float epsilon = 0.02;

float c_vel = 0.5*(J+m*l*l+Jm);

String answer, request;

SPIClass SPI_3(PC12, PC11, PC10);

MagneticSensorSPI motor_sensor = MagneticSensorSPI(PA15, 14, 0x3FFF);
AMS_5600 pendulum_sensor;

float sensitivity = 45.0; // mV/A 
InlineCurrentSense current_sense = InlineCurrentSense(sensitivity, PC1, PC2, PC3); 

BLDCMotor motor = BLDCMotor(7, 12.2, 238);
BLDCDriver3PWM driver = BLDCDriver3PWM(PA8, PA9, PA10);

void setup() {
  Serial.begin(500000);

  Wire.setSDA(pinSDA);
  Wire.setSCL(pinSCL);
  Wire.begin();

  pinMode(PB5, INPUT);
  pinMode(pinNSLEEP, OUTPUT);
  digitalWrite(pinNSLEEP, HIGH);
  pinMode(USR_BTN, INPUT_PULLUP);

  pinMode(PB15, OUTPUT);
  pinMode(PB14, OUTPUT);
  pinMode(PB13, OUTPUT);
  digitalWrite(PB15, HIGH);
  digitalWrite(PB14, HIGH);
  digitalWrite(PB13, HIGH);

  driver.voltage_power_supply = 16;
  driver.init();
  driver.enable();
  motor.linkDriver(&driver);

  current_sense.init();
  current_sense.linkDriver(&driver);
  motor.linkCurrentSense(&current_sense);

  motor_sensor.init(&SPI_3);
  motor.linkSensor(&motor_sensor);
  motor.torque_controller = TorqueControlType::voltage;
  motor.controller = MotionControlType::torque;

  if(pendulum_sensor.detectMagnet() == 0 ){
    while(1){
        if(pendulum_sensor.detectMagnet() == 1 ){
            Serial.print("Current Magnitude: ");
            Serial.println(pendulum_sensor.getMagnitude());
            break;
        }
        else{
            Serial.println("Can not detect magnet");
        }
        delay(1000);
    }
  }

  
  motor.current_limit = 2;
  motor.voltage_limit = 16;
  motor.velocity_limit = 500;
  // motor.current_limit = 1.94;
  // motor.voltage_limit = 15;
  // motor.velocity_limit = 5.5;
  
  motor.init();
  motor.initFOC();


  offset = pendulum_sensor.getRawAngle();
  pendulum_angle = pendulum_sensor.getRawAngle() - offset;
  prev_angle = pendulum_angle;
 


  timer_move->pause();
  timer_move->setOverflow(2000, HERTZ_FORMAT); 
  timer_move->attachInterrupt(move);
  timer_move->refresh();
  timer_move->resume();

  // timer_move->pause();
  // timer_move->setOverflow(200, HERTZ_FORMAT); 
  // timer_move->attachInterrupt(calc_angle_vel);
  // timer_move->refresh();
  // timer_move->resume();

  timer_foc->pause();
  timer_foc->setOverflow(5000, HERTZ_FORMAT);
  timer_foc->attachInterrupt(FOC_func);
  timer_foc->refresh();
  timer_foc->resume();

  timer_send_response->pause();
  timer_send_response->setOverflow(200, HERTZ_FORMAT); 
  timer_send_response->attachInterrupt(send_response);
  timer_send_response->refresh();
  timer_send_response->resume();

  Serial.println(motor.shaft_angle);
  delay(1000);

}

float tmp_e;
void update(){
  calc_angle_vel();
  E = c_vel*sq(pendulum_vel)+ E0*cos(pendulum_angle);
  tmp_e = E-E0;
  if (swing_up_flag){
   time_dot = millis() - t;
   if (abs(E-E0)<epsilon) {
      u = -(k_pa*pendulum_angle + k_pv*pendulum_vel + k_ma*motor_angle + k_mv*motor_velocity);
      
   }
   else {
    u = -k_balance*(E-E0)*sign(pendulum_vel);
   }
   if (u > 12) u = 12;
   else if (u < -12) u = -12;
  }
  else if(stop_flag) u = 0;
  motor.target = u;
}


void calc_angle_vel(){
  angle_tmp = pendulum_sensor.getRawAngle()-offset;
  angle_tmp = 2048 - angle_tmp;
  pendulum_angle = (PI*angle_tmp)/2048;
  if (pendulum_angle > PI){
    angle_tmp = fmod(pendulum_angle, PI);
    pendulum_angle = angle_tmp - PI;
  }
  pendulum_vel = (pendulum_angle - prev_angle)*1000;
  prev_angle = pendulum_angle;
  motor_velocity =  -motor.shaft_velocity;
  motor_angle = -motor.shaft_angle;
}

int sign(float val){
  if (val < 0) return -1;
  if (val==0) return 0;
  return 1;
}

void stop(){
  u = 0;
  stop_flag = 1;
  swing_up_flag = 0;
  motor.move(0);
  Serial.println("#STOP OK");
  delay(100);
}

void FOC_func(){
  motor.loopFOC();
}
void move(){
  motor.move(); 
}
void read_request(){
  if(Serial.available()>0){
    request = Serial.readStringUntil('\n');
    int index = request.indexOf(' ');
    if (index != -1){

      String tmp_sub_req = request.substring(0, index);
      request = request.substring(index+1);
      index = request.indexOf(' ');

      if (tmp_sub_req == "#SETK"){        
        k_pa = request.substring(0, index).toFloat();  
        request = request.substring(index+1);
        index = request.indexOf(' ');  

        k_pv = request.substring(0, index).toFloat();
        request = request.substring(index+1);
        index = request.indexOf(' '); 

        k_ma = request.substring(0, index).toFloat();        
        k_mv = request.substring(index+1).toFloat(); 
        String msg = String("k_pa: ")+k_pa+" k_pv: "+k_pv+" k_ma "+k_ma+" k_mv "+k_mv; 
        // Serial.println(msg);   
        Serial.println("#SETK OK");
      }
      else if (tmp_sub_req == "#MOVE"){
        u = request.toFloat();
        motor.target = u;
        stop_flag = 0;
        swing_up_flag = 0;
        String msg = String("#MOVE OK ")+u;
        Serial.println(msg);
      }
    }
    else {
      if (request == "#STOP"){
        stop();
      }
      else if(request == "#SWING_UP") {
        swing_up_flag = 1;
        t = millis();
      }
    } 
  }
}

void send_response(){
  // if (swing_up_flag){
    //answer = String("#OUT ")+time_dot + " p_a: " + pendulum_angle+ " p_v: " + pendulum_vel + " m_a: "+motor_angle+" m_v: "+ motor_velocity + " E: " + tmp_e + " u: " + u;
    Serial.println(motor_angle);
  // }
}
unsigned long t_tmp = micros();
void loop() {
 // Serial.println(micros() - t_tmp);
 // t_tmp = micros();
  //read_request();
  int buttonState = digitalRead(USR_BTN);
  if(swing_up_flag == 1 && buttonState == LOW ){
    swing_up_flag = 0;
    t = millis();
  }
  if(buttonState == LOW && swing_up_flag == 0){
    swing_up_flag = 1;
    t = millis();
  }
  update();
  delay(1);
  //String msg = String(" pend angle: ")+pendulum_angle+" pend vel: "+pendulum_vel+" motor_velocity: "+motor_velocity + " u "+u + " time: "+(micros()-t);
}

