#pragma once

#include <fstream>
#include <opencv2/opencv.hpp>
#include <string>

class CsvLogger {
public:
    CsvLogger() = default;
    ~CsvLogger();

    bool open(const std::string& outputPath);
    void logCrossing(
        int frameIndex,
        double timestampMs,
        int trackId,
        const std::string& direction,
        const cv::Point2f& centroid,
        const cv::Rect& bbox,
        int positiveCount,
        int negativeCount
    );
    void close();

private:
    std::ofstream stream_;
    bool initialized_ = false;

    void writeHeader();
};
