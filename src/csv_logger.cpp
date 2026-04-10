#include "csv_logger.hpp"

#include <filesystem>
#include <iomanip>

CsvLogger::~CsvLogger() {
    close();
}

bool CsvLogger::open(const std::string& outputPath) {
    const std::filesystem::path path(outputPath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    stream_.open(outputPath);
    initialized_ = stream_.is_open();

    if (initialized_) {
        writeHeader();
    }

    return initialized_;
}

void CsvLogger::writeHeader() {
    stream_
        << "frame_index,timestamp_ms,track_id,direction,centroid_x,centroid_y,"
        << "bbox_x,bbox_y,bbox_width,bbox_height,positive_count,negative_count\n";
}

void CsvLogger::logCrossing(
    int frameIndex,
    double timestampMs,
    int trackId,
    const std::string& direction,
    const cv::Point2f& centroid,
    const cv::Rect& bbox,
    int positiveCount,
    int negativeCount
) {
    if (!initialized_) {
        return;
    }

    stream_
        << frameIndex << ','
        << std::fixed << std::setprecision(2) << timestampMs << ','
        << trackId << ','
        << direction << ','
        << centroid.x << ','
        << centroid.y << ','
        << bbox.x << ','
        << bbox.y << ','
        << bbox.width << ','
        << bbox.height << ','
        << positiveCount << ','
        << negativeCount << '\n';
}

void CsvLogger::close() {
    if (stream_.is_open()) {
        stream_.close();
    }
    initialized_ = false;
}
