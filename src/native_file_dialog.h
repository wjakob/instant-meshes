#pragma once

#include <string>
#include <vector>
#include <utility>

#ifdef __APPLE__
// Native macOS file dialog using NSOpenPanel/NSSavePanel.
// Replaces NanoGUI's file_dialog which is broken on modern macOS (13+).
std::string native_file_dialog(
    const std::vector<std::pair<std::string, std::string>> &filetypes,
    bool save);
#endif
