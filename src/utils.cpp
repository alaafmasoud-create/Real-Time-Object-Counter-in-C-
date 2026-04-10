#include "utils.hpp"

#include <stdexcept>
#include <string>
#include <iostream>

int parseIntegerOrThrow(const std::string& value, const std::string& optionName) {
    try {
        std::size_t processed = 0;
        const int parsed = std::stoi(value, &processed);
        if (processed != value.size()) {
            throw std::invalid_argument("contains non-numeric characters");
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid integer for option " + optionName + ": " + value);
    }
}

void printUsage(const std::string& executableName) {
    std::cout
        << "Usage:\n"
        << "  " << executableName << " [options]\n\n"
        << "Options:\n"
        << "  --video <path>             Use a video file instead of the webcam\n"
        << "  --camera <index>           Use webcam index (default: 0)\n"
        << "  --line <position>          Set the counting line position manually\n"
        << "  --vertical                 Use a vertical counting line instead of horizontal\n"
        << "  --band <pixels>            Half-thickness of the counting band (default: 18)\n"
        << "  --min-area <value>         Minimum contour area (default: 1500)\n"
        << "  --max-distance <value>     Maximum track matching distance (default: 75)\n"
        << "  --max-disappeared <value>  Maximum missing frames before track removal (default: 20)\n"
        << "  --min-track-age <value>    Minimum age before a track may be counted (default: 5)\n"
        << "  --hide-mask                Hide the foreground mask window\n"
        << "  --no-trails                Hide track trails\n"
        << "  --output <path>            Save processed output video\n"
        << "  --csv <path>               Save crossing events to CSV\n"
        << "  --help                     Show this message\n";
}

AppConfig parseArguments(int argc, char** argv) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto requireNext = [&](const std::string& option) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument("Missing value for option " + option);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            config.showHelp = true;
        } else if (arg == "--video") {
            config.useWebcam = false;
            config.videoPath = requireNext(arg);
        } else if (arg == "--camera") {
            config.useWebcam = true;
            config.cameraIndex = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--line") {
            config.linePosition = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--vertical") {
            config.horizontalLine = false;
        } else if (arg == "--band") {
            config.lineBandHalfThickness = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--min-area") {
            config.minContourArea = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--max-distance") {
            config.maxDistance = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--max-disappeared") {
            config.maxDisappeared = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--min-track-age") {
            config.minTrackAge = parseIntegerOrThrow(requireNext(arg), arg);
        } else if (arg == "--hide-mask") {
            config.showMask = false;
        } else if (arg == "--no-trails") {
            config.showTrails = false;
        } else if (arg == "--output") {
            config.saveOutput = true;
            config.outputPath = requireNext(arg);
        } else if (arg == "--csv") {
            config.saveCsv = true;
            config.csvPath = requireNext(arg);
        } else {
            throw std::invalid_argument("Unknown option: " + arg);
        }
    }

    if (config.lineBandHalfThickness < 0) {
        throw std::invalid_argument("--band must be >= 0");
    }
    if (config.minContourArea <= 0) {
        throw std::invalid_argument("--min-area must be > 0");
    }
    if (config.maxDistance <= 0) {
        throw std::invalid_argument("--max-distance must be > 0");
    }
    if (config.maxDisappeared < 0) {
        throw std::invalid_argument("--max-disappeared must be >= 0");
    }
    if (config.minTrackAge < 0) {
        throw std::invalid_argument("--min-track-age must be >= 0");
    }

    return config;
}
