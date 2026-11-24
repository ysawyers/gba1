#pragma once

#include <array>
#include <cassert>
#include <cstdint>

#include "memory.hpp"


namespace emulator
{
    class CPU
    {
    public:
        CPU(Memory& p_memory) : m_memory(p_memory), m_regSet(), m_pipeline() {}

        int tick() noexcept
        {
            // ...
            m_regSet[15] &= ~1;

            if (thumbEnabled())
            {
                assert(false && "TODO");
            }
            else
            {
                auto instr = fetch<std::uint32_t>();

                assert(false && "TODO2");
            }

            return 1;
        }

        void reset() noexcept
        {
            m_regSet = {};
            flushPipeline<std::uint32_t>();
        }

    private:
        template <typename t_width>
        t_width fetch() noexcept
        {
            /*
                the PC is always +8/+4 due to pipelining so PC increment should not happen
                yet; it will occur after this instruction is executed
            */
            t_width instr = m_pipeline[0];
            m_pipeline[0] = m_pipeline[1];
            m_pipeline[1] = m_memory.read<t_width>(m_regSet[15]);
            return instr;
        }

        template <typename t_width>
        void flushPipeline() noexcept
        {
            /*
                the pipeline is cleared and replaced with 2 fetched instructions to keep
                the PC at +8/+4
            */
            m_pipeline[0] = fetch<t_width>();
            m_regSet[15] += sizeof(t_width);
            m_pipeline[1] = fetch<t_width>();
            m_regSet[15] += sizeof(t_width);
        }

        bool thumbEnabled() const noexcept
        {
            return (m_regSet.m_cpsr >> 5) & 1;
        }

        // https://gbadev.net/gbadoc/cpu.html#processor-modes
        enum Mode
        {
            USR,
            SYS = 0,
            FIQ,
            SVC,
            ABT,
            IRQ,
            UND
        };

        struct RegisterFile
        {
            std::array<std::uint32_t, 16> m_regs{};
            std::uint32_t m_spsr{};
        };

        struct RegisterSet
        {
            RegisterSet() : m_banks(), m_cpsr(0x0000001F), m_bankIndex(USR)
            {
                // r0-r12 and LR is indeterminate

                // initialize SP
                m_banks[USR].m_regs[13] = 0x03007F00;
                m_banks[IRQ].m_regs[13] = 0x03007FA0;
                m_banks[SVC].m_regs[13] = 0x03007FE0;

                // initialize PC
                m_banks[USR].m_regs[15] = 0x08000000;
            }

            std::uint32_t& operator[](std::uint8_t p_reg) noexcept
            {
                return m_banks[m_bankIndex].m_regs[p_reg];
            }

            // https://mgba-emu.github.io/gbatek/#overview-11
            std::array<RegisterFile, 6> m_banks;

            // https://mgba-emu.github.io/gbatek/#current-program-status-register-cpsr
            std::uint32_t m_cpsr;

            // index associated with current mode in cpsr to access the proper bank
            Mode m_bankIndex;
        };

        Memory& m_memory;

        RegisterSet m_regSet;

        std::array<std::uint32_t, 2> m_pipeline;
    };
}