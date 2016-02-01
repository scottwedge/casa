/*
 * WeatherRecord.h
 *
 *  Created on: Jan 27, 2016
 *      Author: nakazato
 */

#ifndef SINGLEDISH_FILLER_WeatherRECORD_H_
#define SINGLEDISH_FILLER_WeatherRECORD_H_

#include <casacore/casa/BasicSL/String.h>
#include <casacore/measures/Measures/MPosition.h>
#include <casacore/ms/MeasurementSets/MSWeather.h>
#include <casacore/ms/MeasurementSets/MSWeatherColumns.h>

namespace casa { //# NAMESPACE CASA - BEGIN

struct WeatherRecord {
  typedef MSWeather AssociatingTable;
  typedef MSWeatherColumns AssociatingColumns;

  // mandatory
  Int antenna_id;
  Double time;
  Double interval;

  // optional
  Float temperature;
  Float pressure;
  Float rel_humidity;
  Float wind_speed;
  Float wind_direction;

  // method
  void clear() {
    antenna_id = -1;
    time = 0.0;
    interval = 0.0;
    temperature = -1.0;
    pressure = -1.0;
    rel_humidity = -1.0;
    wind_speed = -1.0;
    wind_direction = -1.0;
  }

  WeatherRecord &operator=(WeatherRecord const &other) {
    antenna_id = other.antenna_id;
    time = other.time;
    interval = other.interval;
    temperature = other.temperature;
    pressure = other.pressure;
    rel_humidity = other.rel_humidity;
    wind_speed = other.wind_speed;
    wind_direction = other.wind_direction;
    return *this;
  }

  void add(AssociatingTable &table, AssociatingColumns &/*columns*/) {
    table.addRow(1, True);
  }

  Bool fill(uInt irow, AssociatingColumns &columns) {
    if (columns.nrow() <= irow) {
      return False;
    }

    columns.antennaId().put(irow, antenna_id);
    columns.time().put(irow, time);
    columns.interval().put(irow, interval);
    columns.temperature().put(irow, temperature);
    columns.pressure().put(irow, pressure);
    columns.relHumidity().put(irow, rel_humidity);
    columns.windSpeed().put(irow, wind_speed);
    columns.windDirection().put(irow, wind_direction);

    return True;
  }
};

} //# NAMESPACE CASA - END

#endif /* SINGLEDISH_FILLER_WeatherRECORD_H_ */
