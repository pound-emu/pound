// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include "aarch64/isa.h"

class JIT
{
   public:
    void translate_and_run(CPU& cpu);
};
