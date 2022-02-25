#pragma once
#include <Arduino.h>
#include <util/atomic.h>

class Stepper
{
public:

    const static unsigned int total_steps = 200;  //!<Total number of steps in one full rotation
    
    unsigned int enPIN = 99; //!< Enable drive pin
    unsigned int dirPIN = 99; //!< Direction select pin
    unsigned int pulsePIN = 99; //!< Pulse pin / drive pin
    
    bool auto_disable_power = false; //!< Automatically disabl;e power after every move
    unsigned int step_hperiod_ms = 5; //!< Half-period of single step
    int dir = +1; //!< Direction of movement {+1, -1}
    int pos = 0; //!< Position of the motor.
    
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

    /** @brief Function to move the motor **forward** by `n` steps. */
    void forward(int n)
    {
        dir = +1;
        digitalWrite(dirPIN, LOW);
        delay(2);
        this->move(n);
    }

    /** @brief Function to move the motor **reverse** by `n` steps. */
    void reverse(int n)
    {
        dir = -1;
        digitalWrite(dirPIN, HIGH);
        delay(2);
        this->move(n);
    }

    /** @brief Set the direction of the motor. */
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

// Controlled movement functions


    /** @brief Generic function that moves n-steps without setting the direction pin.
     * @return `true` if the `stop_state` was satisfied, else `false` if the function returned after completing  fixed_steps. */
    bool move_till(const int fixed_steps, const unsigned int monitor_pin, const int stop_state)
    {
        bool stop_condition = false;
        digitalWrite(enPIN, LOW);
        delay(2);

        for(unsigned int i = 0; (i < fixed_steps && !stop_condition);
            i++)
        {
            digitalWrite(pulsePIN, HIGH);
            delay(step_hperiod_ms);
            digitalWrite(pulsePIN, LOW);
            delay(step_hperiod_ms);
            pos += dir;

            //Debounce detection -> Verify after 500ms
            if(digitalRead(monitor_pin) == stop_state)
            {
                delay(500);
                stop_condition = (digitalRead(monitor_pin) == stop_state);
            }
        }

        return (digitalRead(monitor_pin) == stop_state);
    }


    void forward_till(const int n, const unsigned int monitor_pin, const int stop_state)
    {
        dir = direction::fwd;
        digitalWrite(dirPIN, LOW);
        delay(2);
        this->move_till(n, monitor_pin, stop_state);


    }


    void reverse_till(const int n, const unsigned int monitor_pin, const int stop_state)
    {
        dir = direction::rev;
        digitalWrite(dirPIN, HIGH);
        delay(2);
        this->move_till(n, monitor_pin, stop_state);

    }


    /** @brief Direction states of the stepper motor. */
    enum direction : int
    {
        fwd = 1,
        rev = -1
    };
};
