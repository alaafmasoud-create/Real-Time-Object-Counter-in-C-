# Real-Time Object Counter in C++

**C++ with OpenCV**, ables to detects moving objects, tracks them with stable IDs, counts **entry / exit crossings** across a configurable line, and optionally saves both the processed video and a **CSV log** of crossing events.


## Highlights

- Real-time counting from **webcam** or **video file**
- More accurate entry/exit logic using a **crossing band** instead of a single-pixel line
- Better centroid tracking with **prediction**, **velocity**, and **track history**
- Separate counts in both directions:
  - Horizontal mode: **Down / Up**
  - Vertical mode: **Right / Left**
- Optional **CSV export** with one row per crossing event
- Optional **output video** saving
- Optional **foreground mask** window
- Trail visualization for tracked objects

## Project Structure

```text
real-time-object-counter-cpp/
├── CMakeLists.txt
├── CMakePresets.json
├── LICENSE
├── README.md
├── .gitignore
├── assets/
│   ├── input/
│   │   └── put-your-video-here.txt
│   └── output/
│       └── output-files-will-be-created-here.txt
├── docs/
│   └── quick-start.txt
├── include/
│   ├── config.hpp
│   ├── counter.hpp
│   ├── csv_logger.hpp
│   ├── tracker.hpp
│   └── utils.hpp
├── screenshots/
│   └── add-your-demo-images-here.txt
└── src/
    ├── counter.cpp
    ├── csv_logger.cpp
    ├── main.cpp
    ├── tracker.cpp
    └── utils.cpp
```

## Features in This Version

### 1) More accurate entry/exit counting
Instead of counting when a centroid merely touches a line, this version uses a **band around the counting line**.  
An object is counted only when its stable position changes from one side of the band to the other.

This reduces false counts caused by:
- jitter around the line
- noisy foreground blobs
- temporary centroid fluctuations

### 2) Improved tracking
Each track stores:
- object ID
- current centroid
- previous centroid
- simple velocity estimate
- predicted position
- trail history
- age / visibility / disappeared frames

### 3) CSV logging
When `--csv` is used, the app writes:
- frame index
- timestamp in milliseconds
- track ID
- direction
- centroid position
- bounding box
- total counters at the moment of crossing

## Requirements

- **C++17**
- **CMake 3.16+**
- **OpenCV 4.x** installed on your system

## Build

### Linux / macOS

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### With CMake Presets

```bash
cmake --preset default
cmake --build --preset default
```

### Windows (Visual Studio example)

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

## Run

### Webcam
```bash
./real_time_object_counter
```

### Video file
```bash
./real_time_object_counter --video ../assets/input/demo.mp4
```

### Save processed video
```bash
./real_time_object_counter --video ../assets/input/demo.mp4 --output ../assets/output/counted_output.avi
```

### Save crossing events to CSV
```bash
./real_time_object_counter --video ../assets/input/demo.mp4 --csv ../assets/output/crossings.csv
```

### Vertical counting line
```bash
./real_time_object_counter --video ../assets/input/demo.mp4 --vertical
```

### Example with a custom line and band
```bash
./real_time_object_counter --video ../assets/input/demo.mp4 --line 320 --band 20 --min-area 1800
```

## Command-Line Options

```text
--video <path>             Use a video file instead of the webcam
--camera <index>           Use webcam index (default: 0)
--line <position>          Set the counting line position manually
--vertical                 Use a vertical counting line instead of horizontal
--band <pixels>            Half-thickness of the counting band (default: 18)
--min-area <value>         Minimum contour area to keep (default: 1500)
--max-distance <value>     Maximum track matching distance (default: 75)
--max-disappeared <value>  Maximum missing frames before track removal (default: 20)
--min-track-age <value>    Minimum age before a track may be counted (default: 5)
--hide-mask                Hide the foreground mask window
--no-trails                Hide track trails
--output <path>            Save processed output video
--csv <path>               Save crossing events to CSV
--help                     Show usage information
```

## Controls

- **Q** or **Esc** → quit

## How It Works

1. Read a frame from the camera or video
2. Apply **MOG2** background subtraction
3. Remove shadows / weak pixels with thresholding
4. Clean the mask with morphology
5. Extract contours and bounding boxes
6. Merge overlapping detections
7. Update the tracker using centroid distance + motion prediction
8. Convert centroid position into a stable side:
   - negative side
   - inside band
   - positive side
9. Count only when the stable side changes across the band
10. Draw boxes, IDs, trails, line, band, and counters

## Suggested for GitHub Improvements

To make the repository even stronger after upload:
- Add a **GIF demo**
- Add 2–3 screenshots to the `screenshots/` folder
- Add one short sample video in `assets/input/`
- Create a GitHub release with a compiled binary later

## Author

Built by Alan Masoud
