#include "pins.hpp"
//#include "pmt.hpp"
#include "stepper.hpp"



//Parameters
unsigned int INTEGRATION_TIME_ms = 1000; //!< Signal integration time for PMT
unsigned int PULSE_FREQUENCY = 1000; //!< Pulsing frequency for LED (in Hz)



//Stepper Motor
static Stepper stepper;


// Internal flags
volatile bool experiment_running = false; //!< Flag that executes the `loop`.
bool calibration_failed = true; //!< Indiates whether the calibration failed.
bool calibration_run = false; //!< Is `true` only during the calibration process.
volatile unsigned long last_limit_hit = 0; //!< Last time in `ms` when the limit switches were hit.
volatile unsigned long limit_sw_debounce_time_ms = 500; //!< Debouncing time for limit switches.
volatile int stepper_dir = 1; //!< Volatile stepper direction dupl;icate used using calibration.

// Serial Event variables (used for parsing commands)
String received_str = ""; //!< Stores the string received through serial
bool completed_parse = false; //!< Flag that indicates internally if the parsing is complete.


/** @brief Turn ON the electronics and calibrate. */
void start_exp()
{
	//Set PMT
	//PMT::init_DAC();
	//PMT::init_ADC();
	//PMT::set_ref_volt(700);

	//Set LED
	tone(PULSE_PIN, PULSE_FREQUENCY);


	//Start IR Sensors
	digitalWrite(IR_SENSOR1_VCCPIN, HIGH);
	digitalWrite(IR_SENSOR2_VCCPIN, HIGH);

	stepper.reverse(WHITE_FIELD);
	spiral_calibrate(); //Do calibration 
	
	delay(2000);

	//Start loop operation
	experiment_running = true;

}

/** @brief Stop the electronics. */
void stop_exp()
{
	//Stop loop operation
	experiment_running = false;

	//Shutdown (DAC) MCP & ADC
	//PMT::stop_ADC();
	//PMT::stop_DAC();


	//Start IR Sensors
	digitalWrite(IR_SENSOR1_VCCPIN, LOW);
	digitalWrite(IR_SENSOR2_VCCPIN, LOW);

	//Turn off stepper motor.
	stepper.disable();

	//Stop LED & Pulsing
	pinMode(PULSE_PIN, OUTPUT);
	digitalWrite(PULSE_PIN, LOW);


	//Calibrate on STOP
	//spiral_calibrate();

}


/** @brief Move to the next sample. */
void next_sample(unsigned int checkpoints=1)
{
	bool found = false;
	const unsigned int fixed_steps = SAMPLE_DIST;
	
	for(unsigned int i = 0; i < checkpoints; i++)
	{
			found = false;
			
			//stepper.move(int(WHITE_FIELD*3/4));
			if(digitalRead(IR_SENSOR2_RDPIN) == LOW) //If WHITE_FIELD
			{
				stepper.move_till(int(WHITE_FIELD*1.1), IR_SENSOR2_RDPIN, HIGH); //Detect BLACK_FIELD
			}
			
			delay(200);
			found = stepper.move_till(SAMPLE_DIST, IR_SENSOR2_RDPIN, LOW); //Detect WHITE_FIELD
			delay(200);
			
			if(found)
			{
				Serial.println("# Sample found! ");
				delay(200);
				stepper.move(int(WHITE_FIELD/2));
				delay(200);
			}
			
			else
			{
				Serial.print("# Sample not found! ");
			}
	 		
	 		Serial.print("# Stepper position: ");
	 		Serial.println(stepper.pos);
	}

	return found;
}


/** @brief Sets up pin modes and one time settings. */
void setup()
{
	joystick.init(JS_X_AXIS_PIN, JS_SW_PIN);
	attachInterrupt(digitalPintoInterrupt(JS_SW_PIN), isr_joystick_sw, CHANGE);
	

	//LED
	pinMode(PULSE_PIN, OUTPUT);


	//IR sensors
	pinMode(IR_SENSOR1_VCCPIN, OUTPUT);
	pinMode(IR_SENSOR2_VCCPIN, OUTPUT);
	pinMode(IR_SENSOR1_RDPIN, INPUT);
	pinMode(IR_SENSOR2_RDPIN, INPUT);


	//Serial
	Serial.begin(9600);
		while(!Serial){}
			Serial.println("# Serial is setup");

	//Stepper Configuration
	stepper.init(STEPPER_EN_PIN, STEPPER_DIR_PIN, STEPPER_PULSE_PIN);
	stepper.step_hperiod_ms = 5;

//! testing
	start_exp();
	experiment_running = true;


	Serial.println("# Setup has ended!");
}




void loop()
{
	if(experiment_running && !calibration_failed)
	{
		stepper.set_dir(stepper.fwd); //Set direction to forward

		// Read 7 samples
		for(unsigned int i = 0; i < NO_SAMPLES-1; i++)
		{
			int16_t value = 999;//PMT::read(INTEGRATION_TIME_ms);
			Serial.print(value);
			Serial.print('\t');

			next_sample(1);
			delay(5000);
		}

		// Read last sample
		int16_t value = 999;//PMT::read(INTEGRATION_TIME_ms);
		Serial.print(value);
		Serial.print('\n');

		//Reverse
		stepper.set_dir(stepper.rev);
		next_sample(NO_SAMPLES-1);
	}



	//In loop
	if(joystick_control)
	{
		//Reset joystick buffer for a new session
		if(new_joyst_session)
		{
			joystick.reset();
			new_joyst_session = false;
		}

		joystick.read(50, 50);  						// Read joystick state for 50 ms
		int move_steps = joystick.get_steps(100);		// Number of steps to move -> try for 100.
		stepper.set_dir(move_steps);					// Set appropriate direction
		stepper.move(move_steps);						// Execute quantum of steps

		joystick.register_steps(move_steps);			// Update joystick buffer with the number of steps taken.

		delay(5000);
	}
}


void spiral_calibrate()
{
	calibration_run = true;	

		bool detected = false;
		int sample_pos = 0;

		//Check if already calibrated
		if(digitalRead(IR_SENSOR1_RDPIN) == HIGH && digitalRead(IR_SENSOR2_RDPIN) == LOW)
		{
			delay(500);
			detected = (digitalRead(IR_SENSOR1_RDPIN) == HIGH && digitalRead(IR_SENSOR2_RDPIN) == LOW);
		}


		// Start spiral scan
		for(unsigned int i = 0; (i < NO_SAMPLES-1 && !detected); i++)
		{
			stepper.set_dir(stepper.rev);
			
			for(unsigned int rev_scan = 0; (rev_scan < i  && !detected); rev_scan++)
			{
				next_sample(1);
				sample_pos--;

				if(digitalRead(IR_SENSOR1_RDPIN) == HIGH)
				{
					delay(500);
					if(digitalRead(IR_SENSOR1_RDPIN) == HIGH) //Debounce Sensor
					{
						detected = true;
						Serial.print("# Detected at: ");
						Serial.println(sample_pos);
						break;
					}

				}
			}

			stepper.set_dir(stepper.fwd);
			i = i + 1;
			for(unsigned int fwd_scan = 0; (fwd_scan < i && !detected); fwd_scan++)
			{
				next_sample(1);
				sample_pos++;	
				if(digitalRead(IR_SENSOR1_RDPIN) == HIGH)
				{
					delay(500);
					if(digitalRead(IR_SENSOR1_RDPIN) == HIGH) //Debounce Sensor
					{
						detected = true;
						Serial.print("# Detected at: ");
						Serial.println(sample_pos);
						break;
					}
				}
			}	
		}


		if(detected)
		{
			calibration_failed = false;
			stepper.pos = 0; //Init stepper position variable.
			Serial.println("# Spiral Calibration successful!");
		}

		else
		{
			calibration_failed = true;
			Serial.println("# Spiral Calibration failed!");
		}

	calibration_run = false;

}

/** @brief Check if this is the first sample by correlating the two sensors. */
void is_first_sample()
{
	if(digitalRead(IR_SENSOR2_RDPIN) == LOW)
	{
		delay(500);
		if(digitalRead(IR_SENSOR1_RDPIN) == HIGH && digitalRead(IR_SENSOR2_RDPIN) == LOW)
		{ return true; }

		else { return false; }
	}
}


/** @brief Detection of control codees - {start and stop}. */
/*void serialEvent() 
{
	while (Serial.available() && !completed_parse) 
	{
		char c = (char)Serial.read();
		received_str += c;
		if (c == '\n') { completed_parse = true; }
	}

	if(completed_parse)
	{
		if (received_str.equals("start\n")) 
		{	start_exp();
			completed_parse = false;
			received_str = "";
		}

		else if (received_str.equals("stop\n"))
		{
			stop_exp();
			completed_parse = false;
			received_str = "";
		}

		else //Unknown command
		{
			completed_parse = false;
			received_str = "";
		}
	}
}*/


Joystick1D joystick;
volatile bool joystick_control = false;				//!< Flag that controls operatiobn of joystick in the code
volatile unsigned long last_joysw_time_ms = 0; 		//!< Last time the joystick was triggered.
const unsigned long debounce_time_joysw	= 50;		//!< Debouncing time for the switch.
volatile new_joyst_session = false;
void isr_joystick_sw()
{
	if (millis() - last_joysw_time_ms < debounce_time_joysw)
	{
	
		joystick_control =  !joystick_control;

		if(joystick_control)
		{
			new_joyst_session = true;
		}

	}
}