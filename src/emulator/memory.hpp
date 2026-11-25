#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

#include <sys/mman.h>


namespace emulator
{
    // https://gbadev.net/gbadoc/memory.html#memory
    class Memory
    {
    public:
        Memory() : m_rom(nullptr), m_romSize(0)
        {
            m_ewram.resize(0x40000);
            m_iwram.resize(0x8000);
            m_ioRam.resize(0x400);
            m_paletteRam.resize(0x400);
            m_vram.resize(0x18000);
            m_oam.resize(0x400);
        }

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
                // clear existing state
                munmap(static_cast<void*>(const_cast<std::uint8_t*>(m_rom)), m_romSize);
                std::fill(m_ewram.begin(), m_ewram.end(), 0);
                std::fill(m_iwram.begin(), m_iwram.end(), 0);
                std::fill(m_ioRam.begin(), m_ioRam.end(), 0);
                std::fill(m_paletteRam.begin(), m_paletteRam.end(), 0);
                std::fill(m_vram.begin(), m_vram.end(), 0);
                std::fill(m_oam.begin(), m_oam.end(), 0);
            }

            m_rom = p_rom;
            m_romSize = p_size;
        }

        template <typename t_width, bool t_instr = false>
        t_width read(std::uint32_t p_addr)
        {
            // accessing misaligned addresses is NOT supported
            assert(p_addr % sizeof(t_width) == 0);

            std::uint32_t region = p_addr >> 24;
            std::uint32_t offset = p_addr & 0x00FFFFFF;

            switch (region)
            {
                case 0x00:
                {
                    if constexpr (t_instr)
                    {
                        if (offset < 0x4000) [[likely]]
                        {
                            assert(false && "TODO: bios read");
                        }
                    }
                    assert(false && "TODO: return prefetched instruction -> pipeline[0]");
                }
                case 0x02:
                {
                    return *reinterpret_cast<t_width*>(m_ewram.data() + (offset & 0x0003FFFF));
                }
                case 0x03:
                {
                    return *reinterpret_cast<t_width*>(m_iwram.data() + (offset & 0x00007FFF));
                }
                case 0x04:
                {
                    // TODO: implement correct mirroring
                    return *reinterpret_cast<t_width*>(m_ioRam.data() + (offset & 0x000003FF));
                }
                case 0x05:
                {
                    return *reinterpret_cast<t_width*>(m_paletteRam.data() + (offset & 0x000003FF));
                }
                case 0x06:
                {
                    return *reinterpret_cast<t_width*>(m_vram.data() + (offset & 0x00017FFF));
                }
                case 0x07:
                {
                    return *reinterpret_cast<t_width*>(m_oam.data() + (offset & 0x000003FF));
                }
                case 0x0C:
                case 0x0A:
                case 0x08:
                {
                    if (offset < m_romSize) [[likely]]
                    {
                        return *reinterpret_cast<const t_width*>(m_rom + offset);
                    }
                    assert(false && "rom read out of bounds");
                }
                case 0x0E: // https://gbadev.net/gbadoc/memory.html#cart-ram
                {
                    assert(false && "read cart ram 0x0EXXXXXX");
                }
                case 0x0F: // https://gbadev.net/gbadoc/memory.html#cart-ram
                {
                    assert(false && "read cart ram 0x0FXXXXXX");
                }
                default:
                {
                    assert(false && "unknown read");
                }
            }
        }

        template <typename t_width>
        void write(std::uint32_t p_addr, t_width p_value)
        {
            // accessing misaligned addresses is NOT supported
            assert(p_addr % sizeof(t_width) == 0);

            std::uint32_t region = p_addr >> 24;
            std::uint32_t offset = p_addr & 0x00FFFFFF;

            switch (region)
            {
                default:
                {
                    assert(false && "unknown write");
                }
            }
        }

    private:
        // https://gbadev.net/gbadoc/memory.html#external-work-ram
        std::vector<std::uint8_t> m_ewram;

        // https://gbadev.net/gbadoc/memory.html#internal-work-ram
        std::vector<std::uint8_t> m_iwram;

        // https://gbadev.net/gbadoc/memory.html#palette-ram
        std::vector<std::uint8_t> m_paletteRam;

        // https://gbadev.net/gbadoc/memory.html#io-ram
        std::vector<std::uint8_t> m_ioRam;

        // https://gbadev.net/gbadoc/memory.html#vram
        std::vector<std::uint8_t> m_vram;

        // https://gbadev.net/gbadoc/memory.html#oam
        std::vector<std::uint8_t> m_oam;

        // https://gbadev.net/gbadoc/memory.html#game-pak-rom
        // https://gbadev.net/gbadoc/memory.html#game-pak-rom-image-1
        // https://gbadev.net/gbadoc/memory.html#game-pak-rom-image-2
        const std::uint8_t* m_rom;

        std::uintmax_t m_romSize;
    };
}