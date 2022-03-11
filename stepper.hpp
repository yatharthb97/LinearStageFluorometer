#pragma once
#include <Arduino.h>
#include <util/atomic.h>

/**

    1. Enabled on `LOW` and disables on `HIGH`.
    2. 

*/
class Stepper
{
public:

    const static unsigned int total_steps = 200;  //!<Total number of steps in one full rotation

    unsigned int enPIN = 99;        //!< Enable drive pin
    unsigned int dirPIN = 99;       //!< Direction select pin
    unsigned int pulsePIN = 99;     //!< Pulse pin / drive pin
    
    //Stepper parameters
    bool auto_disable_power = false; //!< Automatically disabl;e power after every move
    unsigned int step_hperiod_ms = 5; //!< Half-period of single step
    
    //Stepper state variables
    int dir = +1; //!< Direction of movement {+1, -1}
    int pos = 0; //!< Position of the motor.
    
    /** @brief Empty Constructor. */
    Stepper()
    {}

    /** @brief Initalization function for module. */
    void init(unsigned int enPIN, unsigned int dirPIN, unsigned int pulsePIN)
    {
        this->enPIN = enPIN;
        this->dirPIN = dirPIN;
        this->pulsePIN = pulsePIN;

        pinMode(enPIN, OUTPUT);
        pinMode(dirPIN, OUTPUT);
        pinMode(pulsePIN, OUTPUT);

    }

    /** @brief Direction states of the stepper motor. 
     * Use outside as `stepper.fwd` and `stepper.rev`.*/
    enum direction : int
    {
        fwd = 1,
        rev = -1
    };

    /** @brief Set the direction of the motor. 
     *  Recommended use: `stepper.set_dir(stepper.fwd)`.*/
    void set_dir(int dir)
    {
        this->dir = dir;

        if(dir < 0)
        {
            digitalWrite(dirPIN, HIGH);
        }

        if(dir > 0)
        {
            digitalWrite(dirPIN, LOW);
        }
        delay(2);
    }

    /** @brief Disable the motor / turn off. */
    void disable()
    {
        digitalWrite(enPIN, HIGH);
    }


    /** @brief Generic function that moves n-steps without setting the direction pin. */
    void move(int n)
    {
        digitalWrite(enPIN, LOW);
        delay(2);
        
        for(unsigned int i = 0; i < n; i++)
        {
            digitalWrite(pulsePIN, HIGH);
            delay(step_hperiod_ms);
            digitalWrite(pulsePIN, LOW);
            delay(step_hperiod_ms);
        }
        pos += dir*n;

        if(auto_disable_power)
            { digitalWrite(enPIN, HIGH); }
    }

    /** @brief Function to move the motor `forward` by `n` steps. */
    void forward(int n)
    {
        dir = +1;
        digitalWrite(dirPIN, LOW);
        delay(2);
        this->move(n);
    }

    /** @brief Function to move the motor `reverse` by `n` steps. */
    void reverse(int n)
    {
        dir = -1;
        digitalWrite(dirPIN, HIGH);
        delay(2);
        this->move(n);
    }


// Controlled movement functions (Movement is controlled by a `monitor_pin`.)


    /** @brief Generic function that moves n-steps without setting the direction pin.
     * 
     * @param Maximum number of steps performed by the motor if the stop_state is never satisfied.
     * @param monitor_pin Is a digital pin that will stop the movement once `stop_state` is satisfied.
     * @param stop_state Is either {HIGH or LOW}.
     * 
     * @return `true` if the `stop_state` was satisfied, else `false` if the function returned after completing  fixed_steps. */
    bool move_till(const int fixed_steps, const unsigned int monitor_pin, const int stop_state)
    {
        bool stop_condition = false;
        digitalWrite(enPIN, LOW);
        delay(2);

        for(unsigned int i = 0; (i < fixed_steps && !stop_condition); i++)
        {
            digitalWrite(pulsePIN, HIGH);
            delay(step_hperiod_ms);
            digitalWrite(pulsePIN, LOW);
            delay(step_hperiod_ms);
            pos += dir;

            //Debounced detection -> Verify twice within 500ms
            if(digitalRead(monitor_pin) == stop_state)
            {
                delay(500);
                stop_condition = (digitalRead(monitor_pin) == stop_state);
            }
        }

        return stop_condition;
    }


    /** @brief `move_till()` with defined forward movement. */
    void forward_till(const int n, const unsigned int monitor_pin, const int stop_state)
    {
        dir = direction::fwd;
        digitalWrite(dirPIN, LOW);
        delay(2);
        this->move_till(n, monitor_pin, stop_state);


    }


    /** @brief `move_till()` with defined reverse movement. */
    void reverse_till(const int n, const unsigned int monitor_pin, const int stop_state)
    {
        dir = direction::rev;
        digitalWrite(dirPIN, HIGH);
        delay(2);
        this->move_till(n, monitor_pin, stop_state);

    }
};
