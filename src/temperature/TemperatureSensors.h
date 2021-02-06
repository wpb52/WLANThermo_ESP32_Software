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
#define NUM_OF_TYPES 33u


enum class SensorType
{
  Maverick = 0,
  wpb31_K = 1,
  wpb33_K = 2,
  wpb38_K = 3,
  wpb42_K = 4,
  wpb44_K = 5,
  wpb45_K = 6,
  wpb01_K = 7,
  wpb02_K = 8,
  wpb03_K = 9,
  wpb04_K = 10,
  wpb05_K = 11,
  wpb06_K = 12,
  wpb31_D = 13,
  wpb33_D = 14,
  wpb38_D = 15,
  wpb42_D = 16,
  wpb44_D = 17,
  wpb45_D = 18,
  wpb01_D = 19,
  wpb02_D = 20,
  wpb03_D = 21,
  wpb04_D = 22,
  wpb05_D = 23,
  wpb06_D =24,       // Ende der privaten Sensordefinitionen
  w100k6A = 25,      // ab hier nicht einfach zu ändernde Definitionen
  w_200K =26,
  IGrill2 = 27,
  Ble = 28,
  MaverickRadio =29,
  TypeK = 30,
  PT100 = 31,
  PT1000 =32
};

typedef struct
{
  SensorType type;
  const char *name;
  boolean fixed;
} SensorTypeInfoType;

const SensorTypeInfoType sensorTypeInfo[NUM_OF_TYPES] = {
    {SensorType::Maverick, "1000K/Maverick", false},
    {SensorType::wpb31_K, "wpb31_K", false},
    {SensorType::wpb33_K, "wpb33_K", false},
    {SensorType::wpb38_K, "wpb38_K", false},
    {SensorType::wpb42_K, "wpb42_K", false},
    {SensorType::wpb44_K, "wpb44_K", false},
    {SensorType::wpb45_K, "wpb45_K", false},
    {SensorType::wpb01_K, "wpb01_K", false},
    {SensorType::wpb02_K, "wpb02_K", false},
    {SensorType::wpb03_K, "wpb03_K", false},
    {SensorType::wpb04_K, "wpb04_K", false},
    {SensorType::wpb05_K, "wpb05_K", false},
    {SensorType::wpb06_K, "wpb06_K", false}, 
    {SensorType::wpb31_D, "wpb31_D", false},
    {SensorType::wpb33_D, "wpb33_D", false},
    {SensorType::wpb38_D, "wpb38_D", false},
    {SensorType::wpb42_D, "wpb42_D", false},
    {SensorType::wpb44_D, "wpb44_D", false},
    {SensorType::wpb45_D, "wpb45_D", false},
    {SensorType::wpb01_D, "wpb01_D", false},
    {SensorType::wpb02_D, "wpb02_D", false},
    {SensorType::wpb03_D, "wpb03_D", false},
    {SensorType::wpb04_D, "wpb04_D", false},
    {SensorType::wpb05_D, "wpb05_D", false},
    {SensorType::wpb06_D, "wpb06_D", false},
    {SensorType::w100k6A, "w100k6A", false},
    {SensorType::w_200K, "w_200K", false},
    {SensorType::IGrill2, "IGrill2", false},
    {SensorType::Ble, "Bluetooth", true},
    {SensorType::MaverickRadio, "Maverick", true}
    };
