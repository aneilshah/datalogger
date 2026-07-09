#include "eventData.h"

#include <string.h>
#include <time.h>

#include "ntp.h"

namespace {

class EventData {
public:
  static const int MAX_EVENTS = 250;
  static const uint32_t WINDOW_MINUTES = 24U * 60U;

  EventData() {
    clear();
  }

  void clear() {
    memset(eventMinute_, 0, sizeof(eventMinute_));
    head_ = 0;
    count_ = 0;
  }

  bool addNow() {
    if (!validClockNow()) return false;

    const uint32_t minute2026 = getMinutesSince2026Now();
    if (minute2026 == 0) return false;

    addMinute(minute2026);
    return true;
  }

  void addMinute(uint32_t minute2026) {
    // De-dupe: ignore repeated events in same minute
    if (count_ > 0) {
      uint32_t newestMinute = 0;
      if (get(count_ - 1, newestMinute) && newestMinute == minute2026) {
        purgeOld(minute2026);
        return;
      }
    }

    eventMinute_[head_] = minute2026;
    head_ = (head_ + 1) % MAX_EVENTS;

    if (count_ < MAX_EVENTS) {
      count_++;
    }

    purgeOld(minute2026);
  }

  void purgeOld(uint32_t nowMinute2026) {
    while (count_ > 0) {
      const int idx = oldestIndex();
      const uint32_t eventMinute = eventMinute_[idx];

      // Guard bad clock jumps
      if (eventMinute > nowMinute2026) {
        break;
      }

      const uint32_t ageMinutes = nowMinute2026 - eventMinute;
      if (ageMinutes <= WINDOW_MINUTES) {
        break;
      }

      count_--;
    }
  }

  int count() const {
    return count_;
  }

  bool empty() const {
    return count_ == 0;
  }

  bool get(int logicalIndex, uint32_t& outMinute2026) const {
    if (logicalIndex < 0 || logicalIndex >= count_) return false;

    outMinute2026 = eventMinute_[physicalIndex(logicalIndex)];
    return true;
  }

  static bool validClockNow() {
    return validClock() && getCurrentEpoch() > 0;
  }

  static uint32_t getMinutesSince2026Now() {
    const uint32_t nowEpoch = getCurrentEpoch();
    if (nowEpoch == 0) return 0;

    struct tm baseTm;
    memset(&baseTm, 0, sizeof(baseTm));
    baseTm.tm_year = 2026 - 1900;
    baseTm.tm_mon = 0;
    baseTm.tm_mday = 1;

    const time_t baseEpoch = mktime(&baseTm);
    if (baseEpoch <= 0) return 0;
    if (nowEpoch <= (uint32_t)baseEpoch) return 0;

    return (nowEpoch - (uint32_t)baseEpoch) / 60U;
  }

private:
  uint32_t eventMinute_[MAX_EVENTS];
  int head_;
  int count_;

  int oldestIndex() const {
    return (head_ - count_ + MAX_EVENTS) % MAX_EVENTS;
  }

  int physicalIndex(int logicalIndex) const {
    return (oldestIndex() + logicalIndex) % MAX_EVENTS;
  }
};

static EventData gEventData;

} // namespace

// ---- Public API ----

void eventDataInit() {
  gEventData.clear();
}

void eventDataClear() {
  gEventData.clear();
}

bool eventDataAddNow() {
  return gEventData.addNow();
}

void eventDataAddMinute(uint32_t minute2026) {
  gEventData.addMinute(minute2026);
}

int eventDataCount() {
  return gEventData.count();
}

bool eventDataEmpty() {
  return gEventData.empty();
}

bool eventDataGetMinute(int index, uint32_t& outMinute2026) {
  return gEventData.get(index, outMinute2026);
}

bool eventDataValidClock() {
  return EventData::validClockNow();
}

uint32_t eventDataGetMinutesSince2026() {
  return EventData::getMinutesSince2026Now();
}

void eventDataPurgeNow() {
  uint32_t now = EventData::getMinutesSince2026Now();
  if (now == 0) return;

  gEventData.purgeOld(now);
}