#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <print>
#include <ranges>

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
                        assert(false && "new instruction!");
                    }
                    else
                    {
                        std::println("instruction: address({:08x}) opcode({:08x})", m_regSet[15] - 8, instr);
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

        // https://mgba-emu.github.io/gbatek/#branch-and-branch-with-link-b-bl-blx_imm
        template <bool t_bl>
        int branch(std::uint32_t p_instr) noexcept
        {
            if constexpr (t_bl)
            {
                m_regSet[14] = m_regSet[15] - 4;
            }
            m_regSet[15] += (p_instr & 0xFFFFFF) * 4;

            // PC update, flush pipeline
            flushPipeline<std::uint32_t>();
            return 1;
        }

        // https://mgba-emu.github.io/gbatek/#opcode-format-5
        template <bool t_pre, bool t_up, bool t_psr, bool t_wb, bool t_load>
        int blockDataTransfer(std::uint32_t p_instr) noexcept
        {
            int rn = (p_instr >> 16) & 0xF;
            auto transferList = static_cast<std::uint16_t>(p_instr);

            assert(rn != 15 && "r15 should not be used as the base register in any LDM or STM instruction");

            /*
                when r15 is present in the transfer list the value is PC+12 so PC increment
                has already occured before the execute stage is complete
            */
            m_regSet[15] += 4;

            int numTransfers = std::popcount(transferList);
            if (numTransfers == 0)
            {
                assert(false && "empty transfer list edge case");
            }

            std::uint32_t writebackAddr;
            std::uint32_t baseAddr;

            if constexpr (t_up)
            {
                writebackAddr = m_regSet[rn] + numTransfers * 4;
                baseAddr = m_regSet[rn];
            }
            else
            {
                writebackAddr = m_regSet[rn] - numTransfers * 4;
                /*
                    the highest register in the transfer list is ALWAYS written last so when
                    decrementing transfers start from the modified value of the base register
                    and increment up
                */
                baseAddr = writebackAddr;
            }

            int reg = std::countr_zero(transferList);
            bool pcTransfer = transferList >> 15;

            if constexpr (t_load)
            {
                // LDM (load from memory)

                if constexpr (t_wb)
                {
                    /*
                        LDM will always overwrite the updated base register if it's in the transfer
                        list so the base register is updated before the load is initiated
                    */
                    m_regSet[rn] = writebackAddr;
                }

                do
                {
                    if constexpr (t_pre)
                    {
                        baseAddr += 4;
                        m_regSet[reg] = m_memory.read<std::uint32_t>(baseAddr);
                    }
                    else
                    {
                        m_regSet[reg] = m_memory.read<std::uint32_t>(baseAddr);
                        baseAddr += 4;
                    }
                    transferList &= transferList - 1;
                    reg = std::countr_zero(transferList);
                }
                while (reg != std::numeric_limits<decltype(transferList)>::digits);

                if constexpr (t_psr)
                {
                    if (pcTransfer)
                    {
                        assert((m_regSet.spsr() & 0x1F) == 0 && "LDM invalid mode change");
                        m_regSet.m_cpsr = m_regSet.spsr();
                    }
                }
            }
            else
            {
                // STM (store to memory)

                if constexpr (t_psr)
                {
                    if (pcTransfer)
                    {
                        assert(!t_wb && "base writeback should not be used with STM and r15 in the transferList (S=1)");

                        assert(false && "TODO: STM user bank transfer");
                    }
                }

                if constexpr (t_wb)
                {
                    /*
                        if the first register transferred is the same as the base register the unmodified
                        value will be written to memory -- all subsequent writes will write the modified
                        value
                    */
                    if (reg == rn)
                    {
                        if constexpr (t_pre)
                        {
                            baseAddr += 4;
                            m_memory.write<std::uint32_t>(baseAddr, m_regSet[rn]);
                        }
                        else
                        {
                            m_memory.write<std::uint32_t>(baseAddr, m_regSet[rn]);
                            baseAddr += 4;
                        }
                        transferList &= transferList - 1;
                        reg = std::countr_zero(transferList);
                    }
                    m_regSet[rn] = writebackAddr;
                }

                while (reg != std::numeric_limits<decltype(transferList)>::digits)
                {
                    if constexpr (t_pre)
                    {
                        baseAddr += 4;
                        m_memory.write<std::uint32_t>(baseAddr, m_regSet[reg]);
                    }
                    else
                    {
                        m_memory.write<std::uint32_t>(baseAddr, m_regSet[reg]);
                        baseAddr += 4;
                    }
                    transferList &= transferList - 1;
                    reg = std::countr_zero(transferList);
                }
            }

            if (pcTransfer)
            {
                if constexpr (t_load)
                {
                    flushPipeline<std::uint32_t>();
                }
                else if constexpr (t_psr)
                {
                    assert(false && "TODO: reset from user bank transfer mode");
                }
            }
            else
            {
                m_regSet[15] += 4;
            }
            return 1;
        }

    private:
        // https://gbadev.net/gbadoc/cpu.html#processor-modes
        enum class Bank
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
            RegisterSet() : m_cpsr(0x0000001F), m_banks(), m_bank(Bank::USR)
            {
                // r0-r12 and LR is indeterminate

                // initialize SP
                m_banks[std::to_underlying(Bank::USR)].m_regs[13] = 0x03007F00;
                m_banks[std::to_underlying(Bank::IRQ)].m_regs[13] = 0x03007FA0;
                m_banks[std::to_underlying(Bank::SVC)].m_regs[13] = 0x03007FE0;

                // initialize PC
                m_banks[std::to_underlying(Bank::USR)].m_regs[15] = 0x08000000;
            }

            std::uint32_t& spsr() noexcept
            {
                return m_banks[std::to_underlying(m_bank)].m_spsr;
            }

            std::uint32_t& operator[](int p_reg) noexcept
            {
                assert(p_reg >= 0 && p_reg < 16 && "register index out of bounds");
                return m_banks[std::to_underlying(m_bank)].m_regs[p_reg];
            }

        public:
            // https://mgba-emu.github.io/gbatek/#current-program-status-register-cpsr
            std::uint32_t m_cpsr;

        private:
            struct RegisterFile
            {
                std::array<std::uint32_t, 16> m_regs{};
                std::uint32_t m_spsr{};

                void logDump()
                {
                    // std::println(
                    //     "r0: {:08x} r1: {:08x} r2: {:08x}\n",

                    // );
                }
            };

            // https://mgba-emu.github.io/gbatek/#overview-11
            std::array<RegisterFile, 6> m_banks;

            // index associated with the current mode in cpsr used to access the proper bank
            Bank m_bank;
        };

        Memory& m_memory;

        RegisterSet m_regSet;

        std::array<std::uint32_t, 2> m_pipeline;

        ///////////////////////////////////////
        // === precompiled lookup tables === //
        //////////////////////////////////////

        // TODO: investigate weird warning propogation on constexpr for range-based variable
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wshadow"

        static constexpr auto m_armLUT = ([]()
        {
            std::array<int(CPU::*)(std::uint32_t), 4096> lut{};

            /*
                original instruction: 32 bits
                31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

                hash: 12 bits
                27 26 25 24 23 22 21 20 7 6 5 4
            */
            template for (constexpr std::uint16_t hash : std::ranges::views::iota(0, 4096))
            {
                switch (hash >> 9)
                {
                    case 0b100:
                    {
                        lut[hash] = &CPU::blockDataTransfer<
                            (hash >> 8) & 1,
                            (hash >> 7) & 1,
                            (hash >> 6) & 1,
                            (hash >> 5) & 1,
                            (hash >> 4) & 1
                        >;
                        break;
                    }
                    case 0b101:
                    {
                        lut[hash] = &CPU::branch<(hash >> 8) & 1>;
                        break;
                    }
                }
            }

            return lut;
        })();

        static constexpr auto m_thumbLUT = ([]()
        {
            std::array<int(CPU::*)(std::uint32_t), 1024> lut;

            template for (constexpr std::uint16_t hash : std::ranges::views::iota(0, 1024))
            {
                lut[hash] = nullptr;
            }

            return lut;
        })();

        #pragma GCC diagnostic pop

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
                lut[0b1010][flags] = n == v;
                lut[0b1011][flags] = n ^ v;
                lut[0b1100][flags] = !z & (n == v);
                lut[0b1101][flags] = z | (n ^ v);
                lut[0b1110][flags] = true;

                // never (ARMv1,v2 only) (Reserved ARMv3 and up)
                lut[0b1111][flags] = false;
            }

            return lut;
        })();
    };
}