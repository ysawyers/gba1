#pragma once

#include <cstdint>

#include <sys/mman.h>


namespace emulator
{
    class Memory
    {
    public:
        Memory() : m_rom(nullptr), m_romSize(0) {};

        // deleted copy to protect mmap reference
        Memory(const Memory&) = delete;
        Memory& operator=(const Memory&) = delete;

        // TODO
        Memory(Memory&&) = delete;
        Memory& operator=(Memory&&) = delete;

        ~Memory() noexcept
        {
            if (m_rom != nullptr)
            {
                munmap(static_cast<void*>(const_cast<std::uint8_t*>(m_rom)), m_romSize);
            }
        }

        void setRom(const std::uint8_t* p_rom, std::uintmax_t p_size) noexcept
        {
            if (m_rom != nullptr)
            {
                munmap(static_cast<void*>(const_cast<std::uint8_t*>(m_rom)), m_romSize);
            }
            m_rom = p_rom;
            m_romSize = p_size;
        }

        template <typename t_width>
        t_width read(std::uint32_t p_addr)
        {
            return 0;
        }

        template <typename t_width>
        void write(std::uint32_t p_addr, t_width p_value)
        {

        }

    private:
        const std::uint8_t* m_rom;

        std::uintmax_t m_romSize;
    };
}