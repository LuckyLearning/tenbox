#pragma once

#include <cstdint>

enum class VCpuExitAction {
    kContinue,
    kHalt,
    kShutdown,
    kError,
};

class HypervisorVCpu {
public:
    virtual ~HypervisorVCpu() = default;
    virtual VCpuExitAction RunOnce() = 0;
    virtual void CancelRun() = 0;
    virtual uint32_t Index() const = 0;

    // Set up x86 32-bit protected mode boot registers in guest RAM.
    // Called once on vCPU 0 after the kernel is loaded.
    virtual bool SetupBootRegisters(uint8_t* ram) = 0;
};
