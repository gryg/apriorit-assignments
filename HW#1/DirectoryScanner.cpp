#include "DirectoryScanner.h"
#include <iostream>

std::vector<FileInfo> DirectoryScanner::scanDirectory(const std::filesystem::path& dirPath, bool recursive) {
    if (!std::filesystem::exists(dirPath)) {
        throw std::runtime_error("Directory does not exist: " + dirPath.string());
    }
    if (!std::filesystem::is_directory(dirPath)) {
        throw std::runtime_error("Path is not a directory: " + dirPath.string());
    }

    std::vector<FileInfo> fileInfos;
    fileInfos.reserve(100); // Pre-allocate some space for efficiency

    try {
        // Choose iterator type based on recursion flag
        if (recursive) {
            std::filesystem::recursive_directory_iterator dirIter(dirPath, std::filesystem::directory_options::skip_permission_denied);
            std::filesystem::recursive_directory_iterator endIter; // Default constructed end iterator

            for (; dirIter != endIter; ++dirIter) {
                try {
                    const auto& entry = *dirIter;
                    if (std::filesystem::is_regular_file(entry.status())) { // Check status first
                        std::uintmax_t size = std::filesystem::file_size(entry);
                        auto writeTime = std::filesystem::last_write_time(entry);
                        auto perms = entry.status().permissions();
                        bool readOnly = (perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none;

                        fileInfos.emplace_back(entry.path(), size, writeTime, readOnly);
                    }
                } catch (const std::filesystem::filesystem_error& fs_err) {
                    // Log or report error for specific file but continue scanning others
                    std::cerr << "Warning: Could not process file '" << dirIter->path().string()
                              << "'. Error: " << fs_err.what() << std::endl;
                     // Optionally skip entry depth if recursive and error occurred
                     if (recursive) dirIter.disable_recursion_pending();
                }
            }
        } else {
            std::filesystem::directory_iterator dirIter(dirPath, std::filesystem::directory_options::skip_permission_denied);
            std::filesystem::directory_iterator endIter; // Default constructed end iterator

             for (; dirIter != endIter; ++dirIter) {
                 try {
                    const auto& entry = *dirIter;
                    if (std::filesystem::is_regular_file(entry.status())) { // Check status first
                        std::uintmax_t size = std::filesystem::file_size(entry);
                        auto writeTime = std::filesystem::last_write_time(entry);
                        auto perms = entry.status().permissions();
                        bool readOnly = (perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none;

                        fileInfos.emplace_back(entry.path(), size, writeTime, readOnly);
                    }
                } catch (const std::filesystem::filesystem_error& fs_err) {
                     std::cerr << "Warning: Could not process file '" << dirIter->path().string()
                              << "'. Error: " << fs_err.what() << std::endl;
                }
             }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Filesystem error scanning directory '" + dirPath.string() + "': " + e.what());
    } catch (const std::exception& e) {
         throw std::runtime_error("Error scanning directory '" + dirPath.string() + "': " + e.what());
    }

    return fileInfos;
}