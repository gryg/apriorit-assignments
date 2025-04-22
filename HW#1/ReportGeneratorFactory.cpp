#include "ReportGeneratorFactory.h"
#include "TxtReportGenerator.h"
#include "CsvReportGenerator.h"
#include <algorithm>
#include <cctype>

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}


std::unique_ptr<IReportGenerator> ReportGeneratorFactory::createReportGenerator(const std::string& formatType) {
    std::string lowerFormat = toLower(formatType);

    if (lowerFormat == "txt") {
        return std::make_unique<TxtReportGenerator>();
    } else if (lowerFormat == "csv") {
        return std::make_unique<CsvReportGenerator>();
    }
    else {
        throw std::invalid_argument("Unsupported report format requested: " + formatType);
    }
}