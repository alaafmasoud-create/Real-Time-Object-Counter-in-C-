#pragma once

#include <string>

struct AppConfig {
    bool useWebcam = true;
    int cameraIndex = 0;
    std::string videoPath;

    bool horizontalLine = true;
    int linePosition = -1;
    int lineBandHalfThickness = 18;

    int minContourArea = 1500;
    int maxDistance = 75;
    int maxDisappeared = 20;
    int minTrackAge = 5;
    int maxTraceLength = 25;

    bool showMask = true;
    bool showTrails = true;

    bool saveOutput = false;
    std::string outputPath = "assets/output/counted_output.avi";

    bool saveCsv = false;
    std::string csvPath = "assets/output/crossings.csv";

    bool showHelp = false;
};
