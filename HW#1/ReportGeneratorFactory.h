#pragma once

#include "IReportGenerator.h"
#include <string>
#include <memory>
#include <stdexcept>


class ReportGeneratorFactory {
public:

    static std::unique_ptr<IReportGenerator> createReportGenerator(const std::string& formatType);
};