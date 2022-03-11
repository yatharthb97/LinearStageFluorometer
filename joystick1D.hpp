#pragma once

/** @brief Class that interfaces a 3 pin joystick. Reads the x and y position and a click switch. */
class JoyStick1D
{
public:

	unsigned int xPIN = 999; //!< Pin that reads the x-axis
	unsigned int swPIN = 999; //!< Pin that reads the switch state


	int tpos = 0; //!< Target number of steps held by the joystick buffer.
	unsigned min_correction = 10; //!< Minimum number of steps performed for correction.

	/** @brief Constructor that initalizes the pins of the joysticks. */
	constexpr JoyStick1D()
	{}

	void init(unsigned int xPIN, unsigned int swPIN)
	{
		this->xPIN = xPIN;
		this->swPIN = swPIN;

		pinMode(xPIN, INPUT);
		pinMode(swPIN, INPUT_PULLUP);
	}

	/** @brief Update the target from the joystick position. 
	 * @param read_time_ms Total integration time and also the normalization parameter.
	 * @param read_cnt Total number of reads performed in `read_time_ms`. */
	void read(unsigned int read_time_ms , unsigned int read_cnt)
	{
		
		int total_readval = 0;
		double wait_time_ms = read_time_ms / read_cnt;
		
		for(unsigned int i = 0; i < read_cnt; i++)
		{
			int read_now = analogRead(xPIN);
			read_now = map(read_now, 0, 1023, -512, 512);
			total_readval += read_now;
			delay(wait_time_ms);
		}
		
		tpos += total_readval/read_time_ms;
		
		Serial.print("# Updated target: ");
		Serial.println(tpos);

	}

	/** @brief Register steps that the motor performed, 
	 * so that it can be subtracted from the target.*/
	void register_steps(int steps)
	{
		tpos -= steps;
	}

	int get_steps(unsigned int hint = 0)
	{
		if(hint == 0)
		{
			hint = min_correction;
		}

		if(abs(tpos) > hint)
		{
			return hint * sign(tpos);
		}

		else
		{
			return tpos;
		}
	}


	void reset()
	{
		tpos = 0;
	}


private:

	template <typename T>
	void sign(T x)
	{
		return (x > 0) - (x < 0);
	}
};





