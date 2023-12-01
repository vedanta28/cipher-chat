#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    // Get the current working directory
    std::string currentPath = fs::current_path();

    // Specify the subdirectory name
    std::string subdirectory = "Files";

    // Create the full path by appending the subdirectory
    std::string downloadDirectory = currentPath + "/" + subdirectory + "/";

    // Check if the directory exists
    if (!fs::exists(downloadDirectory)) {
        // Create the directory if it doesn't exist
        if (fs::create_directory(downloadDirectory)) {
            std::cout << "Directory created: " << downloadDirectory << std::endl;
        } else {
            std::cerr << "Failed to create directory: " << downloadDirectory << std::endl;
            return 1; // Return an error code
        }
    }

    // Rest of your code...

    std::cout << "Download directory: " << downloadDirectory << std::endl;

    return 0;
}
