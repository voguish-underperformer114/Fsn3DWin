#include "app/App.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
void printUsage()
{
    std::cout
        << "Fsn3DWin options:\n"
        << "  --root <path>       Initial root path for the scan panel\n"
        << "  --max-depth <N>     Initial maximum scan depth\n"
        << "  --max-nodes <N>     Initial maximum node count\n"
        << "  --auto-scan         Start a read-only scan immediately\n"
        << "  --select-first-image Select first image after scan for preview demos\n"
        << "  --screenshot-after <seconds> Save a screenshot after startup\n"
        << "  --quit-after <seconds> Close the app after startup\n"
        << "  --help              Show this help\n";
}

std::optional<std::string> readOptionValue(int& index, int argc, char** argv, const std::string& option)
{
    if (index + 1 >= argc) {
        std::cerr << "Missing value for " << option << '\n';
        return std::nullopt;
    }

    ++index;
    return std::string(argv[index]);
}

AppOptions parseOptions(int argc, char** argv)
{
    AppOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        }

        if (arg == "--root") {
            const std::optional<std::string> value = readOptionValue(index, argc, argv, arg);
            if (!value.has_value()) {
                throw std::runtime_error("Invalid --root argument");
            }
            options.rootPath = std::filesystem::path(*value);
        } else if (arg == "--max-depth") {
            const std::optional<std::string> value = readOptionValue(index, argc, argv, arg);
            if (!value.has_value()) {
                throw std::runtime_error("Invalid --max-depth argument");
            }
            options.maxDepth = static_cast<std::uint32_t>(std::stoul(*value));
        } else if (arg == "--max-nodes") {
            const std::optional<std::string> value = readOptionValue(index, argc, argv, arg);
            if (!value.has_value()) {
                throw std::runtime_error("Invalid --max-nodes argument");
            }
            options.maxNodes = static_cast<std::size_t>(std::stoull(*value));
        } else if (arg == "--auto-scan") {
            options.autoScan = true;
        } else if (arg == "--select-first-image") {
            options.selectFirstImage = true;
        } else if (arg == "--screenshot-after") {
            const std::optional<std::string> value = readOptionValue(index, argc, argv, arg);
            if (!value.has_value()) {
                throw std::runtime_error("Invalid --screenshot-after argument");
            }
            options.screenshotAfterSeconds = std::stod(*value);
        } else if (arg == "--quit-after") {
            const std::optional<std::string> value = readOptionValue(index, argc, argv, arg);
            if (!value.has_value()) {
                throw std::runtime_error("Invalid --quit-after argument");
            }
            options.quitAfterSeconds = std::stod(*value);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    return options;
}
}

int main(int argc, char** argv)
{
    try {
        App app(parseOptions(argc, argv));
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "Fsn3DWin failed: " << error.what() << '\n';
        return 1;
    }
}
