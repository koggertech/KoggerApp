// Host-build shim: the real Serial.hpp needs STM32 HAL types (UART_HandleTypeDef,
// DMA_TYPESTREAM, ...) that only exist in an MCU build.
//
// FrameParser.hpp calls into Serial_c from inline member functions, so the class
// must be complete -- but only its signatures matter here. Nothing about the payload
// structs' sizeof/offsetof depends on any of this doing anything, and the generator
// never constructs one.
#pragma once
#include <stdint.h>
#include <stddef.h>

class Serial_c {
public:
    uint32_t readContext(void*) { return 0; }
    uint32_t writeAvail()       { return 0xFFFF; }
    bool     write(const void*, uint32_t) { return true; }
    bool     write(void*, uint32_t)       { return true; }
};
