#include "Utils.h"
#include <ctime> 

namespace Utils {

std::string formatFileTime(const std::filesystem::file_time_type& ftime) {
    try {
        // Convert file_time_type to time_t
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now()
            + std::chrono::system_clock::now()
        );
        std::time_t ctime = std::chrono::system_clock::to_time_t(sctp);

        // Format time_t to string (YYYY-MM-DD HH:MM:SS)
        // Use std::localtime (beware of thread-safety issues if scaling up)
        std::tm* ltm = std::localtime(&ctime); // Potential data race if used across threads without sync
        if (!ltm) {
            return "Invalid Time";
        }

        std::stringstream ss;
        // std::put_time requires <iomanip>
        ss << std::put_time(ltm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    } catch (const std::exception& e) {
        return "Time Error";
    }
}

}