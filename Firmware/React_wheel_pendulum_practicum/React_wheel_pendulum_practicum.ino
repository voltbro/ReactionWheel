/*
The SimpleFoc library is used to control the BLDC motor https://docs.simplefoc.com/ 
Documentation for the motor driver and VBCore module https://voltbro.gitbook.io/vbcores 
*/
#include <VBCoreG4_arduino_system.h>
#include <Wire.h>
#include <AS5600.h>
#include <SimpleFOC.h>

#define pinSDA PB_7_ALT1
#define pinSCL PC6
#define pinNSLEEP PB3

#define K_pa 53.53 //54.9062 
#define K_pv 7.33 //7.3934
#define K_ma 0.008 //0.008717 
#define K_mv 0.023 //0.013016 
#define K_stand_up 35.0

HardwareTimer *timer_move = new HardwareTimer(TIM7);
HardwareTimer *timer_foc = new HardwareTimer(TIM2);

float target_angle, pendulum_angle, prev_angle, motor_angle, motor_velocity, pendulum_vel;
int stop_flag = 0, swing_up_flag = 0;

float offset;
float angle_tmp;
float u = 0;


float k_pa = K_pa;
float k_pv = K_pv;
float k_ma = K_ma;
float k_mv = K_mv;
float k_balance = K_stand_up;

float M = 0.014;
float l = 0.12;
float m = 0.063;
float J = M*l*l/3;
float Jm = 0.0003385;
float g = 9.81;

float E0 = (M/2+m)*g*l; // energy of the pendulum at the upper equilibrium position
float E;
float epsilon = 0.02;

float c_vel = 0.5*(J+m*l*l+Jm);

SPIClass SPI_3(PC12, PC11, PC10);

MagneticSensorSPI motor_sensor = MagneticSensorSPI(PA15, 14, 0x3FFF);
AMS_5600 pendulum_sensor;

float sensitivity = 45.0;
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
  pinMode(LED2, OUTPUT);
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
  
  motor.current_limit = 5;
  motor.voltage_limit = 16;
  motor.velocity_limit = 500; 
  motor.init();
  motor.initFOC();


  offset = pendulum_sensor.getRawAngle();
  pendulum_angle = pendulum_sensor.getRawAngle() - offset;
  prev_angle = pendulum_angle;
 
  timer_move->pause();
  timer_move->setOverflow(2000, HERTZ_FORMAT); 
  timer_move->attachInterrupt(move); // the move function will be called by a timer with a frequency of 2kHz
  timer_move->refresh();
  timer_move->resume();

  timer_foc->pause();
  timer_foc->setOverflow(5000, HERTZ_FORMAT);
  timer_foc->attachInterrupt(FOC_func); // FOC function will be called by a timer with a frequency of 5kHz
  timer_foc->refresh();
  timer_foc->resume();

  delay(1000);

}


void control(){
  calc_angle_vel(); // angles and velocities from the pendulum arm and the flywheel (motor) are calculated
  E = c_vel*sq(pendulum_vel)+ E0*cos(pendulum_angle); // pendulum energy

  if (swing_up_flag){
    if (abs(E - E0) < epsilon) { // if the energy difference is less than epsilon, we hold the unstable equilibrium state
      u = -(k_pa*pendulum_angle + k_pv*pendulum_vel + k_ma*motor_angle + k_mv*motor_velocity);   
    }
    else { // else we swing the pendulum
      u = -k_balance*(E-E0)*sign(pendulum_vel);
    }
  }
  else if(stop_flag) u = -k_balance*(E+E0)*sign(pendulum_vel); //u = 0;

  if (u > 12) u = 12; // limit the voltage to 12 volts
  else if (u < -12) u = -12;
  
  motor.target = u;
}


void calc_angle_vel(){
  angle_tmp = pendulum_sensor.getRawAngle()-offset; //read the angle of the pendulum from the sensor
  angle_tmp = 2048 - angle_tmp; // we read angles from 0 to 4095, let's translate this value to a range of -2048 to 2048
  pendulum_angle = (PI*angle_tmp)/2048; // translate the raw angle to a range of -pi to pi
  if (pendulum_angle > PI){
    angle_tmp = fmod(pendulum_angle, PI);
    pendulum_angle = angle_tmp - PI;
  }
  pendulum_vel = (pendulum_angle - prev_angle)*1000; //the function calc_angle_vel is called at a frequency of ~1kHz, so for the speed to be in rad/sec multiply by 1000.
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
  delay(1000);
}

void start(){
  stop_flag = 0;
  swing_up_flag = 1;
  delay(1000);
}

void loop() {
  int buttonState = digitalRead(USR_BTN);
  if(buttonState == 0){//when the button is pressed, the pendulum starts swinging, if the button is pressed again, the control is switched off
    if(swing_up_flag == 1){
      digitalWrite(LED2, LOW);
      stop();
    }
    else{
      digitalWrite(LED2, HIGH);
      start();
    }
  }
  control();
  delay(1);
}

void FOC_func(){
  motor.loopFOC();
}
void move(){
  motor.move(); 
}

