#pragma once

#include "FileInfo.h"
#include <vector>
#include <string>
#include <filesystem>

// Abstract interface (Product) for all report generators.
class IReportGenerator {
public:
    // Virtual destructor is crucial for proper cleanup via base class pointers.
    virtual ~IReportGenerator() = default;

    // Generates the report content based on file data and saves it to outputPath.
    // Throws std::runtime_error on file I/O errors.
    // Made const as generating a report shouldn't modify the generator's state.
    virtual void generateReport(const std::vector<FileInfo>& fileData,
                                const std::filesystem::path& outputPath) const = 0;
};