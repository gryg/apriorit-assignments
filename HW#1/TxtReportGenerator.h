#pragma once

#include "IReportGenerator.h"

class TxtReportGenerator : public IReportGenerator {
public:
    void generateReport(const std::vector<FileInfo>& fileData,
                        const std::filesystem::path& outputPath) const override;
};