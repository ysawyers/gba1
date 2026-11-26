#include "emulator.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>

#include <sys/mman.h>


namespace
{
    constexpr std::uint64_t s_maximumRomSize = 32ULL * 1024 * 1024 * 1024;
}

namespace emulator
{
    void Emulator::readFile(const std::filesystem::path& p_filepath)
    {
        if (!p_filepath.has_filename())
        {
            throw std::invalid_argument("[emulator::Emulator] invalid filename");
        }

        std::FILE* file;

        try
        {
            file = std::fopen(p_filepath.c_str(), "r");
            if (file == nullptr)
            {
                throw std::runtime_error(
                    std::format("[emulator::Emulator] unable to open {}", p_filepath.c_str())
                );
            }

            std::uintmax_t size = std::filesystem::file_size(p_filepath);
            if (size > s_maximumRomSize)
            {
                throw std::length_error("[emulator::Emulator] rom exceeded 32MB limit");
            }

            auto rom = static_cast<const std::uint8_t*>(
                mmap(
                    nullptr,
                    size,
                    PROT_READ,
                    MAP_PRIVATE,
                    fileno(file),
                    0
                )
            );

            m_memory.setRom(rom, size);
            m_cpu.reset();

            std::fclose(file);
        }
        catch (const std::exception& e)
        {
            std::fclose(file);
            throw;
        }
    }

    const PPU::Frame_t& Emulator::renderFrame() noexcept
    {
        int cycles = m_cpu.tick();

        m_cpu.tick();

        m_cpu.tick();

        // TODO: 1cpi will be enforced until components are flushed out
        assert(cycles == 1);

        return m_ppu.getFrame();
    }
}