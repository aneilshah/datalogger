#include "loggerFlash.h"

#include <esp_partition.h>
#include <string.h>

static const esp_partition_t *loggerPartition = nullptr;

//-----------------------------------------------------
// Helpers
//-----------------------------------------------------

static uint32_t headerAddress()
{
    return 0;
}

static uint32_t hourAddress(uint16_t hour)
{
    return sizeof(EventLogger::LogHeader) +
           (hour * sizeof(EventLogger::HourRecord));
}

//-----------------------------------------------------
// Initialize
//-----------------------------------------------------

bool loggerFlashInit()
{
    loggerPartition =
        esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            (esp_partition_subtype_t)0x40,
            "logger");

    return (loggerPartition != nullptr);
}

//-----------------------------------------------------
// Erase Logger Partition
//-----------------------------------------------------

bool loggerFlashErase()
{
    if (loggerPartition == nullptr)
        return false;

    esp_err_t err =
        esp_partition_erase_range(
            loggerPartition,
            0,
            loggerPartition->size);

    return (err == ESP_OK);
}

//-----------------------------------------------------
// Write Header
//-----------------------------------------------------

bool loggerFlashWriteHeader(
    const EventLogger::LogHeader &header)
{
    if (loggerPartition == nullptr)
        return false;

    esp_err_t err =
        esp_partition_write(
            loggerPartition,
            headerAddress(),
            &header,
            sizeof(header));

    return (err == ESP_OK);
}

//-----------------------------------------------------
// Read Header
//-----------------------------------------------------

bool loggerFlashReadHeader(
    EventLogger::LogHeader &header)
{
    if (loggerPartition == nullptr)
        return false;

    esp_err_t err =
        esp_partition_read(
            loggerPartition,
            headerAddress(),
            &header,
            sizeof(header));

    return (err == ESP_OK);
}

//-----------------------------------------------------
// Append Hour
//-----------------------------------------------------

bool loggerFlashAppendHour(
    const EventLogger::HourRecord &hour)
{
    if (loggerPartition == nullptr)
        return false;

    EventLogger::LogHeader header;

    if (!loggerFlashReadHeader(header))
        return false;

    uint32_t address =
        hourAddress(header.hoursStored);

    //
    // Make sure we don't write past the partition.
    //
    if (address + sizeof(hour) > loggerPartition->size)
        return false;

    esp_err_t err =
        esp_partition_write(
            loggerPartition,
            address,
            &hour,
            sizeof(hour));

    if (err != ESP_OK)
        return false;

    header.hoursStored++;

    return loggerFlashWriteHeader(header);
}

//-----------------------------------------------------
// Read Hour
//-----------------------------------------------------

bool loggerReadHour(
    uint16_t hourIndex,
    EventLogger::HourRecord &hour)
{
    if (loggerPartition == nullptr)
        return false;

    EventLogger::LogHeader header;

    if (!loggerFlashReadHeader(header))
        return false;

    if (hourIndex >= header.hoursStored)
        return false;

    uint32_t address =
        hourAddress(hourIndex);

    esp_err_t err =
        esp_partition_read(
            loggerPartition,
            address,
            &hour,
            sizeof(hour));

    return (err == ESP_OK);
}