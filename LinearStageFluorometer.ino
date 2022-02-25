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

	//Set LED
	tone(PULSE_PIN, PULSE_FREQUENCY);


	//Start IR Sensors
	digitalWrite(IR_SENSOR1_VCCPIN, HIGH);
	digitalWrite(IR_SENSOR2_VCCPIN, HIGH);

	stepper.reverse(WHITE_FIELD);
	spiral_calibrate();
	delay(2000);

	//Start loop operation
	experiment_running = true;

}

/** @brief Stop the electronics. */
void stop_exp()
{
	//Stop loop operation
	experiment_running = false;

	//Shutdown (DAC) MCP


	//Shutdown ADC


	//Start IR Sensors
	digitalWrite(IR_SENSOR1_VCCPIN, LOW);
	digitalWrite(IR_SENSOR2_VCCPIN, LOW);

	//Turn off stepper motor.
	stepper.disable();

	//Stop LED & Pulsing
	pinMode(PULSE_PIN, OUTPUT);
	digitalWrite(PULSE_PIN, LOW);


	//Calibrate on STOP
	//calibrate();

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
	//stepper.reverse(WHITE_FIELD);
	//stepper.set_dir(stepper.fwd);
	//stepper.move(500);
	//next_sample(1);
	//calibrate();
	///*stepper.forward(200);
	//stepper.set_dir(stepper.rev);
	//next_sample(7);
	
	//stepper.set_dir(stepper.rev);
	//for(unsigned int i = 0; i < 7; i++)
	//{
	//	next_sample();
	//	delay(5000);
	//}
	experiment_running = true;
	Serial.println("# Setup has ended!");

	Serial.println("# Starting Experment!");

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
			Serial.print(i+1);
			Serial.print(". ");
			Serial.print(value);
			Serial.print('\n');


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
}




/** @brief Align the optics with the position of sample 1. */
void calibrate()
{
	calibration_run = true;

		int i = 0; //Number of samples covered
		bool detected = false;
		//const unsigned int max_steps = SAMPLE_DIST + 200;

		// Initial Reverse
		stepper.set_dir(stepper.rev);
		stepper_dir = 0;
		delay(1000);

		//Forward scan
		stepper.set_dir(stepper.fwd);
		stepper_dir = 1;
		while(!detected && i < NO_SAMPLES)
		{
			next_sample(1);
			if(digitalRead(IR_SENSOR1_RDPIN == HIGH))
			{
				detected = true;
			}
			i++;
		}

		//Reverse scan
		if(!detected)
		{
			stepper_dir = 0;
			stepper.set_dir(stepper.dir*-1);
			i = 0;

			while(!detected && i < NO_SAMPLES)
			{
				next_sample(1);
				if(digitalRead(IR_SENSOR1_RDPIN == HIGH))
				{
					detected = true;
				}
				i++;
			}
		}

		if(detected)
		{
			stepper.pos = 0; //Init stepper position variable.
			Serial.println("# Calibration successful!");
		}

		else
		{
			calibration_failed = true;
			Serial.println("# Calibration failed!");
		}

	calibration_run = false;		
}



/** @brief `Interrupt service routine` is launched when a limit switch hit causes an interrupt to fire. */
void isr_limit_sw_hit()
{
	if((millis() - last_limit_hit) >= limit_sw_debounce_time_ms)
	{
		if(calibration_run)
		{
			//Reverse direction of motor
			stepper.set_dir(stepper.dir*-1); //Might not work.

			digitalWrite(STEPPER_DIR_PIN, !stepper_dir);

		}

		else
		{
			//Disable stepper motor
			stepper.disable();
			digitalWrite(STEPPER_EN_PIN, LOW);
		}
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