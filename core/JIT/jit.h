// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include "ARM/cpu.h"

[[deprecated("using rem instead")]]
class JIT {
public:
    void translate_and_run(CPU& cpu);
};
