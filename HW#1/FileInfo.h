#pragma once

#include <filesystem>
#include <string>
#include <chrono>


class FileInfo {
public:
    std::filesystem::path filePath;
    std::uintmax_t fileSize;
    std::filesystem::file_time_type lastWriteTime;
    bool isReadOnly; 

    FileInfo(std::filesystem::path path,
             std::uintmax_t size,
             std::filesystem::file_time_type writeTime,
             bool readOnly)
        : filePath(std::move(path)),
          fileSize(size),
          lastWriteTime(writeTime),
          isReadOnly(readOnly) {}

    FileInfo() = default;
};