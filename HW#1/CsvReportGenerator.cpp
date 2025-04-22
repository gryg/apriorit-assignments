#include "CsvReportGenerator.h"
#include "Utils.h" 
#include <fstream>
#include <stdexcept>
#include <sstream> 

// Helper to escape strings for CSV format
std::string CsvReportGenerator::escapeCsv(const std::string& input) const {
    // Basic CSV escaping: If contains comma, newline, or double quote, enclose in double quotes.
    // Double up existing double quotes within the string.
    if (input.find_first_of(",\"\n") == std::string::npos) {
        return input; 
    }

    std::stringstream ss;
    ss << '"'; 
    for (char c : input) {
        if (c == '"') {
            ss << "\"\""; 
        } else {
            ss << c;
        }
    }
    ss << '"'; 
    return ss.str();
}


void CsvReportGenerator::generateReport(const std::vector<FileInfo>& fileData,
                                       const std::filesystem::path& outputPath) const {
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open output file for CSV report: " + outputPath.string());
    }

    // --- Write Header ---
    outFile << "FilePath,FileSize,LastWriteTime,IsReadOnly\n";

    // --- Write Data ---
    for (const auto& info : fileData) {
         try {
            outFile << escapeCsv(info.filePath.string()) << ","
                    << info.fileSize << ","
                    << escapeCsv(Utils::formatFileTime(info.lastWriteTime)) << ","
                    << (info.isReadOnly ? "true" : "false") // Use true/false for CSV boolean
                    << "\n";
         } catch (const std::exception& e) {
              outFile << escapeCsv(info.filePath.string())
                    << ",<<< Error processing this file's data: " << escapeCsv(e.what()) << " >>>\n";
         }
    }

     if (outFile.fail()) {
         throw std::runtime_error("Error occurred while writing to CSV report file: " + outputPath.string());
    }
    // No explicit close needed due to RAII
}