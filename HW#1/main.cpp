#include "DirectoryScanner.h"
#include "ReportGeneratorFactory.h"
#include "IReportGenerator.h" 
#include <iostream>
#include <string>
#include <vector>
#include <memory>     
#include <stdexcept>  
#include <cstdlib>    
#include <filesystem> 

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) { 
        std::cerr << "Usage: " << argv[0] << " <directory_path> <format (txt|csv)> [-r for recursive]\n";
        return EXIT_FAILURE;
    }

    std::filesystem::path directoryPath = argv[1];
    std::string format = argv[2];
    bool recursive = false;
    if (argc == 4 && std::string(argv[3]) == "-r") {
        recursive = true;
    }

    std::filesystem::path outputPath = directoryPath;
    if (std::filesystem::is_directory(directoryPath)) {
         outputPath = directoryPath / (directoryPath.filename().string() + "_report." + format);
    } else {
         std::string filename_str = directoryPath.filename().string();
         if (filename_str.empty() || filename_str == ".") { 
             outputPath = directoryPath.parent_path() / ("report." + format);
         } else {
            outputPath = directoryPath.parent_path() / (filename_str + "_report." + format);
         }
    }

    try {
        DirectoryScanner scanner;
        std::cout << "Scanning directory '" << directoryPath.string() << "' "
                  << (recursive ? "(recursively)..." : "...") << std::endl;
        std::vector<FileInfo> fileData = scanner.scanDirectory(directoryPath, recursive);
        std::cout << "Found " << fileData.size() << " files." << std::endl;


        bool dirExists = std::filesystem::exists(directoryPath) && std::filesystem::is_directory(directoryPath);

        if (fileData.empty()) {
             if (!dirExists) {
                 std::cerr << "Warning: Input directory does not exist. Report will be empty." << std::endl;
             } else {
                 std::cout << "Directory exists but contains no files matching criteria. Report will be empty." << std::endl;
             }
        }

        std::unique_ptr<IReportGenerator> reportGenerator = ReportGeneratorFactory::createReportGenerator(format);
        std::cout << "Generating " << format << " report to '" << outputPath.string() << "'..." << std::endl;

        reportGenerator->generateReport(fileData, outputPath); 

        std::cout << "Report generated successfully!" << std::endl;

    } catch (const std::invalid_argument& e) { 
        std::cerr << "Error: Invalid argument provided. " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::filesystem::filesystem_error& e) { 
         std::cerr << "Filesystem Error: " << e.what() << "\n"
                   << "Path1: " << e.path1().string() << "\n"
                   << "Path2: " << e.path2().string() << std::endl;
         return EXIT_FAILURE;
    } catch (const std::runtime_error& e) { 
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) { 
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) { 
        std::cerr << "An unknown error occurred." << std::endl;
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}