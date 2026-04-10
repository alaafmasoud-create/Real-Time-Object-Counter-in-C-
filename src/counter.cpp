#include "counter.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

double overlapRatio(const cv::Rect& a, const cv::Rect& b) {
    const cv::Rect intersection = a & b;
    if (intersection.area() <= 0) {
        return 0.0;
    }

    const double minArea = static_cast<double>(std::min(a.area(), b.area()));
    if (minArea <= 0.0) {
        return 0.0;
    }

    return static_cast<double>(intersection.area()) / minArea;
}

std::vector<cv::Rect> mergeOverlappingDetections(const std::vector<cv::Rect>& detections) {
    std::vector<cv::Rect> merged;
    std::vector<bool> used(detections.size(), false);

    for (std::size_t i = 0; i < detections.size(); ++i) {
        if (used[i]) {
            continue;
        }

        cv::Rect current = detections[i];
        used[i] = true;

        bool changed = true;
        while (changed) {
            changed = false;

            for (std::size_t j = 0; j < detections.size(); ++j) {
                if (used[j]) {
                    continue;
                }

                if (overlapRatio(current, detections[j]) > 0.35) {
                    current = current | detections[j];
                    used[j] = true;
                    changed = true;
                }
            }
        }

        merged.push_back(current);
    }

    return merged;
}

} // namespace

ObjectCounter::ObjectCounter(const AppConfig& config)
    : config_(config),
      tracker_(
          config.maxDisappeared,
          static_cast<float>(config.maxDistance),
          config.maxTraceLength
      ),
      subtractor_(cv::createBackgroundSubtractorMOG2(500, 16.0, true)) {
    subtractor_->setShadowValue(127);
}

bool ObjectCounter::openInput() {
    if (config_.useWebcam) {
        return capture_.open(config_.cameraIndex);
    }
    return capture_.open(config_.videoPath);
}

void ObjectCounter::initializeLinePositionIfNeeded(const cv::Mat& frame) {
    if (linePosition_ >= 0) {
        return;
    }

    if (config_.linePosition >= 0) {
        linePosition_ = config_.linePosition;
    } else {
        linePosition_ = config_.horizontalLine ? frame.rows / 2 : frame.cols / 2;
    }

    if (config_.horizontalLine) {
        linePosition_ = std::clamp(linePosition_, 0, frame.rows - 1);
    } else {
        linePosition_ = std::clamp(linePosition_, 0, frame.cols - 1);
    }
}

void ObjectCounter::initializeWriterIfNeeded(const cv::Mat& frame) {
    if (!config_.saveOutput || writerInitialized_) {
        return;
    }

    const std::filesystem::path outputPath(config_.outputPath);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    const double fps = std::max(1.0, capture_.get(cv::CAP_PROP_FPS));
    const int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');

    writerInitialized_ = writer_.open(
        config_.outputPath,
        fourcc,
        fps,
        frame.size(),
        true
    );

    if (!writerInitialized_) {
        std::cerr << "Warning: could not initialize output video writer at "
                  << config_.outputPath << '\n';
    }
}

void ObjectCounter::preprocessMask(const cv::Mat& rawMask, cv::Mat& cleanMask) const {
    cv::threshold(rawMask, cleanMask, 200, 255, cv::THRESH_BINARY);
    cv::medianBlur(cleanMask, cleanMask, 5);

    const cv::Mat openKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    const cv::Mat closeKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));

    cv::morphologyEx(cleanMask, cleanMask, cv::MORPH_OPEN, openKernel, cv::Point(-1, -1), 2);
    cv::morphologyEx(cleanMask, cleanMask, cv::MORPH_CLOSE, closeKernel, cv::Point(-1, -1), 2);
    cv::dilate(cleanMask, cleanMask, closeKernel, cv::Point(-1, -1), 1);
}

std::vector<cv::Rect> ObjectCounter::detectObjects(const cv::Mat& cleanMask) const {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(cleanMask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Rect> detections;
    detections.reserve(contours.size());

    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < config_.minContourArea) {
            continue;
        }

        cv::Rect bbox = cv::boundingRect(contour);
        if (bbox.width < 10 || bbox.height < 10) {
            continue;
        }

        detections.push_back(bbox);
    }

    return mergeOverlappingDetections(detections);
}

StableSide ObjectCounter::classifyStableSide(const cv::Point2f& point) const {
    const float value = config_.horizontalLine ? point.y : point.x;
    const float lowerBound = static_cast<float>(linePosition_ - config_.lineBandHalfThickness);
    const float upperBound = static_cast<float>(linePosition_ + config_.lineBandHalfThickness);

    if (value < lowerBound) {
        return StableSide::Negative;
    }
    if (value > upperBound) {
        return StableSide::Positive;
    }
    return StableSide::Unknown;
}

void ObjectCounter::updateCounts(double timestampMs) {
    for (auto& [id, track] : tracker_.getTracks()) {
        if (track.disappeared != 0) {
            continue;
        }
        if (track.age < config_.minTrackAge) {
            continue;
        }

        const StableSide currentStableSide = classifyStableSide(track.centroid);
        if (currentStableSide == StableSide::Unknown) {
            continue;
        }

        if (track.stableSide == StableSide::Unknown) {
            track.stableSide = currentStableSide;
            continue;
        }

        if (track.stableSide == currentStableSide) {
            continue;
        }

        if (track.stableSide == StableSide::Negative && currentStableSide == StableSide::Positive) {
            positiveCount_++;
            csvLogger_.logCrossing(
                frameIndex_,
                timestampMs,
                id,
                positiveLabel(),
                track.centroid,
                track.bbox,
                positiveCount_,
                negativeCount_
            );
        } else if (track.stableSide == StableSide::Positive && currentStableSide == StableSide::Negative) {
            negativeCount_++;
            csvLogger_.logCrossing(
                frameIndex_,
                timestampMs,
                id,
                negativeLabel(),
                track.centroid,
                track.bbox,
                positiveCount_,
                negativeCount_
            );
        }

        track.stableSide = currentStableSide;
    }
}

std::string ObjectCounter::positiveLabel() const {
    return config_.horizontalLine ? "Down" : "Right";
}

std::string ObjectCounter::negativeLabel() const {
    return config_.horizontalLine ? "Up" : "Left";
}

void ObjectCounter::drawOverlay(cv::Mat& frame) const {
    const cv::Scalar lineColor(0, 255, 255);
    const cv::Scalar bandColor(255, 180, 0);
    const cv::Scalar boxColor(0, 255, 0);
    const cv::Scalar centroidColor(0, 0, 255);
    const cv::Scalar trailColor(255, 0, 255);
    const cv::Scalar textColor(255, 255, 255);

    const int lower = std::max(
        0,
        linePosition_ - config_.lineBandHalfThickness
    );
    const int upper = config_.horizontalLine
        ? std::min(frame.rows - 1, linePosition_ + config_.lineBandHalfThickness)
        : std::min(frame.cols - 1, linePosition_ + config_.lineBandHalfThickness);

    if (config_.horizontalLine) {
        cv::line(frame, cv::Point(0, linePosition_), cv::Point(frame.cols, linePosition_), lineColor, 2);
        cv::line(frame, cv::Point(0, lower), cv::Point(frame.cols, lower), bandColor, 1);
        cv::line(frame, cv::Point(0, upper), cv::Point(frame.cols, upper), bandColor, 1);
    } else {
        cv::line(frame, cv::Point(linePosition_, 0), cv::Point(linePosition_, frame.rows), lineColor, 2);
        cv::line(frame, cv::Point(lower, 0), cv::Point(lower, frame.rows), bandColor, 1);
        cv::line(frame, cv::Point(upper, 0), cv::Point(upper, frame.rows), bandColor, 1);
    }

    for (const auto& [id, track] : tracker_.getTracks()) {
        const bool active = (track.disappeared == 0);
        const cv::Scalar activeBoxColor = active ? boxColor : cv::Scalar(120, 120, 120);

        cv::rectangle(frame, track.bbox, activeBoxColor, 2);
        cv::circle(
            frame,
            cv::Point(static_cast<int>(track.centroid.x), static_cast<int>(track.centroid.y)),
            4,
            centroidColor,
            cv::FILLED
        );

        if (config_.showTrails && track.trail.size() >= 2) {
            for (std::size_t i = 1; i < track.trail.size(); ++i) {
                cv::line(
                    frame,
                    cv::Point(static_cast<int>(track.trail[i - 1].x), static_cast<int>(track.trail[i - 1].y)),
                    cv::Point(static_cast<int>(track.trail[i].x), static_cast<int>(track.trail[i].y)),
                    trailColor,
                    1
                );
            }
        }

        std::ostringstream label;
        label << "ID " << id;
        cv::putText(
            frame,
            label.str(),
            cv::Point(track.bbox.x, std::max(20, track.bbox.y - 8)),
            cv::FONT_HERSHEY_SIMPLEX,
            0.55,
            textColor,
            2
        );
    }

    std::ostringstream info1;
    info1 << positiveLabel() << ": " << positiveCount_;
    cv::putText(frame, info1.str(), cv::Point(20, 35), cv::FONT_HERSHEY_SIMPLEX, 0.8, textColor, 2);

    std::ostringstream info2;
    info2 << negativeLabel() << ": " << negativeCount_;
    cv::putText(frame, info2.str(), cv::Point(20, 70), cv::FONT_HERSHEY_SIMPLEX, 0.8, textColor, 2);

    std::ostringstream info3;
    info3 << "Tracks: " << tracker_.getTracks().size();
    cv::putText(frame, info3.str(), cv::Point(20, 105), cv::FONT_HERSHEY_SIMPLEX, 0.7, textColor, 2);

    cv::putText(
        frame,
        "Q or Esc to quit",
        cv::Point(20, frame.rows - 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        textColor,
        2
    );
}

int ObjectCounter::run() {
    if (!openInput()) {
        std::cerr << "Error: could not open input source." << std::endl;
        return 1;
    }

    if (config_.saveCsv && !csvLogger_.open(config_.csvPath)) {
        std::cerr << "Error: could not open CSV file at " << config_.csvPath << std::endl;
        return 1;
    }

    cv::namedWindow("Object Counter", cv::WINDOW_NORMAL);
    if (config_.showMask) {
        cv::namedWindow("Foreground Mask", cv::WINDOW_NORMAL);
    }

    cv::Mat frame;
    while (capture_.read(frame)) {
        if (frame.empty()) {
            break;
        }

        ++frameIndex_;
        initializeLinePositionIfNeeded(frame);

        cv::Mat rawMask;
        subtractor_->apply(frame, rawMask);

        cv::Mat cleanMask;
        preprocessMask(rawMask, cleanMask);

        const std::vector<cv::Rect> detections = detectObjects(cleanMask);
        tracker_.update(detections);

        const double timestampMs = capture_.get(cv::CAP_PROP_POS_MSEC);
        updateCounts(timestampMs);

        cv::Mat displayFrame = frame.clone();
        drawOverlay(displayFrame);

        initializeWriterIfNeeded(displayFrame);
        if (writerInitialized_) {
            writer_.write(displayFrame);
        }

        cv::imshow("Object Counter", displayFrame);
        if (config_.showMask) {
            cv::imshow("Foreground Mask", cleanMask);
        }

        const int key = cv::waitKey(config_.useWebcam ? 1 : 25);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    capture_.release();
    if (writerInitialized_) {
        writer_.release();
    }
    csvLogger_.close();
    cv::destroyAllWindows();

    std::cout << positiveLabel() << ": " << positiveCount_ << '\n';
    std::cout << negativeLabel() << ": " << negativeCount_ << '\n';

    return 0;
}
