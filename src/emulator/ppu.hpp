#pragma once

#include <array>
#include <cstdint>


namespace emulator
{
    class Memory;

    class PPU
    {
    public:
        static constexpr int s_frameWidth = 240;
        static constexpr int s_frameHeight = 160;

        using Frame_t = std::array<std::array<std::uint16_t, s_frameWidth>, s_frameHeight>;

        PPU(Memory& p_memory) : m_memory(p_memory), m_frame() {}

        const Frame_t& getFrame() const noexcept
        {
            return m_frame;
        }

    private:
        Memory& m_memory;

        Frame_t m_frame;
    };
}