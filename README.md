# ESP_GEM: AI Camera with Gemini Integration

ESP_GEM is an ESP32-S3-based AI camera project that captures images, analyzes them using Google's Gemini AI, and provides a web interface for viewing results. It supports real-time chat with the AI about captured images and logs conversations.

## Features

- **Image Capture:** High-quality image capture using ESP32-S3-EYE with OV2640/OV3660 camera.
- **AI Analysis:** Upload images to Gemini API for detailed descriptions and analysis.
- **Serial Chat:** Interact with Gemini via serial commands, including image context.
- **Web Server:** Built-in web server to view captured images and chat logs.
- **Persistent Storage:** Saves images and logs to LittleFS, with unique filenames(using Time API).
- **LED Indicator:** Visual feedback during capture using GPIO2.
- **Command System:** Supports CAPTURE, CHAT, RESET, and CLEAR commands via serial.

## Hardware Requirements

- **ESP32-S3-EYE Development Board:** Includes camera, LCD, and microphone (camera module used here).
- **USB Cable:** For programming and power.
- **Optional:** External LED on GPIO2 for visual feedback.

## Software Requirements

- **PlatformIO:** For building and uploading the project.
- **Arduino Framework:** ESP32 board support.
- **Libraries:**
  - ArduinoJson
  - esp32-camera
  - ESP32Async/AsyncTCP
  - ESP32Async/ESPAsyncWebServer
  - Preferences (built-in ESP32)

## Installation

1. **Clone the Repository:**

   ```bash
   git clone <repository-url>
   cd ESP_GEM
   ```

2. **Install PlatformIO:** If not installed, follow [PlatformIO installation guide](https://platformio.org/install).

3. **Configure Gemini API:**
   - Obtain a Gemini API key from [Google AI Studio](https://makersuite.google.com/app/apikey).
   - In `src/config.cpp` or environment variables, set `GEMINI_API_KEY` and `GEMINI_MODEL` and `Region` for Time API.

4. **WiFi Setup:**
   - Update `ssid` and `password` in the code for your network.

5. **Build and Upload:**

   ```bash
   pio run -t upload
   ```

6. **Monitor Serial:**
   ```bash
   pio device monitor
   ```

## Usage

1. **Power On:** The ESP32 connects to WiFi and starts the web server.

2. **Capture Image:**
   - Send `CAPTURE:` via serial monitor.
   - Image is saved to LittleFS and uploaded to Gemini.

3. **Chat with AI:**
   - Send `CHAT:<message>` to query Gemini, including the last captured image context.

4. **Web Interface:**
   - Access `http://<ESP32_IP>/` for the main page.
   - `http://<ESP32_IP>/api/images` lists .jpg files.
   - `http://<ESP32_IP>/api/files` lists .md logs.
   - Direct links: `http://<ESP32_IP>/<filename>`.

## Serial Commands

- **CAPTURE:** Captures and uploads an image to Gemini.
- **CHAT:<message>** Sends a message to Gemini, including image context.
- **RESET:** Resets the image counter in Preferences.
- **CLEAR:** Deletes all files in LittleFS.

## Configuration

- **Camera Settings:** Adjusted in `src/camera.cpp` (brightness, contrast, etc.).
- **LED Pin:** GPIO2 in `src/main.cpp` and `src/serial.cpp`.
- **File Storage:** LittleFS for images (.jpg) and logs (.md).
- **Time API:** Used for timestamped filenames; falls back to counters using Preferences.h.

## Troubleshooting

- **Camera Not Working:** Check camera pins in `include/camera.h`. Ensure PSRAM is enabled.
- **WiFi Issues:** Verify credentials and network.
- **Gemini Errors:** Check API key and quota.
- **Image Quality:** Adjust lighting or camera settings.
- **Web Server:** Hard refresh browser if images don't update.
- **Serial Issues:** Ensure correct line endings (CRLF).

## Project Structure

- `src/`: Main source files (main.cpp, camera.cpp, serial.cpp, etc.).
- `include/`: Headers (camera.h, config.h, etc.).
- `lib/`: Custom libraries if any.
- `test/`: Test files.
- `platformio.ini`: Build configuration.

## Acknowledgments

- [Neovim PlatformIO](https://github.com/anurag3301/nvim-platformio.lua).
- ESP32 community and Espressif.
- Google Gemini API.
- PlatformIO for tooling.
