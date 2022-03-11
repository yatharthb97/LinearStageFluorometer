#pragma once

#include "pins.hpp"
#include <MCP48xx.h>
#include <Adafruit_ADS1X15.h>

namespace PMT
{
    static Adafruit_ADS1115 ADS;
    static MCP4822 MCP(PMT_CS_PIN); //!< DAC used to set the PMT reference voltage
    static unsigned int max_ref_volt = 850; //!< Highest Reference Voltage allowed.


    /** @brief Initalize DAC. */
    static void init_DAC()
    {
        PMT::MCP.init();
        PMT::MCP.setGainA(MCP4822:: High);
        PMT::MCP.setGainB(MCP4822:: Low);
        PMT::MCP.turnOnChannelA();          //Turn ON channel A
        PMT::MCP.setVoltageA(0);          //500= 0.505 volt controll voltage
        PMT::MCP.updateDAC();               //Update DAC
    }

    /** @brief Initalized the 16-bit ADC for reading the PMT signals. */
    static void init_ADC()
    {
        // Initialize ads1015 at the default address 0x48
        if (!PMT::ADS.begin()) 
        {
          Serial.println("# Failed to initialize ADS1115 (ADC).");
          return;
        }

        PMT::ADS.setGain(GAIN_ONE);     // Unity gain
        PMT::ADS.setDataRate(RATE_ADS1115_860SPS); //860 samples per second read rate [fastest]
        PMT::ADS.readADC_SingleEnded(0); // Trigger first conversion 
    }

    /** @brief Turn off ADC. (ADS1115) */
    static void stop_ADC()
    {
        // No stop procedure defined yet
    }

    /** @brief Turn off DAC (MCP4822). */
    static void stop_DAC()
    {
        PMT::MCP.setVoltageA(0); //Controll voltage off
        PMT::MCP.updateDAC(); //updating DAC
    }

    
    /** @breif Set the reference voltage for PMT to the value `volt`.
     * \attention If the value passed is greater than the `PMT::max_ref_volt`, then an error 
     * is generated and the call is ignored.*/
    static void set_ref_volt(unsigned int volt)
    {
        if(volt > PMT::max_ref_volt)
        {
            Serial.println("# ERROR : | PMT > Safety mechanism prevented setting of a high reference voltage.");
        }

        else
        {
            PMT::MCP.setVoltageA(volt); //500= 0.505 volt controll voltage
            PMT::MCP.updateDAC(); //updating DAC
            Serial.print("# | PMT > Reference Voltage = ");
            Serial.println(volt);
        }
    }


    /** @brief Reads and returns the pmt reading.
     * @param integn_time_ms Integration time over which the value is averaged.
     * /note At fastest rate the ADC reads 860 samples/s. 
     * This function reads 500 samples/s. */
    static double read(const unsigned int integn_time_ms)
    {
        uint16_t signal = 0;                      // Instantaneous
        double sig_integrated = 0.0;              // Sum
        
        //Integration loop
        for(unsigned int i = 0; i < int(integn_time_ms/2); i++)
        {
            delayMicroseconds(1000); //Delay for 1 ms
            signal = PMT::ADS.readADC_SingleEnded(0); //Read value
            sig_integrated += signal;   
        }
        
        sig_integrated = sig_integrated / double(integn_time_ms/2);

        //! TODO : Clean
        //Serial.print("signal =  ");
        //Serial.println(sig_integrated);

        return(sig_integrated);
    }
};