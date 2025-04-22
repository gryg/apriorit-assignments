#pragma once

#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream> 

namespace Utils {
    std::string formatFileTime(const std::filesystem::file_time_type& ftime);
}