#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <print>

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
                assert(false && "TODO: handle thumb instructions");
            }
            else
            {
                auto instr = fetch<std::uint32_t>();

                std::uint32_t cond = instr >> 28;
                std::uint32_t flags = m_regSet.m_cpsr >> 28;

                if (m_condLUT[cond][flags])
                {
                    std::uint32_t hash = ((instr >> 16) & 0xFF0) | ((instr >> 4) & 0x00F);

                    auto opcodeHandler = m_armLUT[hash];
                    if (opcodeHandler == nullptr)
                    {
                        std::println("unhandled instruction: address({:08x}) opcode({:08x})", m_regSet[15] - 8, instr);
                        assert(false);
                    }
                    else
                    {
                        return (this->*opcodeHandler)(instr);
                    }
                }
                else
                {
                    m_regSet[15] += 4;
                }
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
            m_pipeline[1] = m_memory.read<t_width, true>(m_regSet[15]);
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

        int bx(std::uint32_t p_instr) noexcept
        {
            return 1;
        }

    private:
        // https://gbadev.net/gbadoc/cpu.html#processor-modes
        enum Mode
        {
            USR, // SYS
            FIQ,
            SVC,
            ABT,
            IRQ,
            UND
        };

        class RegisterSet
        {
        public:
            RegisterSet() : m_cpsr(0x0000001F), m_banks(), m_bankIndex(USR)
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

        public:
            // https://mgba-emu.github.io/gbatek/#current-program-status-register-cpsr
            std::uint32_t m_cpsr;

        private:
            struct RegisterFile
            {
                std::array<std::uint32_t, 16> m_regs{};
                std::uint32_t m_spsr{};
            };

            // https://mgba-emu.github.io/gbatek/#overview-11
            std::array<RegisterFile, 6> m_banks;

            // index associated with the current mode in cpsr used to access the proper bank
            Mode m_bankIndex;
        };

        Memory& m_memory;

        RegisterSet m_regSet;

        std::array<std::uint32_t, 2> m_pipeline;

        ///////////////////////////////////////
        // === precompiled lookup tables === //
        //////////////////////////////////////

        static constexpr auto m_armLUT = ([]()
        {
            std::array<int(CPU::*)(std::uint32_t), 4096> lut{};

            return lut;
        })();

        static constexpr auto m_thumbLUT = ([]()
        {
            std::array<int(CPU::*)(std::uint32_t), 1024> lut{};

            return lut;
        })();

        static constexpr auto m_condLUT = ([]()
        {
            std::array<std::array<bool, 16>, 16> lut;

            for (std::uint8_t flags = 0; flags < 16; flags++)
            {
                bool v = flags & 1;
                bool c = (flags >> 1) & 1;
                bool z = (flags >> 2) & 1;
                bool n = (flags >> 3) & 1;

                lut[0b0000][flags] = z;
                lut[0b0001][flags] = !z;
                lut[0b0010][flags] = c;
                lut[0b0011][flags] = !c;
                lut[0b0100][flags] = n;
                lut[0b0101][flags] = !n;
                lut[0b0110][flags] = v;
                lut[0b0111][flags] = !v;
                lut[0b1000][flags] = c & !z;
                lut[0b1001][flags] = !c | z;
                lut[0b1010][flags] = n & v;
                lut[0b1011][flags] = n ^ v;
                lut[0b1100][flags] = !z && (n & v);
                lut[0b1101][flags] = z | (n ^ v);
                lut[0b1110][flags] = true;

                // never (ARMv1,v2 only) (Reserved ARMv3 and up)
                lut[0b1111][flags] = false;
            }

            return lut;
        })();
    };
}