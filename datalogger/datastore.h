#pragma once

#include "global.h"
#include <string.h>

#define DATASTORE_MAX 50
#define CURRENTSTORE_MAX 100

class DataStoreInt {
  public:
    DataStoreInt(int size, String units);
    DataStoreInt();
    float avg();
    float ssAvg(float tolPct = 0.10f);   // Steady State Avg
    float stdev();
    void reset();
    void addData(int x);
    long getTotalCount();
    String dataText();
    String htmlText();
    void init(int size, String units);
    int getData(int i) const;
    
  private:
    int _data[DATASTORE_MAX+2];
    int _dataCount;
    int _maxCount;
    long _totalCount;
    String _units;
};


class DataStoreFloat {
  public:
    DataStoreFloat(int size, int prec, String units);
    DataStoreFloat();
    float avg();
    float stdev();
    void reset();
    void addData(float x);
    String dataText();
    long getTotalCount();
    void init(int size, int prec, String units);
    
  private:
    float _data[DATASTORE_MAX+2];
    int _dataCount;
    int _maxCount;
    int _prec;
    long _totalCount;
    String _units;
};

class CurrentStoreInt {
  public:
    CurrentStoreInt();
    float avg();
    float ssAvg(float tolPercent = 0.2f);
    void reset();
    void addData(int x);
    String dataText();
    String htmlText();
    String getLastDataText();
    String getLastHtmlText();
    
  private:
    int _data[CURRENTSTORE_MAX+2];
    int _dataCount;
    int _maxCount;
    String _units;
    String lastDataText;
    String lastHtmlText;
};
