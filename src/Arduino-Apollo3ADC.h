/*
 * Arduino-Apollo3ADC is an Arduino Library for the ADC module on the 
 * Ambiq Apollo3 Blue MCU.
 * Copyright (C) 2021  eResearch, James Cook University
 * Author: NigelB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Repository: https://github.com/jcu-eresearch/Arduino-Apollo3ADC
 *
 */

#ifndef ARDUINO_APOLLO3_ADC_H
#define ARDUINO_APOLLO3_ADC_H

#include "PinNames.h"
#include "bridge/pins.h"
#include "am_mcu_apollo.h"
#include "mbed_assert.h"
#include "Arduino.h"

#define Apollo3ADC_SLOTS 8
#define Apollo3ADC_Timer 3

enum Apollo3ADC_Slot_e
{
    Apollo3ADC_Slot_0 = 0,
    Apollo3ADC_Slot_1 = 1,
    Apollo3ADC_Slot_2 = 2,
    Apollo3ADC_Slot_3 = 3,
    Apollo3ADC_Slot_4 = 4,
    Apollo3ADC_Slot_5 = 5,
    Apollo3ADC_Slot_6 = 6,
    Apollo3ADC_Slot_7 = 7,
};

class Apollo3ADC;

class Apollo3ADC_Slot
{
    private:
        Apollo3ADC* adc;
        Apollo3ADC_Slot_e slot;
        PinName pinname = NC;
        am_hal_gpio_pincfg_t pincfg = {
            .uFuncSel       = AM_HAL_PIN_16_ADCSE0,
        };
        am_hal_adc_slot_config_t channel = {
            .eMeasToAvg = AM_HAL_ADC_SLOT_AVG_1,
            .ePrecisionMode = AM_HAL_ADC_SLOT_8BIT,
            .eChannel = AM_HAL_ADC_SLOT_CHSEL_SE0,
            .bWindowCompare = false,
            .bEnabled = false,
        };
        bool dirty = true;
        uint32_t ui32IntMask;
        
    
    public:
        Apollo3ADC_Slot(Apollo3ADC* adc, Apollo3ADC_Slot_e slot);
        ~Apollo3ADC_Slot();
        uint32_t commit();
        void setAveraging(am_hal_adc_meas_avg_e averaging);
        void setPrecision(am_hal_adc_slot_prec_e precision);
        bool setPin(PinName name);
        void setWindowsCompareEnable(bool enable);
        void setEnable(bool enable);

        int32_t readAnalog();
        int32_t readAnalog(PinName name);

        int32_t startSample();
        int32_t readResult(int32_t &result);


        void markDirty();
};

class Apollo3ADC
{
    friend Apollo3ADC_Slot;
    private:
        uint8_t adc_number = 0;
        void* g_ADCHandle;
        am_hal_adc_config_t ADCConfig = {
            .eClock = AM_HAL_ADC_CLKSEL_HFRC,
            .ePolarity = AM_HAL_ADC_TRIGPOL_RISING,
            .eTrigger = AM_HAL_ADC_TRIGSEL_SOFTWARE,
            .eReference = AM_HAL_ADC_REFSEL_INT_2P0,
            .eClockMode = AM_HAL_ADC_CLKMODE_LOW_LATENCY,
            .ePowerMode = AM_HAL_ADC_LPMODE0,
            .eRepeat = AM_HAL_ADC_SINGLE_SCAN
        };
        Apollo3ADC_Slot * slots[Apollo3ADC_SLOTS];
        bool dirty = true;
        bool started = false;
        bool timer_started = false;
        bool LFRC_started = false;
        
        uint32_t averaging_freq = AM_HAL_CTIMER_HFRC_12MHZ;
        uint32_t averaging_period  = 10;
        uint32_t averaging_on_time = 5;
        uint32_t averaging_timer = AM_HAL_CTIMER_TIMERA;

        void markDirty();
        void initTimer();
        void deinitTimer();

    public:
        Apollo3ADC();
        uint32_t begin();
        uint32_t commit();
        
        void setClock(am_hal_adc_clksel_e clock);
        void setTriggerPolarity(am_hal_adc_trigpol_e polarity);
        void setTrigger(am_hal_adc_trigsel_e trigger);
        void setReference(am_hal_adc_refsel_e reference);
        void setClockMode(am_hal_adc_clkmode_e mode);
        void setLowPowerMode(am_hal_adc_lpmode_e power_mode);
        void setRepeat(am_hal_adc_repeat_e repeat);

        Apollo3ADC_Slot* getADCSlot(Apollo3ADC_Slot_e slot);
        
        void setAveragingFreq(uint32_t freq);
        void setAveragingPeriod(uint32_t freq);
        void setAveragingOnTime(uint32_t freq);
        void setAveragingTimer(uint32_t timer);

        uint32_t end();
        ~Apollo3ADC();
};

#endif