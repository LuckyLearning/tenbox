#include "core/device/rtc/pl031_rtc.h"

static uint32_t HostEpoch() {
    return static_cast<uint32_t>(time(nullptr));
}

void Pl031Rtc::MmioRead(uint64_t offset, uint8_t /*size*/, uint64_t* value) {
    switch (offset) {
    case kRTCDR:
        *value = static_cast<uint32_t>(static_cast<int64_t>(HostEpoch()) + tick_offset_);
        return;
    case kRTCMR:
        *value = match_reg_;
        return;
    case kRTCLR:
        *value = load_reg_;
        return;
    case kRTCCR:
        *value = cr_;
        return;
    case kRTCIMSC:
        *value = imsc_;
        return;
    case kRTCRIS:
        *value = ris_;
        return;
    case kRTCMIS:
        *value = ris_ & imsc_;
        return;

    // PrimeCell peripheral ID: PL031 part number = 0x031, designer ARM = 0x41
    case kPERIPH_ID0: *value = 0x31; return;  // Part number [7:0]
    case kPERIPH_ID1: *value = 0x10; return;  // Part number [11:8] | Designer [3:0]
    case kPERIPH_ID2: *value = 0x04; return;  // Revision | Designer [7:4]
    case kPERIPH_ID3: *value = 0x00; return;

    // PrimeCell identification
    case kCELL_ID0: *value = 0x0D; return;
    case kCELL_ID1: *value = 0xF0; return;
    case kCELL_ID2: *value = 0x05; return;
    case kCELL_ID3: *value = 0xB1; return;

    default:
        *value = 0;
        return;
    }
}

void Pl031Rtc::MmioWrite(uint64_t offset, uint8_t /*size*/, uint64_t value) {
    switch (offset) {
    case kRTCMR:
        match_reg_ = static_cast<uint32_t>(value);
        return;
    case kRTCLR:
        load_reg_ = static_cast<uint32_t>(value);
        tick_offset_ = static_cast<int64_t>(load_reg_) - static_cast<int64_t>(HostEpoch());
        return;
    case kRTCCR:
        cr_ = static_cast<uint32_t>(value) & 1;
        return;
    case kRTCIMSC:
        imsc_ = static_cast<uint32_t>(value) & 1;
        UpdateIrq();
        return;
    case kRTCICR:
        ris_ &= ~static_cast<uint32_t>(value);
        UpdateIrq();
        return;
    default:
        return;
    }
}

void Pl031Rtc::UpdateIrq() {
    if (irq_level_callback_) {
        irq_level_callback_((ris_ & imsc_) != 0);
    }
}
