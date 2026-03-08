#include "native_file_dialog.h"

#ifdef __APPLE__

#include <cstdio>
#include <string>
#include <algorithm>

std::string native_file_dialog(
    const std::vector<std::pair<std::string, std::string>> &filetypes,
    bool save)
{
    // Build the list of extensions for AppleScript
    // e.g. {"obj", "ply", "aln"}
    std::string typeList = "{";
    for (size_t i = 0; i < filetypes.size(); ++i) {
        typeList += "\"" + filetypes[i].first + "\"";
        if (i + 1 < filetypes.size())
            typeList += ", ";
    }
    typeList += "}";

    std::string cmd;
    if (save) {
        // For save, we just get a file path from the user
        cmd = "osascript -e 'POSIX path of (choose file name)' 2>/dev/null";
    } else {
        cmd = "osascript -e 'POSIX path of (choose file of type " + typeList + ")' 2>/dev/null";
    }

    char buffer[4096];
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        result += buffer;

    int status = pclose(pipe);
    if (status != 0)
        return "";

    // Remove trailing newline
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());

    return result;
}

#endif
