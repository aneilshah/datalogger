#include "datastore.h"

// ################################################################################################
// CLASS DataStoreInt
// ################################################################################################

DataStoreInt::DataStoreInt(int size, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _units = units;
  reset();
}

DataStoreInt::DataStoreInt() {
  _units = "";
  _maxCount = DATASTORE_MAX;
  reset();
}

float DataStoreInt::avg() {
  uint32_t sum = 0;

  for (int i = 0; i < _dataCount; i++) {
    sum += _data[i];
  }

  float result = 0.0;
  if (_dataCount) result = float(sum / _dataCount);
  return result;
}

int DataStoreInt::getData(int i) const {
  if (i < 0 || i >= _dataCount) return 0;
  return _data[i];
}

float DataStoreInt::stdev() {
  float devSum = 0;
  float delta = 0;
  const float Avg = avg();

  for (int i = 0; i < _dataCount; i++) {
    delta = float(_data[i] - Avg);
    devSum += delta * delta;
  }

  float result = 0.0;
  if (_dataCount) result = float(sqrt(devSum / _dataCount));
  return result;
}

void DataStoreInt::reset() {
  _dataCount = 0;
  _totalCount = 0;
  for (int i = 0; i < _maxCount; i++) _data[i] = 0;
}

void DataStoreInt::init(int size, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _units = units;
}

long DataStoreInt::getTotalCount() {return _totalCount;}

void DataStoreInt::addData(int x) {
  for (int i = _maxCount - 1; i > 0; i--) _data[i] = _data[i - 1]; 
  _data[0] = x;
  if (_dataCount < _maxCount) _dataCount++;
  _totalCount++;
}

String DataStoreInt::dataText() {
  float Avg = avg();
  float Stdev = stdev();
  String str = "";
  int Max = _dataCount;
  if (Max > DATASTORE_MAX) Max = DATASTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + " ";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());
  str += " | StDev:" + String(stdev());

  return str;
}

String DataStoreInt::htmlText() {
  float Avg = avg();
  float Stdev = stdev();
  String str = "";
  int Max = _dataCount;
  if (Max > DATASTORE_MAX) Max = DATASTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + "&ensp;";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());
  str += " | StDev:" + String(stdev());

  return str;
}

// ################################################################################################
// CLASS DataStoreFloat
// ################################################################################################

DataStoreFloat::DataStoreFloat(int size, int prec, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _prec = prec;
  _units = units;
  reset();
}

DataStoreFloat::DataStoreFloat() {
  _units = "";
  _maxCount = DATASTORE_MAX;
  _prec = 1;
  reset();
}

float DataStoreFloat::avg() {
  float sum = 0;

  for (int i = 0; i < _dataCount; i++) {
    sum += _data[i];
  }

  float result = 0.0;
  if (_dataCount) result = float(sum / _dataCount);
  return result;
}

float DataStoreFloat::stdev() {
  float devSum = 0;
  float delta = 0;
  const float Avg = avg();

  for (int i = 0; i < _dataCount; i++) {
    delta = float(_data[i] - Avg);
    devSum += delta * delta;
  }

  float result = 0.0;
  if (_dataCount) result = float(sqrt(devSum / _dataCount));
  return result;
}

void DataStoreFloat::reset() {
  _dataCount = 0;
  _totalCount = 0;
  for (int i = 0; i < _maxCount; i++) _data[i] = 0.0;
}

void DataStoreFloat::addData(float x) {
  for (int i = _maxCount - 1; i > 0; i--) _data[i] = _data[i - 1]; 
  _data[0] = x;
  if (_dataCount < _maxCount) _dataCount++;
  _totalCount++;
}

void DataStoreFloat::init(int size, int prec, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _units = units;
  _prec = prec;
}

long  DataStoreFloat::getTotalCount() {return _totalCount;}

String DataStoreFloat::dataText() {
  float Avg = avg();
  float Stdev = stdev();
  String str = "";
  int Max = _dataCount;
  if (Max > DATASTORE_MAX) Max = DATASTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i],_prec) + " ";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());
  str += " | StDev:" + String(stdev());

  return str;
}


// ################################################################################################
// CLASS CurrentStoreInt
// ################################################################################################

CurrentStoreInt::CurrentStoreInt() {
  _units = "";
  _maxCount = CURRENTSTORE_MAX;
  reset();
}

float CurrentStoreInt::avg() {
  uint32_t sum = 0;

  for (int i = 0; i < _dataCount; i++) {
    sum += _data[i];
  }

  float result = 0.0;
  if (_dataCount) result = float(sum / _dataCount);
  return result;
}

float CurrentStoreInt::ssAvg(float tolPercent) { // Steady State average, tolPercent = 0.1 for 10%
  if (_dataCount <= 0) return 0.0f;

  // First-pass average (includes inrush/tail)
  const float a0 = avg();
  if (a0 <= 0.0f) return a0;  // avoid weirdness when idle/zero

  // Keep only samples within +/- tolPercent of the first average
  const float band = fabsf(a0) * tolPercent;

  uint32_t sum = 0;
  int n = 0;

  for (int i = 0; i < _dataCount; i++) {
    const float x = (float)_data[i];
    if (fabsf(x - a0) <= band) {
      sum += _data[i];
      n++;
    }
  }

  // If filter excluded everything (or almost everything), fall back to a0
  if (n <= 0) return a0;

  return (float)sum / (float)n;
}

void CurrentStoreInt::reset() {
  _dataCount = 0;
  lastDataText = dataText();
  lastHtmlText = htmlText();
  for (int i = 0; i < _maxCount; i++) _data[i] = 0;  // reset data
}

void CurrentStoreInt::addData(int x) {
  for (int i = _maxCount - 1; i > 0; i--) _data[i] = _data[i - 1]; 
  _data[0] = x;
  if (_dataCount < _maxCount) _dataCount++;
}

String CurrentStoreInt::dataText() {
  float Avg = avg();
  String str = "";
  int Max = _dataCount;
  if (Max > CURRENTSTORE_MAX) Max = CURRENTSTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + " ";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());

  return str;
}

String CurrentStoreInt::htmlText() {
  float Avg = avg();
  String str = "";
  int Max = _dataCount;
  if (Max > CURRENTSTORE_MAX) Max = CURRENTSTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + "&ensp;";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());

  return str;
}

String CurrentStoreInt::getLastDataText() {return lastDataText;}
String CurrentStoreInt::getLastHtmlText() {return lastHtmlText;}