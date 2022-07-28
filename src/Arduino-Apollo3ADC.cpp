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

#include "Arduino-Apollo3ADC.h"

static void* adc_isr_g_ADCHandle;

Apollo3ADC_Slot::Apollo3ADC_Slot(Apollo3ADC* adc, Apollo3ADC_Slot_e slot)
{
    this->adc = adc;
    this->slot = slot;
}

void Apollo3ADC_Slot::markDirty()
{
    dirty = true;
}

uint32_t Apollo3ADC_Slot::commit()
{
    uint32_t status = AM_HAL_STATUS_SUCCESS;
    if(pinname != NC)
    {
        adc->commit();
        
        if(dirty){
            status = am_hal_gpio_pinconfig(pinname, pincfg);
            if(status != AM_HAL_STATUS_SUCCESS){return status;}

            status = am_hal_adc_disable(adc->g_ADCHandle);
            if(status != AM_HAL_STATUS_SUCCESS){return status;}

            status = am_hal_adc_configure_slot(adc->g_ADCHandle, slot, &channel);
            if(status != AM_HAL_STATUS_SUCCESS){return status;}    
            
            status = am_hal_adc_enable(adc->g_ADCHandle);
            if(status != AM_HAL_STATUS_SUCCESS){return status;}
            
            if(channel.eMeasToAvg != AM_HAL_ADC_SLOT_AVG_1)
            {
                adc->initTimer();
            }

            dirty = false;
        }
    }
    return status;
}

void Apollo3ADC_Slot::setAveraging(am_hal_adc_meas_avg_e averaging){
    markDirty();
    channel.eMeasToAvg = averaging;
    if(averaging == AM_HAL_ADC_SLOT_AVG_1)
    {
        adc->setTrigger(AM_HAL_ADC_TRIGSEL_SOFTWARE);
        adc->setRepeat(AM_HAL_ADC_SINGLE_SCAN);
    }else{
        // If we are using averaging we need to use Timer3 to generate triggers.
        adc->setTrigger(AM_HAL_ADC_TRIGSEL_SOFTWARE);
        adc->setRepeat(AM_HAL_ADC_REPEATING_SCAN);
    }
}

void Apollo3ADC_Slot::setPrecision(am_hal_adc_slot_prec_e precision){
    markDirty();
    channel.ePrecisionMode = precision;
}

bool Apollo3ADC_Slot::setPin(PinName name){

    if(name != pinname)
    {
        am_hal_adc_slot_chan_e chan;
        pinname = name;
        switch(pinname)
        {
            case 11:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE2;
                pincfg.uFuncSel = AM_HAL_PIN_11_ADCSE2;
            }break;
            case 12:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE9;
                pincfg.uFuncSel = AM_HAL_PIN_12_ADCD0NSE9;
            }break;
            case 13:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE8;
                pincfg.uFuncSel = AM_HAL_PIN_13_ADCD0PSE8;
            }break;
            case 16:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE0;
                pincfg.uFuncSel = AM_HAL_PIN_16_ADCSE0;
            }break;
            case 29:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE1;
                pincfg.uFuncSel = AM_HAL_PIN_29_ADCSE1;
            }break;
            case 31:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE3;
                pincfg.uFuncSel = AM_HAL_PIN_31_ADCSE3;
            }break;
            case 32:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE4;
                pincfg.uFuncSel = AM_HAL_PIN_32_ADCSE4;
            }break;
            case 33:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE5;
                pincfg.uFuncSel = AM_HAL_PIN_33_ADCSE5;
            }break;
            case 34:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE6;
                pincfg.uFuncSel = AM_HAL_PIN_34_ADCSE6;
            }break;
            case 35:
            {
                chan = AM_HAL_ADC_SLOT_CHSEL_SE7;
                pincfg.uFuncSel = AM_HAL_PIN_35_ADCSE7;
            }break;
            default:
            {
                return false;
            }
            break;
        }
        markDirty();
        channel.eChannel = chan;
    }
    return true;
}

void Apollo3ADC_Slot::setWindowsCompareEnable(bool enable){
    markDirty();
    channel.bWindowCompare = enable;
}

void Apollo3ADC_Slot::setEnable(bool enable){
    markDirty();
    channel.bEnabled = enable;
}

int32_t Apollo3ADC_Slot::readAnalog(PinName name){
    setPin(name);
    return readAnalog();
}

int32_t Apollo3ADC_Slot::readAnalog(){
    startSample();
    int32_t result;
    readResult(result);
    return result;
}

int32_t Apollo3ADC_Slot::startSample(){
    uint32_t status = AM_HAL_STATUS_SUCCESS;
    if(!channel.bEnabled)
    {
        setEnable(true);
    }
    if(dirty)
    {
        status = commit();
        if(status != AM_HAL_STATUS_SUCCESS)
        {
            return 0;
        }
    }


    // Clear the ADC interrupt.
    am_hal_adc_interrupt_status(adc->g_ADCHandle, &ui32IntMask, false);
    uint32_t st = am_hal_adc_interrupt_clear(adc->g_ADCHandle, ui32IntMask);

    MBED_ASSERT(AM_HAL_STATUS_SUCCESS == st);
    
    // status = am_hal_adc_disable(adc->g_ADCHandle);
    status = am_hal_adc_enable(adc->g_ADCHandle);
    if(status != AM_HAL_STATUS_SUCCESS){ 
        Serial.println("Enable FAIL!");
        return status; 
    }

    if(channel.eMeasToAvg == AM_HAL_ADC_SLOT_AVG_1)
    {
        am_hal_adc_sw_trigger(adc->g_ADCHandle);
    }else{
        am_hal_ctimer_start(Apollo3ADC_Timer, AM_HAL_CTIMER_TIMERA);
        am_hal_adc_sw_trigger(adc->g_ADCHandle);
        
    }
    return status;
}

int32_t Apollo3ADC_Slot::readResult(int32_t &result)
{
    am_hal_adc_sample_t Sample;
    Sample.ui32Slot = this->slot;
    uint32_t ui32NumSamples = 1;    
    uint32_t status = AM_HAL_STATUS_SUCCESS;
    
    do { // Wait for interrupt
        MBED_ASSERT(AM_HAL_STATUS_SUCCESS == am_hal_adc_interrupt_status(adc->g_ADCHandle, &ui32IntMask, false));
    } while(!(ui32IntMask & AM_HAL_ADC_INT_CNVCMP));
    
    if(channel.eMeasToAvg != AM_HAL_ADC_SLOT_AVG_1)
    {
        am_hal_ctimer_stop(Apollo3ADC_Timer, AM_HAL_CTIMER_TIMERA);
    }
    
    am_hal_adc_interrupt_clear(adc->g_ADCHandle, 0xFFFFFFFF);

    MBED_ASSERT(AM_HAL_STATUS_SUCCESS == am_hal_adc_samples_read(adc->g_ADCHandle, false, NULL, &ui32NumSamples, &Sample));
    
    result = Sample.ui32Sample;

    return status;
}


Apollo3ADC::Apollo3ADC()
{

}

void Apollo3ADC::markDirty()
{
    dirty = true;
}

void Apollo3ADC::initTimer()
{
    am_hal_ctimer_config_single(Apollo3ADC_Timer, averaging_timer,
                            averaging_freq |
                            AM_HAL_CTIMER_FN_REPEAT |
                            AM_HAL_CTIMER_INT_ENABLE
    );

    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA3);

    am_hal_ctimer_period_set(Apollo3ADC_Timer, averaging_timer, averaging_period, averaging_on_time);    
    am_hal_ctimer_adc_trigger_enable();
}

void Apollo3ADC::setAveragingFreq(uint32_t freq)
{
    if(averaging_freq != freq){markDirty();}
    averaging_freq = freq;
}

void Apollo3ADC::setAveragingPeriod(uint32_t period)
{
    if(averaging_period != period){markDirty();}
    averaging_period = period;
}

void Apollo3ADC::setAveragingOnTime(uint32_t onTime)
{
    if(averaging_on_time != onTime){markDirty();}
    averaging_on_time = onTime;
}

void Apollo3ADC::setAveragingTimer(uint32_t timer)
{
    if(averaging_on_time != timer){markDirty();}
    averaging_timer = timer;
}

uint32_t Apollo3ADC::begin()
{
    // analogRead();
    am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_ADC);
    uint32_t status = AM_HAL_STATUS_SUCCESS;
    if(!started){
        for(size_t i = 0; i < Apollo3ADC_SLOTS; i++)
        {
            slots[i] = NULL;
        }

        status = am_hal_adc_initialize(adc_number, &g_ADCHandle);
        if(status != AM_HAL_STATUS_SUCCESS){ return status; }


        status = am_hal_adc_power_control(g_ADCHandle, AM_HAL_SYSCTRL_WAKE, false);
        if(status != AM_HAL_STATUS_SUCCESS){ return status; }

        // status = am_hal_adc_interrupt_enable(g_ADCHandle, AM_HAL_ADC_INT_CNVCMP );
        // if(status != AM_HAL_STATUS_SUCCESS){ return status; }
        
        started = true;
    }

    return status;
}

uint32_t Apollo3ADC::commit()
{
    if(dirty)
    {
        if(!started)
        {
            begin();
        }
        uint32_t r = am_hal_adc_configure(g_ADCHandle, &ADCConfig);  
        am_hal_adc_disable(g_ADCHandle);      

        return r;
    }
    return AM_HAL_STATUS_SUCCESS;
}

void Apollo3ADC::setClock(am_hal_adc_clksel_e clock)
{
    markDirty();
    ADCConfig.eClock = clock;
}

void Apollo3ADC::setTriggerPolarity(am_hal_adc_trigpol_e polarity)
{
    markDirty();
    ADCConfig.ePolarity = polarity;
}

void Apollo3ADC::setTrigger(am_hal_adc_trigsel_e trigger)
{
    markDirty();
    ADCConfig.eTrigger = trigger;
}

void Apollo3ADC::setReference(am_hal_adc_refsel_e reference)
{
    markDirty();
    ADCConfig.eReference = reference;
}

void Apollo3ADC::setClockMode(am_hal_adc_clkmode_e mode)
{
    markDirty();
    ADCConfig.eClockMode = mode;
}

void Apollo3ADC::setLowPowerMode(am_hal_adc_lpmode_e power_mode)
{
    markDirty();
    ADCConfig.ePowerMode = power_mode;
}

void Apollo3ADC::setRepeat(am_hal_adc_repeat_e repeat)
{
    markDirty();
    ADCConfig.eRepeat = repeat;
}


Apollo3ADC_Slot* Apollo3ADC::getADCSlot(Apollo3ADC_Slot_e slot)
{
    if(slots[slot] == NULL)
    {
        slots[slot] = new Apollo3ADC_Slot(this, slot);    
        slots[slot]->setEnable(true);
    }
    return slots[slot];
}

uint32_t Apollo3ADC::end()
{
    uint32_t status = AM_HAL_STATUS_SUCCESS;
    if(started)
    {
        status = am_hal_adc_disable(g_ADCHandle);
        if(status != AM_HAL_STATUS_SUCCESS){ return status; }

        status = am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
        if(status != AM_HAL_STATUS_SUCCESS){ return status; }

        status = am_hal_adc_deinitialize(g_ADCHandle);
        if(status != AM_HAL_STATUS_SUCCESS){ return status; }
        started = false;
    }
    return status;
}

Apollo3ADC::~Apollo3ADC()
{
    end();
    for(size_t i = 0; i < Apollo3ADC_SLOTS; i++)
    {
        if(slots[i] != NULL)
        {
            delete slots[i];
        }
    }
}