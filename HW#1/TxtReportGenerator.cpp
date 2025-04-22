#include "TxtReportGenerator.h"
#include "Utils.h"
#include <fstream>
#include <stdexcept>
#include <iomanip>

void TxtReportGenerator::generateReport(const std::vector<FileInfo>& fileData,
                                       const std::filesystem::path& outputPath) const {
    // Use RAII for file stream management. Destructor handles closing.
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open output file for TXT report: " + outputPath.string());
    }

    outFile << "--- Directory Report ---\n";
    outFile << "Total Files: " << fileData.size() << "\n\n";
    outFile << std::left << std::setw(60) << "File Path"
            << std::setw(15) << "Size (Bytes)"
            << std::setw(25) << "Last Modified"
            << std::setw(12) << "Read Only"
            << "\n";
    outFile << std::string(112, '-') << "\n";

    for (const auto& info : fileData) {
        try {
            outFile << std::left << std::setw(60) << info.filePath.string()
                    << std::setw(15) << info.fileSize
                    << std::setw(25) << Utils::formatFileTime(info.lastWriteTime)
                    << std::setw(12) << (info.isReadOnly ? "Yes" : "No")
                    << "\n";
        } catch (const std::exception& e) {
             outFile << std::left << std::setw(60) << info.filePath.string()
                    << " <<< Error processing this file's data: " << e.what() << " >>>\n";
        }
    }

    if (outFile.fail()) {
         throw std::runtime_error("Error occurred while writing to TXT report file: " + outputPath.string());
    }
    // No explicit close needed due to RAII
}