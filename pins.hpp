#pragma once


// LED Pins
	const unsigned int PULSE_PIN = 6;


//IR Sensor
	const unsigned int IR_SENSOR1_RDPIN = 3;
	const unsigned int IR_SENSOR2_RDPIN = 2;


	const unsigned int IR_SENSOR1_VCCPIN = 4;
	const unsigned int IR_SENSOR2_VCCPIN = 5;

//Stepper Pins
	const unsigned int STEPPER_EN_PIN = 11;
	const unsigned int STEPPER_DIR_PIN = 13;	
	const unsigned int STEPPER_PULSE_PIN = 12;

//Limit Switch Interrupt
	const unsigned int LIMIT_SW_X2 = 3;


//PMT Control Pins
	const int PMT_CS_PIN = 53;


//Dedicated DC Vibration motor control pins
	/** \todo Add pins. */
const int VIB_IN_PIN = 0;
const int VIB_VCC_PIN = 0;

#define SAMPLE_DIST 1300
#define STAGE_LEN 9100

#define NO_SAMPLES 8


#define EDGE_LEN 0
#define STRIP_LEN 0
#define WHITE_FIELD 700
#define BLACK_FIELD 600