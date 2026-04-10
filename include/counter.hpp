#pragma once

#include "config.hpp"
#include "csv_logger.hpp"
#include "tracker.hpp"

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class ObjectCounter {
public:
    explicit ObjectCounter(const AppConfig& config);
    int run();

private:
    AppConfig config_;
    CentroidTracker tracker_;
    cv::Ptr<cv::BackgroundSubtractor> subtractor_;
    CsvLogger csvLogger_;

    int positiveCount_ = 0;
    int negativeCount_ = 0;
    int linePosition_ = -1;
    bool writerInitialized_ = false;
    int frameIndex_ = 0;

    cv::VideoCapture capture_;
    cv::VideoWriter writer_;

    bool openInput();
    void initializeLinePositionIfNeeded(const cv::Mat& frame);
    void initializeWriterIfNeeded(const cv::Mat& frame);

    void preprocessMask(const cv::Mat& rawMask, cv::Mat& cleanMask) const;
    std::vector<cv::Rect> detectObjects(const cv::Mat& cleanMask) const;
    void updateCounts(double timestampMs);
    void drawOverlay(cv::Mat& frame) const;

    StableSide classifyStableSide(const cv::Point2f& point) const;
    std::string positiveLabel() const;
    std::string negativeLabel() const;
};
