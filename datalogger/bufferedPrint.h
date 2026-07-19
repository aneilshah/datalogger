#pragma once

#include <Arduino.h>

class BufferedPrint : public Print
{
public:

  explicit BufferedPrint(Print &output);
  ~BufferedPrint();

  using Print::write;

  size_t write(uint8_t c) override;
  size_t write(const uint8_t *data,
               size_t length) override;

  void flush();

private:

  static constexpr size_t BUFFER_SIZE = 1024;

  Print &out;

  char buffer[BUFFER_SIZE];

  size_t used = 0;
};