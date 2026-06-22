# Radar-Based Motorcycle Detection with Image Processing

A low-cost, radar-triggered sidewalk motorcycle detection prototype designed to reduce the cost of continuous video processing in urban surveillance systems.

## Project Overview

This project was developed as a graduation thesis at Çukurova University, Department of Computer Engineering.

The main goal of the system is to detect suspicious motorcycle movement on sidewalks without continuously processing video streams. Instead of running object detection on city cameras 24/7, the proposed architecture uses a low-cost Doppler radar module as a first-stage trigger. When the radar detects fast motorcycle-like movement, an ESP32-S3 Sense camera captures a short image burst and sends it to a server. The server then performs YOLO11s-based motorcycle detection, logs the event, and displays the results on a dashboard.

The prototype demonstrates an event-triggered radar-camera validation workflow. In the updated implementation, the visual validation model was trained with real motorcycle images. Therefore, the system performs object detection directly on the motorcycle target class instead of using a proxy object.

## Demo Videos

* [Video 1 – Radar signal and system demonstration](https://youtu.be/Pqjs36R-g7s)
* [Video 2 – Radar signal and system demonstration 2](https://youtu.be/cXx1Mhaml8s)
* [Video 3 - Speed and detection test-explanation with HB100](https://youtu.be/M53GATuA4gE)

## Main Features

* Radar-triggered event detection instead of continuous video processing
* HB100 Doppler radar-based motion sensing
* MCP6002-based analog signal conditioning circuit
* STM32F411RE-based ADC sampling, DMA buffering, and FFT analysis
* UART-based trigger communication between STM32 and ESP32-S3
* ESP32-S3 Sense camera image burst capture
* Wi-Fi image transmission to a Python server
* Server-side YOLO11s motorcycle detection
* Event logging with confidence scores, best frame selection, and analysis time
* Streamlit dashboard for event inspection and result visualization
* Low-cost and scalable prototype architecture for smart city applications

## System Architecture

The general workflow of the system is:

```text
HB100 Doppler Radar
        ↓
MCP6002 Signal Conditioning Circuit
        ↓
STM32F411RE ADC + DMA + FFT Analysis
        ↓
UART Trigger Message
        ↓
ESP32-S3 Sense Camera
        ↓
Wi-Fi Image Transmission
        ↓
Python Server
        ↓
YOLO11s Motorcycle Detection
        ↓
Event Logging + Streamlit Dashboard
```

## Hardware Components

| Component                  | Function                                                                           |
| -------------------------- | ---------------------------------------------------------------------------------- |
| HB100 Doppler Radar Module | Detects moving objects using Doppler frequency shift                               |
| MCP6002 Op-Amp Circuit     | Amplifies and biases the low-amplitude radar IF signal                             |
| STM32F411RE Nucleo Board   | Performs ADC sampling, DMA buffering, FFT analysis, and first-stage decision logic |
| ESP32-S3 Sense Board       | Captures images after receiving a trigger                                          |
| Camera Module              | Provides visual data for server-side validation                                    |
| Wi-Fi Network              | Transfers captured images and event data to the server                             |
| Server Computer            | Runs YOLO11s inference, stores logs, and serves dashboard output                    |

## Software Components

### STM32 Firmware

The STM32F411RE firmware is responsible for radar signal acquisition and first-stage motion analysis.

Main tasks:

* Reads the amplified HB100 radar IF signal through ADC
* Uses timer-triggered ADC sampling
* Transfers ADC data using DMA
* Applies mean subtraction to remove DC offset
* Performs 256-point FFT using CMSIS-DSP
* Extracts dominant Doppler frequency and peak signal power
* Classifies movement as pedestrian-like or motorcycle-like
* Sends UART trigger messages to the ESP32-S3 Sense board

Example trigger messages:

```text
MOTOR
YAYA
```

### ESP32-S3 Sense Firmware

The ESP32-S3 Sense board acts as the second-stage image acquisition unit.

Main tasks:

* Waits for a trigger message from STM32
* Captures an image burst after the trigger
* Uses VGA JPEG image configuration
* Stores frame buffers in PSRAM
* Sends captured frames to the server over Wi-Fi

Prototype capture configuration:

```text
Resolution: 640 × 480
Format: JPEG
Maximum frames: 60
Capture duration: ~3 seconds
Frame interval: ~45 ms
```

### Python Server

The Python server receives image bursts from the ESP32 and performs visual motorcycle detection.

Main tasks:

* Receives image frames through network communication
* Saves each event as a separate image burst
* Runs YOLO11s inference on received frames
* Detects motorcycle objects in the captured images
* Records frame-level and event-level detection results
* Selects the best frame based on confidence score
* Stores structured logs for dashboard visualization

### Streamlit Dashboard

The dashboard converts detection outputs into an interpretable monitoring interface.

Main features:

* Event list display
* Event filtering
* Best frame preview
* YOLO confidence score visualization
* Final event decision display
* Downloadable logs and result files
* Inspection of supporting evidence for each detected event

## YOLO11s Motorcycle Detection

The server-side validation stage uses YOLO11s for object detection.

In the updated prototype, the model was trained with real motorcycle images. The purpose of this stage is to determine whether the radar-triggered image burst contains a motorcycle. Each received frame is analyzed separately, and the final event decision is made using multi-frame evidence.

The YOLO11s validation stage provides:

* Motorcycle object detection
* Frame-level confidence scores
* Positive and negative frame counting
* Best frame selection
* Event-level decision generation
* Structured TXT, JSON, and CSV output files
* Dashboard-ready detection results

The system does not rely on a single frame only. Instead, all frames in the event burst are evaluated together. This improves the reliability of the final decision because the event is validated using both the number of positive frames and the highest detection confidence.

## Why Radar-Triggered Detection?

A traditional camera-only system must process video streams continuously, even when no violation occurs. This creates high CPU/GPU usage, energy consumption, network traffic, and server cost.

This project proposes a more efficient approach:

| Continuous Camera Processing   | Radar-Triggered System                   |
| ------------------------------ | ---------------------------------------- |
| Processes video 24/7           | Processes only suspicious events         |
| High CPU/GPU cost              | Lower processing cost                    |
| High network load              | Event-based data transmission            |
| Camera-only detection          | Radar + camera + server validation       |
| Less scalable for many cameras | More scalable for selected problem areas |

The radar unit acts as a low-cost pre-detection layer. The camera and YOLO model are activated only when the radar detects suspicious fast movement. This reduces unnecessary image processing and allows the system to focus on relevant time intervals.

## Experimental Results

The prototype confirmed that the system can perform the complete event-triggered workflow:

* Radar signal is captured and analyzed on STM32
* FFT-based dominant frequency extraction is performed
* Trigger messages activate ESP32 image capture
* ESP32 sends captured image bursts to the server
* YOLO11s processes incoming frames
* Motorcycle detections are generated from real motorcycle images
* Event-level outputs are generated
* Dashboard displays detections, best frames, confidence values, and logs

In the prototype tests, 35-frame and 60-frame image bursts were evaluated. The observed analysis time was approximately:

```text
35-frame events: ~5.8–7.1 seconds
60-frame events: ~9.5–9.6 seconds
```

Since the system is designed for event-based validation instead of continuous video processing, this processing time is acceptable for the prototype stage.

## Limitations

The current prototype has several limitations:

* The dataset size is limited compared to large-scale real-world surveillance datasets.
* The HB100 radar has a practical detection range of approximately 15 meters in the prototype.
* ESP32-S3 Sense camera quality is limited compared to professional surveillance cameras.
* Detection performance may decrease under poor lighting, motion blur, distance, or partial visibility.
* Radar thresholds require calibration for different environments.
* Radar alone cannot reliably distinguish motorcycles from all other fast-moving objects.
* Real-world deployment requires a larger dataset, outdoor testing, and improved model validation.
* The system is a prototype and is not yet a production-ready traffic violation detection system.

## Future Work

Planned improvements include:

* Expanding the motorcycle image dataset with more sidewalk and outdoor scenarios
* Fine-tuning YOLO11s or another YOLO model with a larger and more diverse motorcycle dataset
* Testing the system in real outdoor sidewalk environments
* Improving radar threshold calibration
* Using a longer-range or more advanced radar module
* Improving camera quality for low-light and long-distance detection
* Adding street camera integration for event-based video interval extraction
* Deploying multiple devices in selected high-risk urban locations
* Adding cloud-based monitoring and municipality-level integration
* Improving robustness against lighting changes, motion blur, and partial occlusion

## Technologies Used

* STM32F411RE
* HB100 Doppler Radar
* FFT Signal Processing
* MCP6002 Operational Amplifier
* ESP32-S3 Sense
* UART Communication
* ADC + DMA
* CMSIS-DSP
* PlatformIO
* Arduino Framework
* Python
* YOLO11s
* Streamlit

## Academic Context

This project was developed as a graduation thesis:

**Title:**  
A Radar-Triggered Sidewalk Motorcycle Detection System for Reducing Continuous Video Processing Costs

**Advisors:**
- Arş. Gör. Üyesi BARIŞ ATA
- Prof. Dr. Mustafa GÖK(Department of Electrical and Electronics Engineering)

**Author:**  
Mehmet Dora

**University:**  
Çukurova University  
Faculty of Engineering  
Department of Computer Engineering

**Year:**  
2026

## Disclaimer

This repository contains a prototype implementation. The current system demonstrates radar-triggered image acquisition, server-side YOLO11s motorcycle detection, event logging, and dashboard visualization. It is not yet a production-ready motorcycle violation detection system.

A real-world deployment requires:

* Larger and more diverse motorcycle datasets
* Additional model training and validation
* Outdoor validation under different environmental conditions
* Environmental robustness testing
* Legal and privacy compliance checks
* Integration with official city camera infrastructure

## License

This project is intended for academic and research purposes. Add a suitable license before public reuse or distribution.
