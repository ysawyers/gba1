#pragma once

#include <filesystem>

#include "cpu.hpp"
#include "memory.hpp"
#include "ppu.hpp"


namespace emulator
{
    class Emulator
    {
    public:
        Emulator() : m_memory(), m_cpu(m_memory), m_ppu(m_memory) {}

        void readFile(const std::filesystem::path& p_filepath);

        const PPU::Frame_t& renderFrame() noexcept;

        const PPU::Frame_t& singleStep() noexcept;

    private:
        Memory m_memory;
        CPU m_cpu;
        PPU m_ppu;
    };
}