/*************************************************** 
    Copyright (C) 2019  Martin Koerner

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

#include "TemperatureBase.h"
#include "TemperatureGrp.h"
#include "Settings.h"

#define LOWEST_VALUE -31
#define HIGHEST_VALUE 999
#define DEFAULT_MIN_VALUE 10.0
#define DEFAULT_MAX_VALUE 35.0
#define MAX_COLORS 8u
#define MEDIAN_SIZE 9u
#define DEFAULT_CHANNEL_NAME "Kanal "

const static String colors[MAX_COLORS] = {"#0C4C88", "#22B14C", "#EF562D", "#FFC100", "#A349A4", "#804000", "#5587A2", "#5C7148"};
TemperatureCalculation_t TemperatureBase::typeFunctions[NUM_OF_TYPES] = {
    TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC,
    TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC,
    TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC,
    TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC, TemperatureBase::calcTemperatureNTC,
    TemperatureBase::calcTemperaturePTx, TemperatureBase::calcTemperaturePTx, TemperatureBase::calcTemperatureNTC,
    NULL, NULL};

TemperatureBase::TemperatureBase()
{
  this->medianValue = new MedianFilter<float>(MEDIAN_SIZE);
  this->fixedSensor = false;
  this->loadDefaultValues(TemperatureGrp::count());
  this->settingsChanged = false;
  this->cbCurrentValue = INACTIVEVALUE;
  this->cbAlarmStatus = NoAlarm;
  this->calcTemperature = typeFunctions[0];
  this->acknowledgedAlarm = false;
  this->connected = false;
}

TemperatureBase::~TemperatureBase()
{
}

void TemperatureBase::loadDefaultValues(uint8_t index)
{
  this->currentUnit = Celsius;
  this->currentValue = INACTIVEVALUE;
  this->preValue = INACTIVEVALUE;
  this->currentGradient = 0;
  this->gradientSign = 0;
  this->minValue = DEFAULT_MIN_VALUE;
  this->maxValue = DEFAULT_MAX_VALUE;
  this->name = DEFAULT_CHANNEL_NAME + String(index + 1u);

  if (false == this->isFixedSensor())
  {
    this->type = SensorType::Maverick;
  }

  this->alarmSetting = AlarmOff;
  this->notificationCounter = 1u;

  if (index < MAX_COLORS)
  {
    this->color = colors[index];
  }
  else
  {
    this->color = colors[random(0, MAX_COLORS - 1u)];
  }

  settingsChanged = true;
}

void TemperatureBase::loadConfig()
{
  DynamicJsonBuffer jsonBuffer(Settings::jsonBufferSize);
  JsonObject &json = Settings::read(kChannels, &jsonBuffer);

  if (json.success())
  {
    for (uint8_t i = 0u; i < json["tname"].size(); i++)
    {
      if (json.containsKey("taddress") && json.containsKey("tlindex"))
      {
        if ((this->address == json["taddress"][i].asString()) && (this->localIndex == json["tlindex"][i].as<uint8_t>()))
        {
          this->name = json["tname"][i].asString();
          this->type = (SensorType)json["ttyp"][i].as<uint8_t>();
          setType((uint8_t)this->type);
          this->minValue = json["tmin"][i];
          this->maxValue = json["tmax"][i];
          this->alarmSetting = (AlarmSetting)json["talarm"][i].as<uint8_t>();
          this->color = json["tcolor"][i].asString();
        }
      }
    }
  }
}

boolean TemperatureBase::checkNewValue()
{
  boolean newValue = false;

  AlarmStatus newAlarmStatus = getAlarmStatus();

  if ((cbAlarmStatus != newAlarmStatus) || (cbCurrentValue != currentValue))
  {
    cbAlarmStatus = newAlarmStatus;
    cbCurrentValue = getValue();
    newValue = true;
  }

  return newValue;
}

boolean TemperatureBase::checkNewSettings()
{
  boolean newSettings = settingsChanged;
  settingsChanged = false;

  return newSettings;
}

float TemperatureBase::getValue()
{
  return (this->currentValue == INACTIVEVALUE) ? INACTIVEVALUE : getUnitValue(this->currentValue);
}

float TemperatureBase::getPreValue()
{
  return (this->preValue == INACTIVEVALUE) ? INACTIVEVALUE : getUnitValue(this->preValue);
}

int8_t TemperatureBase::getGradient()
{
  return this->currentGradient;
}

float TemperatureBase::getMinValue()
{
  return getUnitValue(this->minValue);
}

float TemperatureBase::getMaxValue()
{
  return getUnitValue(this->maxValue);
}

String TemperatureBase::getName()
{
  return this->name;
}

String TemperatureBase::getAddress()
{
  return this->address;
}

String TemperatureBase::getColor()
{
  return this->color;
}

AlarmSetting TemperatureBase::getAlarmSetting()
{
  return this->alarmSetting;
}

uint8_t TemperatureBase::getType()
{
  return (uint8_t)this->type;
}

uint8_t TemperatureBase::getTypeCount()
{
  return NUM_OF_TYPES;
}

String TemperatureBase::getTypeName()
{
  return getTypeName((uint8_t)this->type);
}

String TemperatureBase::getTypeName(uint8_t index)
{
  return (index < NUM_OF_TYPES) ? sensorTypeInfo[index].name : "";
}

boolean TemperatureBase::isTypeFixed(uint8_t index)
{
  return (index < NUM_OF_TYPES) ? sensorTypeInfo[index].fixed : false;
}

void TemperatureBase::setType(uint8_t type)
{
  if (false == this->fixedSensor)
  {
    this->type = (SensorType)type;
    this->calcTemperature = (type < NUM_OF_TYPES) ? typeFunctions[type] : NULL;
  }
}

void TemperatureBase::setMinValue(float value)
{
  if (value > LOWEST_VALUE && value < HIGHEST_VALUE)
  {
    this->minValue = setUnitValue(value);
    settingsChanged = true;
  }
}

void TemperatureBase::setMaxValue(float value)
{
  if (value > LOWEST_VALUE && value < HIGHEST_VALUE)
  {
    this->maxValue = setUnitValue(value);
    settingsChanged = true;
  }
}

void TemperatureBase::setName(const char *name)
{
  this->name = name;
  settingsChanged = true;
}

void TemperatureBase::setAddress(const char *address)
{
  this->address = address;
  settingsChanged = true;
}

void TemperatureBase::setColor(const char *color)
{
  this->color = color;
  settingsChanged = true;
}

void TemperatureBase::setAlarmSetting(AlarmSetting alarmSetting)
{
  this->alarmSetting = alarmSetting;
  settingsChanged = true;
}

void TemperatureBase::setUnit(TemperatureUnit unit)
{
  this->currentUnit = unit;
  settingsChanged = true;
}

uint8_t TemperatureBase::getNotificationCounter()
{
  return this->notificationCounter;
}

void TemperatureBase::setNotificationCounter(uint8_t notificationCounter)
{
  this->notificationCounter = notificationCounter;
}

void TemperatureBase::updateNotificationCounter()
{
  if (this->notificationCounter > 0u)
    this->notificationCounter--;
}

AlarmStatus TemperatureBase::getAlarmStatus()
{
  AlarmStatus status = NoAlarm;

  if (INACTIVEVALUE == this->currentValue)
    status = NoAlarm;
  else if (this->currentValue <= this->minValue)
    status = MinAlarm;
  else if (this->currentValue >= this->maxValue)
    status = MaxAlarm;

  if (NoAlarm == status)
    acknowledgedAlarm = false;

  return status;
}

boolean TemperatureBase::isActive()
{
  return (INACTIVEVALUE != this->currentValue);
}

void TemperatureBase::refresh()
{
  // save last
  this->preValue = this->currentValue;
  int8_t preGradientSign = this->gradientSign;

  // get current
  float currentVal = this->medianValue->GetFiltered();
  float gradient = (isActive() == true) ? decimalPlace(currentVal) - decimalPlace(this->preValue) : 0;
  this->gradientSign = (0 == gradient) ? 0 : (0 < gradient) ? 1 : -1;
  this->currentGradient = (0 == gradient) ? 0 : gradient / abs(gradient);

   
  if (INACTIVEVALUE == currentVal) 
  {
    this->currentValue = INACTIVEVALUE;
  }
  // gradient sign filter
  else if (preGradientSign == gradientSign) 
  {
    if (this->type == SensorType::TypeK && INACTIVEVALUE != preValue)
    {
      this->currentValue = ((currentVal*2.0) + preValue)/3.0;
    }
    else {
      this->currentValue = currentVal;
    }
  }
  else
  {
    this->currentValue = this->preValue;
  }
  
}

void TemperatureBase::update()
{
}

float TemperatureBase::decimalPlace(float value)
{
  return (((int)value * 10.0) / 10.0);
}

float TemperatureBase::getUnitValue(float value)
{
  float convertedValue = value;

  if (this->currentUnit == Fahrenheit)
  {
    convertedValue = ((value * (1.8)) + 32);
  }

  return convertedValue;
}

float TemperatureBase::setUnitValue(float value)
{
  float convertedValue = value;

  if (this->currentUnit == Fahrenheit)
  {
    convertedValue = (value - 32) * (5.0 / 9);
  }

  return convertedValue;
}

float TemperatureBase::calcTemperatureNTC(uint16_t rawValue, SensorType type)
{

  float Rmess = 47;
  float a, b, c, Rn, mOffset;

  // kleine Abweichungen an GND verursachen Messfehler von wenigen Digitalwerten
  // daher werden nur Messungen mit einem Digitalwert von mind. 10 ausgewertet,
  // das entspricht 5 mV
  if (rawValue < 10)
    return INACTIVEVALUE; // Kanal ist mit GND gebrückt
  // wpb: mOffset eingeführt, um geringe Eintauchtiefe des Sensors auszugleichen
  switch (type)
  {
  case SensorType::Maverick: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = 0.0;
    break;
  case SensorType::wpb1M31:   // 
    Rn = 998.200;
    a= 3.3544077E-03;
    b= 2.2489572E-04;  
    c= 2.6892541E-06;
    mOffset = -0.68;                // -0,68 Kessel fix
    break;
  case SensorType::wpb1M33:   // 
    Rn = 995.200;	 
    a = 3.3549731E-03;	 
    b = 2.2569356E-04;	 
    c =2.8392429E-06;
    mOffset = -0.8 + 0.3 + 0.9;                 // -0,8 Kessel fix
    break;
  case SensorType::wpb1M38:   // TopDampf 40mm ET 
    Rn = 996.000	;	 
    a = 3.3539243E-03;	 
    b = 2.2503760E-04;	 
    c = 2.6735522E-06;
    mOffset = -0.8 +0.5+ 1.0;             // -0,8 Kessel fix
    break;
  case SensorType::wpb1M42:   // 
    Rn = 	995.100;	 
    a = 3.3539366E-03;	 
    b = 2.2483282E-04;	 
    c = 2.6320875E-06;
    mOffset = 0.42 -1.5;           //0,42 Dampf fix
    break;
  case SensorType::wpb1M44:   //  
    Rn = 	997.300;	 
    a = 3.3543503E-03;	 
    b = 2.2500659E-04;	 
    c = 2.7100574E-06;
    mOffset = 0.54 -1.7;            // +0,54 Dampf fix
    break;
  case SensorType::wpb1M45:   // Kessel 2	 
    Rn = 	993.000;	 
    a = 3.3541843E-03;	 
    b = 2.2528738E-04;	 
    c = 2.6969023E-06;
    mOffset = +0.65;         // +0,65 Dampf fix
    break;
  case SensorType::wpb1M01: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = + 1.18 ;       // +1,18 DAmpf fix
    break;
  case SensorType::wpb1M02: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = 1.1-1.2;         // 1,1 DAmpf fix
    break;
  case SensorType::wpb1M03: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = 1.3 -1.5;        // +1,3 DAmpf fix
    break;
  case SensorType::wpb1M04: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = -0.115 +0.8 +1.1 ;             //-0,115 DAmpf fix
    break;
  case SensorType::wpb1M05: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = -0.246 + 1.4 +0.2;              //-0,246 DAmpf fix
    break;
  case SensorType::wpb1M06: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    mOffset = -0,31;          //  -0,31 DAmpf fix
    break;
  default:
    return INACTIVEVALUE;
  }

  float Rt = Rmess * ((4096.0 / (4096 - rawValue)) - 1);
  float v = log(Rt / Rn);
  float erg = ((1 / (a + b * v + c * v * v)) - 273.15) * (1+ mOffset/100.0); // 

  return (erg > LOWEST_VALUE) ? erg : INACTIVEVALUE;
}

float TemperatureBase::calcTemperaturePTx(uint16_t rawValue, SensorType type)
{
  float a, b, Rpt, Rmess;

  if (rawValue < 10)
    return INACTIVEVALUE; // Kanal ist mit GND gebrückt

  switch (type)
  {
  case SensorType::PT100: // PT100
  Rpt = 0.1;
  Rmess = 0.0998;
  break;

  case SensorType::PT1000: // PT1000
    Rpt = 1.0;
    Rmess = 0.9792;
    break;

  default:
    return INACTIVEVALUE;
  }

  float Rt = Rmess * ((4096.0 / (4096 - rawValue)) - 1);
  a = 3.9083e-03;
  b = -5.775e-07;
  float erg = (-1) * sqrt((Rt / (Rpt * b)) + ((a * a) / (4 * (b * b))) - 1 / (b)) - (a / (2 * b));

  return (erg > LOWEST_VALUE) ? erg : INACTIVEVALUE;
}