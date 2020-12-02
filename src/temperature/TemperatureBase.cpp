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
  float a, b, c, Rn;

  // kleine Abweichungen an GND verursachen Messfehler von wenigen Digitalwerten
  // daher werden nur Messungen mit einem Digitalwert von mind. 10 ausgewertet,
  // das entspricht 5 mV
  if (rawValue < 10)
    return INACTIVEVALUE; // Kanal ist mit GND gebrückt

  switch (type)
  {
  case SensorType::Maverick: // Maverik
    Rn = 1000;
    a = 0.003358;
    b = 0.0002242;
    c = 0.00000261;
    break;
  case SensorType::wpb1000k1: // wpb1000k1	 
    Rn = 1.00000000E+03;
    a = 3.35357526E-03;	 
    b = 2.25076982E-04;
    c = 2.73856371E-06
    break;
  case SensorType::wpb1000k2: // wpb1000k2	 
    Rn = 1.00000000E+03;	 
    a = 3.35458439E-03;	 
    b = 2.26028596E-04;	 
    c = 2.93121094E-06;
    break;
  case SensorType::wpb1000k3: // wpb1000k3	 
    Rn = 1.00000000E+03;	 
    a = 3.35534794E-03;	 
    b = 2.25498257E-04;	 
    c = 2.78667544E-06;
    break;
  case SensorType::wpb1000k4: // wpb1000k4	 
    Rn = 	1.00000000E+03;	 
    a = 3.35404928E-03;	 
    b = 2.26045590E-04;	 
    c = 2.88726657E-06;
    break;
  case SensorType::wpb1000k5: // wpb1000k5	 
    Rn = 	1.00000000E+03;	 
    a = 3.35472823E-03;	 
    b = 2.25778383E-04;	 
    c = 2.82085116E-06;
    break;
  case SensorType::wpb1000k6: // wpb1000k6	 
    Rn = 	1.00000000E+03;	 
    a = 3.35364502E-03;	 
    b = 2.24978671E-04;	 
    c = 2.73823940E-06;
    break;
  case SensorType::wpb1000k0: // wpb1000k0	 Mittelwert
    Rn = 	1.00000000E+03;	 
    a = 3.35425697E-03;	 
    b = 2.25568425E-04;	 
    c = 2.82576794E-06;
    break;
  case SensorType::NTC100K6A1B: // NTC 100K6A1B (lila Kopf)
    Rn = 100;
    a = 0.00335639;
    b = 0.000241116;
    c = 0.00000243362;
    break;
  case SensorType::Weber6743: // Weber_6743
    Rn = 102.315;
    a = 3.3558796e-03;
    b = 2.7111149e-04;
    c = 3.1838428e-06;
    break;
  case SensorType::Santos: // Santos
    Rn = 200.82;
    a = 3.3561093e-03;
    b = 2.3552814e-04;
    c = 2.1375541e-06;
    break;
  case SensorType::NTC5K3A1B: // NTC 5K3A1B (orange Kopf)
    Rn = 5;
    a = 0.0033555;
    b = 0.0002570;
    c = 0.00000243;
    break;
  case SensorType::ThermoWorks: // ThermoWorks
    Rn = 97.31;
    a = 3.3556417e-03;
    b = 2.5191450e-04;
    c = 2.3606960e-06;
    break;
  default:
    return INACTIVEVALUE;
  }

  float Rt = Rmess * ((4096.0 / (4096 - rawValue)) - 1);
  float v = log(Rt / Rn);
  float erg = (1 / (a + b * v + c * v * v)) - 273.15;

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