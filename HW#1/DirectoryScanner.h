#pragma once

#include "FileInfo.h"
#include <vector>
#include <filesystem>
#include <string>
#include <stdexcept>

class DirectoryScanner {
public:
    std::vector<FileInfo> scanDirectory(const std::filesystem::path& dirPath, bool recursive = false);
};