#include "bufferedPrint.h"

#include <string.h>

//*****************************************************************************
// Constructor
//*****************************************************************************
BufferedPrint::BufferedPrint(Print &output)
  : out(output)
{
}

//*****************************************************************************
// Destructor
//*****************************************************************************
BufferedPrint::~BufferedPrint()
{
  flush();
}

//*****************************************************************************
// Write One Character
//*****************************************************************************
size_t BufferedPrint::write(uint8_t c)
{
  return write(&c, 1);
}

//*****************************************************************************
// Write Block
//*****************************************************************************
size_t BufferedPrint::write(const uint8_t *data,
                            size_t length)
{
  size_t total = length;

  while (length > 0)
  {
    size_t space = BUFFER_SIZE - used;

    if (space == 0)
    {
      flush();
      space = BUFFER_SIZE;
    }

    size_t count = (length < space) ? length : space;

    memcpy(buffer + used, data, count);

    used += count;
    data += count;
    length -= count;
  }

  return total;
}

//*****************************************************************************
// Flush
//*****************************************************************************
void BufferedPrint::flush()
{
  if (used == 0)
    return;

  out.write((const uint8_t *)buffer, used);

  used = 0;
}