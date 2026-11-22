#include "emulator.hpp"


namespace emulator
{
    void Emulator::readFile(const std::filesystem::path& p_filepath)
    {

    }

    const PPU::Frame_t& Emulator::renderFrame()
    {
        return m_ppu.getFrame();
    }
}