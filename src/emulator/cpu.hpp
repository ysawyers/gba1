#pragma once


namespace emulator
{
    class Memory;

    class CPU
    {
    public:
        CPU(Memory& p_memory) : m_memory(p_memory) {}

    private:
        Memory& m_memory;
    };
}