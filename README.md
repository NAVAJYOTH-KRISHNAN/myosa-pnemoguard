---
publishDate: 2026-08-24
title: PneumoGuard - AI-Assisted COPD Exacerbation Early Warning System
excerpt: A sub-₹500 wearable that senses the barometric, humidity, and motion triggers behind COPD exacerbations and warns patients hours before an acute episode.
image: PneumoGuard/pneumoguard-cover.jpg
tags:
- HealthTech
- IoT
- MYOSA-Mini
- COPD
- WearableSensors
---

> Sensing the weather before your lungs feel it — an early-warning wearable for COPD patients.

---

## Acknowledgements

We thank our faculty mentor, **Dr. Nishanth N**, Department of Electronics and Communication Engineering, TKM College of Engineering, for his guidance throughout the ideation of PneumoGuard. We also acknowledge **IEEE MYOSA 6.0** and the MYOSA Mini Kit ecosystem for providing an integrated sensor platform that let us prototype a clinically-motivated idea without first having to source and bring up individual sensor breakout boards.

**Team Members:**

->Ribin KV

->Nivedita Ranjish

->Navajyoth Krishnan D


---
<p align="center">
<img src="assets/cover.png" width="800"><br/>
<i>PneumoGuard</i>
</p>

## Overview

**PneumoGuard** is a low-cost, chest-worn wearable that gives Chronic Obstructive Pulmonary Disease (COPD) patients advance warning of respiratory exacerbations. It works by continuously sensing the environmental and physiological signals that clinical literature associates with the onset of acute episodes — sudden barometric pressure drops, humidity spikes, and laboured-breathing motion — and fusing them into a single, easy-to-read risk level.

**Who it is for:** COPD patients, particularly in low-resource and rural settings (such as rural Kerala) where specialist respiratory care and clinical-grade monitoring equipment are not readily accessible.

**What problem it solves:** COPD affects 380 million people worldwide and causes roughly 3.2 million deaths annually. Over 80% of acute exacerbations are preceded by detectable environmental shifts, yet no affordable home device currently flags these triggers before symptoms escalate — existing clinical monitoring equipment can cost upwards of ₹50,000 and requires a hospital visit. PneumoGuard closes this gap with a sub-₹500 device that gives patients hours of actionable lead time to take a preventive inhaler dose or contact a physician.

**How it works, at a glance:**
1. Onboard sensors sample pressure, temperature, motion, and ambient light continuously.
2. A 30-minute sliding-window analysis extracts three clinically relevant features: pressure-drop rate, humidity-spike status, and a breathing-effort index derived from accelerometer variance.
3. These features are fused into a composite **LOW / MEDIUM / HIGH** risk score, context-adjusted for indoor vs. outdoor conditions.
4. The risk level is displayed locally on an OLED screen and, for MEDIUM/HIGH risk, announced with a buzzer alert — while every reading is also logged to a cloud dashboard for caregiver or telemedicine review.

**Key features:**
* On-device, real-time multi-factor risk scoring (no cloud round-trip needed for an alert)
* Environmental-context awareness (indoor/outdoor) that adapts alert thresholds
* Dual-channel alerting: local OLED + buzzer for the patient, cloud dashboard for caregivers
* Fully wearable, battery-powered, sub-₹500 bill of materials

---

## Demo / Examples

### Images

<p align="center">
<img src="/assets/cover.png" width="800"><br/>
<i>PneumoGuard</i>
</p>

### Videos

<video controls width="100%">
  <source src="videos/myosa-demo.mp4" type="video/mp4">
  <!-- This text only shows if the browser doesn't support the video tag entirely -->
  Your browser does not support the video tag.
</video>

<br>
<i>Demo Video</i><br>
<i>If the video is not loaded, you can refer to the link <a href="https://drive.google.com/file/d/1XbHQ_bCuon8WHASx3V6HSJImWwQOupHc/view?usp=sharing">here</a>.</i>

<video controls width="100%">
  <source src="videos/myosa-presentaion.mp4" type="video/mp4">
  <!-- This text only shows if the browser doesn't support the video tag entirely -->
  Your browser does not support the video tag.
</video>

<br>
<i>Presentation Video</i><br>
<i>If the video is not loaded, you can refer to the link <a href="https://drive.google.com/file/d/1XWFNMp34jiv2y-eyGPzSh6iasK4rQmyM/view?usp=sharing">here</a>.</i>

---

## Features (Detailed)

### 1. Multi-Sensor Environmental Trigger Detection
The **BMP180** barometric pressure and temperature sensor is the primary trigger detector — a pressure drop of 2–4 hPa within a short window precedes roughly 80% of exacerbation episodes in the clinical literature we reviewed. PneumoGuard logs pressure at a fixed sampling interval and computes a rolling rate-of-change over a 30-minute sliding window, rather than relying on single-point thresholds, so that transient sensor noise does not trigger false alerts. This sliding-window approach is a standard technique for improving the **signal-to-noise reliability** of low-cost MEMS pressure sensors, since it smooths out short-term drift and single-sample outliers before they reach the decision layer.

### 2. Breathing-Effort Proxy via Motion Sensing
The **MPU6050** 6-axis accelerometer/gyroscope, worn on the chest, proxies breathing effort by measuring the variance and periodicity of chest-wall motion. A breathing-effort index is derived from the variance of the vertical-axis acceleration signal over the same sliding window used for pressure — laboured or irregular breathing produces a measurably higher variance than normal respiration. Because raw accelerometer output is noisy, the index is computed only after basic signal conditioning (windowed variance rather than instantaneous readings), which is the same statistical-smoothing principle used for the pressure channel and is standard practice for improving the reliability of low-cost inertial sensors in wearable health monitoring.

### 3. Context-Aware Risk Fusion
The **APDS-9960** ambient light/RGB/proximity sensor distinguishes indoor from outdoor context. This matters scientifically because pressure and humidity dynamics — and their clinical significance — differ indoors vs. outdoors; using a single fixed threshold for both settings would reduce specificity. The on-device algorithm therefore fuses all three sensor streams (pressure-drop rate, breathing-effort index, humidity-spike status) into a single composite score, weighting each factor by the current indoor/outdoor context, and outputs one of three discrete risk tiers: LOW, MEDIUM, HIGH.

### 4. Real-Time Local Alerting
The **OLED display** renders the live pressure trend as a simple line graph alongside a colour-coded risk gauge, so the risk state is legible at a glance without needing a connected phone. The **buzzer** sounds automatically whenever the composite score crosses into MEDIUM or HIGH risk, closing the sense-decide-act loop entirely on-device — this is important for patients in low-connectivity rural settings, where a cloud dependency would compromise the alert's reliability.

### 5. Caregiver Dashboard & Longitudinal Validation
Every reading (raw sensor values, computed features, and risk tier) is simultaneously transmitted over Wi-Fi/BLE to a lightweight cloud dashboard for caregiver or telemedicine review. For validation, we correlate six months of IMD (India Meteorological Department) Kerala weather data — specifically barometric pressure and humidity time series — against regional COPD hospital-admission records, using time-series overlay plots to visually and statistically confirm that admission spikes cluster around the pressure-drop events PneumoGuard is designed to detect. This kind of retrospective correlation plotting is a standard method for validating an environmental-trigger hypothesis before committing to a fixed on-device alert threshold, and it also lets us sanity-check the sensor's real-world reliability against an independent, clinically meaningful outcome (hospital admissions) rather than relying on sensor accuracy specs alone.

<p align="center">
<img src="assets/warning.jpeg" width="800"><br/>
<i>Web Dashboard</i>
</p>

---

## Scientific Methodology & Algorithm Design

Low-cost MEMS sensors are inherently noisy, cross-sensitive to confounding variables, and prone to drift — so PneumoGuard's core engineering challenge is not *reading* the sensors, but extracting a clinically trustworthy signal from them in real time, on a microcontroller with no cloud dependency. This section documents the algorithms used at each stage and the specific reliability problems they work around.

### 3.1 Stage 1 — Noise Rejection on the Pressure Channel

The BMP180's raw output fluctuates by ±0.3–0.5 hPa sample-to-sample even when true atmospheric pressure is stable — well within the same order of magnitude as the 2–4 hPa clinical trigger threshold. A single raw reading is therefore not trustworthy on its own.

**Workaround — Exponential Moving Average (EMA) filter:**

```
P_filtered[i] = α · P_raw[i] + (1 − α) · P_filtered[i−1],   α = 0.15
```

α = 0.15 was chosen empirically to suppress high-frequency sensor jitter while still tracking a genuine multi-hour pressure trend without excessive lag.

**A second, subtler problem:** the drop-rate feature is a *derivative* of pressure (`dP/dt`), and numerical differentiation amplifies any residual high-frequency noise left after filtering — visible in Fig. 1 as the pale red trace. A single-stage filter is not sufficient for a stable derivative.

**Workaround — double-stage smoothing:** the derivative itself is passed through a second EMA (α = 0.08) before being compared against the trigger threshold, giving the stable dark-red drop-rate curve in Fig. 1.

<p align="center">
<img src="assets/fig1-pressure-filtering.png" width="800"><br/>
<i>Fig. 1 — Raw vs. EMA-filtered BMP180 signal and the resulting double-smoothed pressure drop-rate (simulated data, illustrating the filtering algorithm)</i>
</p>

### 3.2 Stage 2 — Breathing-Effort Feature Extraction

A chest-mounted accelerometer does not measure "breathing" directly — it measures acceleration, which is a mixture of respiratory motion, postural shifts, and general body movement. Treating instantaneous acceleration as a breathing signal is unreliable.

**Workaround — windowed variance as a proxy feature:** rather than the raw signal, PneumoGuard computes the variance of the vertical-axis acceleration over a rolling 40-sample window:

```
BEI[i] = Var( a_z[i−w : i] ),   w = 40 samples
BEI_norm[i] = BEI[i] / max(BEI)
```

Laboured breathing manifests as elevated-amplitude, semi-periodic motion, which raises windowed variance measurably above the quiet-breathing baseline — this is a standard technique for turning a noisy motion signal into a stable physiological index without needing spectral analysis (FFT), which would be too computationally expensive to run continuously on the ESP32.

<p align="center">
<img src="assets/fig2-breathing-effort-index.png" width="800"><br/>
<i>Fig. 2 — Breathing-effort index (BEI) derived from MPU6050 windowed variance, showing detection of a simulated laboured-breathing episode</i>
</p>

### 3.3 Stage 3 — Context Normalization (Indoor / Outdoor)

Pressure and humidity dynamics differ meaningfully indoors vs. outdoors (e.g., HVAC-conditioned indoor air masks true atmospheric trends). Using one fixed threshold for both settings reduces specificity and risks false alerts.

**Workaround — APDS-9960-derived context weighting:** ambient light and proximity readings classify the current environment, and this context feeds into the fusion step below as a weighting term rather than a hard switch — avoiding abrupt risk-score discontinuities when a patient walks between indoor and outdoor settings.

### 3.4 Stage 4 — Weighted Sensor Fusion & Risk Tiering

The three processed features are combined into a single composite score:

```
R(t) = w_p · PDR_norm(t) + w_b · BEI_norm(t) + w_c · C(t)

where   w_p = 0.5,  w_b = 0.3,  w_c = 0.2   (empirically weighted; pressure
        drop-rate is the strongest single predictor per clinical literature)

Risk tier =  LOW      if R(t) ≤ 0.35
             MEDIUM   if 0.35 < R(t) ≤ 0.60
             HIGH     if R(t) > 0.60
```

The weights reflect that pressure-drop rate is the dominant clinical predictor (≈80% of exacerbations per the literature reviewed), with breathing effort and context acting as corroborating/adjusting signals rather than equal partners — this avoids a false HIGH alert being triggered by motion noise alone.

<p align="center">
<img src="assets/fig3-risk-fusion.png" width="800"><br/>
<i>Fig. 3 — Composite risk-fusion score R(t) over time with LOW/MEDIUM/HIGH alert bands (simulated data, illustrating the weighted-fusion algorithm)</i>
</p>

### 3.5 Validation Approach

Before fixing the threshold constants above, the underlying hypothesis — that pressure-drop events precede COPD hospital admissions — is checked against real-world outcome data, using a simple linear-correlation plot of monthly pressure-drop-event counts (IMD Kerala) against regional COPD admissions:

<p align="center">
<img src="assets/fig4-correlation-validation.png" width="700"><br/>
<i>Fig. 4 — Illustrative validation method: correlating pressure-drop-event frequency with COPD hospital admissions over a 6-month window (synthetic data shown here to demonstrate the method; final proposal will substitute real IMD/hospital records)</i>
</p>

This correlation-and-linear-fit approach is a standard, low-overhead way to sanity-check an environmental-trigger hypothesis before committing engineering effort to on-device thresholds — it does not prove causation, but a strong positive correlation (as illustrated) supports proceeding with clinical pilot testing.

**Summary of reliability workarounds implemented:**

| Sensor issue | Workaround |
|---|---|
| BMP180 sample-to-sample jitter | EMA filtering (α = 0.15) |
| Noise amplification in the pressure derivative | Second-stage EMA on drop-rate (α = 0.08) |
| Accelerometer signal mixes breathing with general motion | Windowed-variance feature (BEI) instead of raw signal |
| Fixed thresholds fail across indoor/outdoor contexts | APDS-9960 context weighting in the fusion formula |
| Single noisy sensor causing false alerts | Multi-sensor weighted fusion instead of single-threshold triggering |

---

## Usage Instructions
**Clone the Repository**
Start by cloning the project repository to your local machine and navigating into the project folder:

```bash
git clone [https://github.com/NAVAJYOTH-KRISHNAN/myosa-pneumoguard.git](https://github.com/NAVAJYOTH-KRISHNAN/myosa-pneumoguard.git)
cd myosa-pneumoguard
```
Example firmware upload command (Arduino CLI on the MYOSA Mini / ESP32 target):

```plaintext
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 ./pneumoguard_firmware

(also setup the web dashboard file as given in the repo)
```

Example on-device risk-scoring snippet:
(Algorithm Pseudocode .The code is in C!) 

```python
# Simplified illustrative snippet of the sliding-window risk score
def compute_risk(pressure_window, accel_window, is_indoor):
    pressure_drop_rate = (pressure_window[0] - pressure_window[-1]) / len(pressure_window)
    breathing_effort_index = variance(accel_window)
    threshold = INDOOR_THRESHOLD if is_indoor else OUTDOOR_THRESHOLD

    score = (0.5 * pressure_drop_rate) + (0.3 * breathing_effort_index) + (0.2 * threshold)

    if score > HIGH_RISK_CUTOFF:
        return "HIGH"
    elif score > MEDIUM_RISK_CUTOFF:
        return "MEDIUM"
    return "LOW"
```

---

## Tech Stack

* **MYOSA Mini (ESP32-based motherboard)** — central controller, Wi-Fi/BLE connectivity
* **BMP180** — barometric pressure & temperature sensing
* **MPU6050** — 6-axis motion sensing (breathing-effort proxy)
* **APDS-9960** — ambient light / RGB / proximity (indoor-outdoor context)
* **OLED Display** — local real-time risk visualisation
* **Buzzer / Actuator** — audible patient alerting
* **Arduino/C++ firmware** — on-device sliding-window signal processing and risk-fusion algorithm
* **Cloud dashboard** (lightweight web front-end) — caregiver/telemedicine trend review
* **Python** — offline validation and correlation analysis against IMD Kerala weather and hospital-admission data

---

## Requirements / Installation
Firmware dependencies (Arduino/PlatformIO libraries):

```bash
arduino-cli lib install "Adafruit BMP085 Library" "MPU6050" "Adafruit SSD1306"
```

Hardware requirements:
* MYOSA Mini Kit (ESP32 motherboard + BMP180 + MPU6050 + APDS-9960 + OLED + Buzzer)
* Li-Po battery and charging module
* 3D-printed / laser-cut chest-strap wearable enclosure

---

## File Structure (Optional)

```
/myosa-pneumoguard
├── assets/
│   ├── cover.jpeg
│   ├── warning.jpeg
│   ├── cover.png
│   ├── fig1-pressure-filtering.png
│   ├── fig2-breathing-effort-index.png
│   ├── fig3-risk-fusion.png
│   └── fig4-correlation-validation.png
├── dashboard/
│   └── appscript.js
├── firmware/
│   └── pneumoguard_firmware.ino
├── videos/
│   ├── myosa-demo.mp4
│   └── myosa-presentation.mp4
├── LICENSE
└── README.md
```

---

## License (Optional)

This project is released under the MIT License. See `LICENSE` for details.

---

## Contribution Notes (Optional)

We welcome feedback from the MYOSA mentor panel and community, particularly on refining the risk-fusion weighting and expanding the validation dataset beyond six months of IMD Kerala data. Issues and suggestions can be raised via the project's GitHub repository once published.
