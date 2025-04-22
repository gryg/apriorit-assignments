#pragma once

#include "IReportGenerator.h"

// Concrete implementation (ConcreteProduct) for generating CSV reports.
class CsvReportGenerator : public IReportGenerator {
public:
    void generateReport(const std::vector<FileInfo>& fileData,
                        const std::filesystem::path& outputPath) const override;

private:
    // Helper to escape strings for CSV format (handles commas and quotes)
    std::string escapeCsv(const std::string& input) const;
};