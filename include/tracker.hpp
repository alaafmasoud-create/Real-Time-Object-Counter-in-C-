#pragma once

#include <deque>
#include <map>
#include <opencv2/opencv.hpp>
#include <vector>

enum class StableSide {
    Negative = -1,
    Unknown = 0,
    Positive = 1
};

struct Track {
    int id = -1;
    cv::Rect bbox;
    cv::Point2f centroid;
    cv::Point2f previousCentroid;
    cv::Point2f velocity;
    cv::Point2f predictedCentroid;

    int disappeared = 0;
    int age = 0;
    int visibleFrames = 0;

    StableSide stableSide = StableSide::Unknown;
    std::deque<cv::Point2f> trail;
};

class CentroidTracker {
public:
    CentroidTracker(int maxDisappeared, float maxDistance, int maxTraceLength);

    void update(const std::vector<cv::Rect>& detections);

    std::map<int, Track>& getTracks();
    const std::map<int, Track>& getTracks() const;

private:
    int nextObjectId_;
    int maxDisappeared_;
    float maxDistance_;
    int maxTraceLength_;
    std::map<int, Track> tracks_;

    void registerTrack(const cv::Rect& bbox, const cv::Point2f& centroid);
    void deregisterTrack(int objectId);
    void pushTrailPoint(Track& track, const cv::Point2f& point) const;

    static cv::Point2f computeCentroid(const cv::Rect& bbox);
};
