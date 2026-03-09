#pragma once

#include "core/device/device.h"
#include <ctime>
#include <functional>

// ARM PrimeCell PL031 RTC emulation (MMIO-based).
// Returns host wall-clock time via RTCDR; alarms and interrupts are stubbed.
class Pl031Rtc : public Device {
public:
    static constexpr uint64_t kMmioSize = 0x1000;

    using IrqLevelCallback = std::function<void(bool asserted)>;
    void SetIrqLevelCallback(IrqLevelCallback cb) { irq_level_callback_ = std::move(cb); }

    void MmioRead(uint64_t offset, uint8_t size, uint64_t* value) override;
    void MmioWrite(uint64_t offset, uint8_t size, uint64_t value) override;

private:
    void UpdateIrq();

    // PL031 register offsets
    static constexpr uint64_t kRTCDR  = 0x000;  // Data Register (RO)
    static constexpr uint64_t kRTCMR  = 0x004;  // Match Register
    static constexpr uint64_t kRTCLR  = 0x008;  // Load Register
    static constexpr uint64_t kRTCCR  = 0x00C;  // Control Register
    static constexpr uint64_t kRTCIMSC = 0x010; // Interrupt Mask Set/Clear
    static constexpr uint64_t kRTCRIS = 0x014;  // Raw Interrupt Status
    static constexpr uint64_t kRTCMIS = 0x018;  // Masked Interrupt Status
    static constexpr uint64_t kRTCICR = 0x01C;  // Interrupt Clear

    // PrimeCell identification registers
    static constexpr uint64_t kPERIPH_ID0 = 0xFE0;
    static constexpr uint64_t kPERIPH_ID1 = 0xFE4;
    static constexpr uint64_t kPERIPH_ID2 = 0xFE8;
    static constexpr uint64_t kPERIPH_ID3 = 0xFEC;
    static constexpr uint64_t kCELL_ID0   = 0xFF0;
    static constexpr uint64_t kCELL_ID1   = 0xFF4;
    static constexpr uint64_t kCELL_ID2   = 0xFF8;
    static constexpr uint64_t kCELL_ID3   = 0xFFC;

    uint32_t match_reg_ = 0;
    uint32_t load_reg_ = 0;
    uint32_t cr_ = 1;         // RTC enabled by default
    uint32_t imsc_ = 0;       // All interrupts masked
    uint32_t ris_ = 0;        // No raw interrupts pending

    // Offset applied when guest writes RTCLR: guest_time = host_time + tick_offset_
    int64_t tick_offset_ = 0;

    IrqLevelCallback irq_level_callback_;
};
