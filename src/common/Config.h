// Copyright 2025 Pound Emulator Project. All rights reserved.

#pragma once

#include <filesystem>

namespace Config
{

void Load(const std::filesystem::path& path);
void Save(const std::filesystem::path& path);

int windowWidth();

int windowHeight();

bool isLogAdvanced();

std::string logType();

}  // namespace Config
