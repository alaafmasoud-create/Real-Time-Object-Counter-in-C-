#include "tracker.hpp"

#include <algorithm>
#include <set>
#include <vector>

CentroidTracker::CentroidTracker(int maxDisappeared, float maxDistance, int maxTraceLength)
    : nextObjectId_(0),
      maxDisappeared_(maxDisappeared),
      maxDistance_(maxDistance),
      maxTraceLength_(maxTraceLength) {}

std::map<int, Track>& CentroidTracker::getTracks() {
    return tracks_;
}

const std::map<int, Track>& CentroidTracker::getTracks() const {
    return tracks_;
}

cv::Point2f CentroidTracker::computeCentroid(const cv::Rect& bbox) {
    return cv::Point2f(
        static_cast<float>(bbox.x + bbox.width / 2.0),
        static_cast<float>(bbox.y + bbox.height / 2.0)
    );
}

void CentroidTracker::pushTrailPoint(Track& track, const cv::Point2f& point) const {
    track.trail.push_back(point);
    while (static_cast<int>(track.trail.size()) > maxTraceLength_) {
        track.trail.pop_front();
    }
}

void CentroidTracker::registerTrack(const cv::Rect& bbox, const cv::Point2f& centroid) {
    Track track;
    track.id = nextObjectId_++;
    track.bbox = bbox;
    track.centroid = centroid;
    track.previousCentroid = centroid;
    track.velocity = cv::Point2f(0.0F, 0.0F);
    track.predictedCentroid = centroid;
    track.disappeared = 0;
    track.age = 1;
    track.visibleFrames = 1;
    pushTrailPoint(track, centroid);

    tracks_[track.id] = track;
}

void CentroidTracker::deregisterTrack(int objectId) {
    tracks_.erase(objectId);
}

void CentroidTracker::update(const std::vector<cv::Rect>& detections) {
    if (detections.empty()) {
        std::vector<int> toRemove;

        for (auto& [id, track] : tracks_) {
            track.previousCentroid = track.centroid;
            track.predictedCentroid = track.centroid + track.velocity;
            track.disappeared++;
            track.age++;

            if (track.disappeared > maxDisappeared_) {
                toRemove.push_back(id);
            }
        }

        for (const int id : toRemove) {
            deregisterTrack(id);
        }
        return;
    }

    std::vector<cv::Point2f> inputCentroids;
    inputCentroids.reserve(detections.size());
    for (const auto& detection : detections) {
        inputCentroids.push_back(computeCentroid(detection));
    }

    if (tracks_.empty()) {
        for (std::size_t i = 0; i < detections.size(); ++i) {
            registerTrack(detections[i], inputCentroids[i]);
        }
        return;
    }

    std::vector<int> trackIds;
    trackIds.reserve(tracks_.size());
    for (const auto& [id, _] : tracks_) {
        trackIds.push_back(id);
    }

    struct CandidateMatch {
        int trackIndex = -1;
        int detectionIndex = -1;
        float cost = 0.0F;
    };

    std::vector<CandidateMatch> candidates;
    candidates.reserve(trackIds.size() * detections.size());

    for (int trackIndex = 0; trackIndex < static_cast<int>(trackIds.size()); ++trackIndex) {
        const Track& track = tracks_.at(trackIds[trackIndex]);
        const cv::Point2f predicted = track.centroid + track.velocity;
        const float previousArea = static_cast<float>(track.bbox.area());

        for (int detectionIndex = 0; detectionIndex < static_cast<int>(detections.size()); ++detectionIndex) {
            const float distance = cv::norm(predicted - inputCentroids[detectionIndex]);

            const float currentArea = static_cast<float>(detections[detectionIndex].area());
            float areaPenalty = 0.0F;
            if (previousArea > 1.0F) {
                areaPenalty = std::abs(currentArea - previousArea) / previousArea;
            }

            const float cost = distance + (areaPenalty * 10.0F);
            candidates.push_back({trackIndex, detectionIndex, cost});
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const CandidateMatch& a, const CandidateMatch& b) {
            return a.cost < b.cost;
        }
    );

    std::set<int> usedTrackIndices;
    std::set<int> usedDetectionIndices;

    for (const auto& candidate : candidates) {
        if (usedTrackIndices.count(candidate.trackIndex) > 0 ||
            usedDetectionIndices.count(candidate.detectionIndex) > 0) {
            continue;
        }

        const int trackId = trackIds[candidate.trackIndex];
        Track& track = tracks_.at(trackId);

        const float centroidDistance = cv::norm(track.centroid - inputCentroids[candidate.detectionIndex]);
        if (centroidDistance > maxDistance_) {
            continue;
        }

        track.previousCentroid = track.centroid;
        track.centroid = inputCentroids[candidate.detectionIndex];
        track.velocity = track.centroid - track.previousCentroid;
        track.predictedCentroid = track.centroid + track.velocity;
        track.bbox = detections[candidate.detectionIndex];
        track.disappeared = 0;
        track.age++;
        track.visibleFrames++;
        pushTrailPoint(track, track.centroid);

        usedTrackIndices.insert(candidate.trackIndex);
        usedDetectionIndices.insert(candidate.detectionIndex);
    }

    std::vector<int> toRemove;
    for (int trackIndex = 0; trackIndex < static_cast<int>(trackIds.size()); ++trackIndex) {
        if (usedTrackIndices.count(trackIndex) > 0) {
            continue;
        }

        Track& track = tracks_.at(trackIds[trackIndex]);
        track.previousCentroid = track.centroid;
        track.predictedCentroid = track.centroid + track.velocity;
        track.disappeared++;
        track.age++;

        if (track.disappeared > maxDisappeared_) {
            toRemove.push_back(trackIds[trackIndex]);
        }
    }

    for (const int id : toRemove) {
        deregisterTrack(id);
    }

    for (int detectionIndex = 0; detectionIndex < static_cast<int>(detections.size()); ++detectionIndex) {
        if (usedDetectionIndices.count(detectionIndex) == 0) {
            registerTrack(detections[detectionIndex], inputCentroids[detectionIndex]);
        }
    }
}
