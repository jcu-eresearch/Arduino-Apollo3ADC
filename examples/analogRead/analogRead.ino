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

#include "Arduino.h"
#include "Arduino-Apollo3ADC.h"

Apollo3ADC *adc;
Apollo3ADC_Slot *slot;

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting...  ");
    
    adc = new Apollo3ADC();
    
    /*
    Valid values for setReference:
    
        AM_HAL_ADC_REFSEL_INT_2P0
        AM_HAL_ADC_REFSEL_INT_1P5
        AM_HAL_ADC_REFSEL_EXT_2P0
        AM_HAL_ADC_REFSEL_EXT_1P5
    */
    adc->setReference(AM_HAL_ADC_REFSEL_INT_2P0);

    // Valid values are Apollo3ADC_Slot_0 through Apollo3ADC_Slot_7
    slot = adc->getADCSlot(Apollo3ADC_Slot_1);

    // Valid values are AM_HAL_ADC_SLOT_8IT, AM_HAL_ADC_SLOT_10BIT, AM_HAL_ADC_SLOT_12BIT, AM_HAL_ADC_SLOT_14BIT
    slot->setPrecision(AM_HAL_ADC_SLOT_14BIT);
}


void loop()
{
    long start = micros();
    int32_t val = slot->readAnalog(A29);
    long res = micros() - start;
    Serial.printf("[TS: %lu ms] [Sample Time: %lu us] Reading: %i\r\n", millis(), res, val);
}