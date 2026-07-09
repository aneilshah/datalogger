#pragma once
#include "datastore.h"
#include "pump365.h"

#define GAL_PER_MIN 48
#define GAL_PER_CYCLE 5

#define CUR_WAVE_MAX 600

class PumpData {
  public:
    PumpData();
    float getAvgCycleMin();
    float getStDevCycleMin();
    float getDeltaSec();
    float getDeltaMin();
    float getCPH();
    int getGPD();
    int getPumpEventCount();
    int getLastPumpOnTimeLoops();
    uint32_t getPumpOnTimeLoops();
    uint32_t getLastPumpEventLoops();
    float getPumpCur();
    int getPumpCurCount();
    String getPumpCurText();
    void getPumpCurText(char* out, size_t outSize);
    String getPumpCurTextFull();
    String getCycleDataText();
    String getLastEventCurText();
    String getLastEventSSCurText();
    int getDailyCount();

    DataStoreInt GPDDailyHistory;
    DataStoreFloat CycleDailyHistory;
    DataStoreFloat CPHDailyHistory;
    DataStoreFloat CurrentDailyHistory;

    Pump365Data Daily365;

    int   getLastCycleCurrentSamplesCount() const;
    int   getLastCycleCurrentSampleCounts(int idx) const;
    float getLastCycleCurrentSampleAmps(int idx) const;

    float getLastCycleCurrentAvgAmps() const;
    float getLastCycleCurrentMinAmps() const;
    float getLastCycleCurrentMaxAmps() const;

    float getLastCycleEnergyAmpSeconds() const;
    String getLastCycleCurrentSummaryText() const;

    void processPumpOnEvent(uint32_t loopCount);
    void processPumpOffEvent(int pumpOnTimeLoops);
    void updateAvgCurrent(int adcCount);

  private:
    uint32_t DELTA_SEC;
    float DELTA_MIN;
    int AVG_PUMP_CUR_COUNT;
    int PUMP_EVENT_COUNT;
    int LAST_PUMP_ON_TIME_LOOPS;
    uint32_t PUMP_ON_TIME_LOOPS;
    uint32_t LAST_PUMP_EVENT;
    float PUMP_CPH;
    int PUMP_GPD;
    String LAST_PUMP_EVENT_CUR_TEXT;
    String LAST_PUMP_EVENT_SS_CUR_TEXT;

    DataStoreInt CycData;
    DataStoreInt CycData10;
    CurrentStoreInt Current;

    int curWave[CUR_WAVE_MAX];
    int curWaveCount;
    int lastWave[CUR_WAVE_MAX];
    int lastWaveCount;

    void updateAvgCycleTime();
    void updateTestModeHistoryData();
};

// Pump Instance
extern PumpData Pump;
