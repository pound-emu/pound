// Copyright 2025 Pound Emulator Project. All rights reserved.

#include "jit.h"

#include <rem.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

#include <vector>

using JitFunc = void (*)();

void JIT::translate_and_run(CPU& cpu)
{

    // TODO: Create REM Context
    create_rem_context(nullptr, nullptr, nullptr, nullptr, nullptr);

#ifdef WIN32
    u8* code = (u8*)VirtualAlloc(NULL, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    u8* code = (u8*)mmap(nullptr, 64, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif

    size_t offset = 0;

    // Decode mock instructions from cpu.memory
    if (cpu.memory[0] == 0x05)
    {                           // MOVZ placeholder
        code[offset++] = 0x48;  // mov rax, imm64
        code[offset++] = 0xB8;
        u64 imm = 5;
        std::memcpy(&code[offset], &imm, sizeof(imm));
        offset += 8;
    }

    if (cpu.memory[4] == 0x03)
    {                           // ADD placeholder
        code[offset++] = 0x48;  // add rax, imm32
        code[offset++] = 0x05;
        u32 addval = 3;
        std::memcpy(&code[offset], &addval, sizeof(addval));
        offset += 4;
    }

    code[offset++] = 0xC3;  // ret

    JitFunc fn = reinterpret_cast<JitFunc>(code);
    u64 result;
#if defined(__x86_64__)
    asm volatile(
        "call *%1\n"
        "mov %%rax, %0\n"
        : "=r"(result)
        : "r"(fn)

        : "%rax");
#elif defined(__aarch64__)
        asm volatile("blr %1\n"
                     "mov %0, x0\n"
                     : "=r"(result)
                     : "r"(fn)
                     : "x0");
#endif

    cpu.regs[0] = result;
}
