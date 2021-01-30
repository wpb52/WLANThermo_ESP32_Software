/*************************************************** 
    Copyright (C) 2020  Martin Koerner

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    HISTORY: Please refer Github History
    
****************************************************/
#pragma once

#include "Arduino.h"
#define NUM_OF_TYPES 18u


enum class SensorType
{
  Maverick = 0,
  wpb1M31 = 1,
  wpb1M33 = 2,
  wpb1M38 = 3,
  wpb1M42 = 4,
  wpb1M44 = 5,
  wpb1M45 = 6,
  wpb1M01 = 7,
  wpb1M02 = 8,
  wpb1M03 = 9,
  wpb1M04 = 10,
  wpb1M05 = 11,
  PT100 = 12,
  PT1000 = 13,
  wpb1M06 = 14,
  TypeK = 15,
  Ble = 16,
  MaverickRadio = 17
};

typedef struct
{
  SensorType type;
  const char *name;
  boolean fixed;
} SensorTypeInfoType;

const SensorTypeInfoType sensorTypeInfo[NUM_OF_TYPES] = {
    {SensorType::Maverick, "1000K/Maverick", false},
    {SensorType::wpb1M31, "wpb1M31", false},
    {SensorType::wpb1M33, "wpb1M33", false},
    {SensorType::wpb1M38, "wpb1M38", false},
    {SensorType::wpb1M42, "wpb1M42", false},
    {SensorType::wpb1M44, "wpb1M44", false},
    {SensorType::wpb1M45, "wpb1M45", false},
    {SensorType::wpb1M01, "wpb1M01", false},
    {SensorType::wpb1M02, "wpb1M02", false},
    {SensorType::wpb1M03, "wpb1M03", false},
    {SensorType::wpb1M04, "wpb1M04", false},
    {SensorType::wpb1M05, "wpb1M05", false},
    {SensorType::PT100, "PT100", false},
    {SensorType::PT1000, "PT1000", false},
    {SensorType::wpb1M06, "wpb1M06", false},
    {SensorType::TypeK, "Typ K", true},
    {SensorType::Ble, "Bluetooth", true},
    {SensorType::MaverickRadio, "Maverick", true}};
