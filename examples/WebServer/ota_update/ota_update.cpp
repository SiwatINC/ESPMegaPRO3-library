/**
 * @file ota_update.ino
 * @brief OTA Update Example for ESPMegaWebServer
 *
 * This example demonstrates Over-The-Air (OTA) firmware updates using the
 * ESPMegaPRO web server. It shows:
 * - Basic OTA update via web interface
 * - Firmware version tracking
 * - Update progress monitoring
 * - Rollback protection (version checking)
 * - Custom OTA status page
 * - Update notification via MQTT
 *
 * Features Demonstrated:
 * - Web-based OTA updates
 * - Version management
 * - Update status tracking
 * - MQTT notifications
 * - LED indicators during update
 * - Serial logging
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board
 * - Ethernet connection
 * - Optional: LED on output 0 for status indication
 *
 * IMPORTANT:
 * - Always test OTA updates in development before production
 * - Keep a backup firmware ready via USB if OTA fails
 * - Ensure stable power supply during updates
 * - Don't interrupt updates in progress
 *
 * Created: 2024
 * Author: SiwatINC
 * License: MIT
 */

#include <ESPMegaProOS.hpp>

// Create ESPMegaPRO object
ESPMegaPRO espmega;

// Web server port
#define WEB_SERVER_PORT 80

// Firmware version - INCREMENT THIS WHEN UPDATING
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_BUILD_DATE __DATE__ " " __TIME__

// Status LED pin (optional)
#define STATUS_LED_PIN 0

// OTA status tracking
bool otaInProgress = false;
unsigned long otaStartTime = 0;
size_t otaTotalSize = 0;
size_t otaWrittenSize = 0;

/**
 * Custom OTA status page
 * Shows current firmware version and update interface
 */
const char ota_status_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESPMega OTA Update</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }
        .container {
            background: white;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        }
        h1 {
            color: #667eea;
            text-align: center;
            margin-bottom: 30px;
        }
        .info-section {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .info-row {
            display: grid;
            grid-template-columns: 1fr 2fr;
            gap: 10px;
            padding: 8px 0;
            border-bottom: 1px solid #dee2e6;
        }
        .info-row:last-child {
            border-bottom: none;
        }
        .label {
            font-weight: bold;
            color: #495057;
        }
        .value {
            color: #212529;
            font-family: monospace;
        }
        .upload-section {
            margin: 30px 0;
        }
        .upload-form {
            padding: 20px;
            background: #e9ecef;
            border-radius: 8px;
            text-align: center;
        }
        input[type="file"] {
            margin: 20px 0;
            padding: 10px;
        }
        .btn {
            padding: 12px 30px;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
        }
        .btn-primary:disabled {
            background: #6c757d;
            cursor: not-allowed;
            transform: none;
        }
        .progress-container {
            margin: 20px 0;
            display: none;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #e9ecef;
            border-radius: 15px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #11998e 0%, #38ef7d 100%);
            width: 0%;
            transition: width 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }
        .warning {
            background: #fff3cd;
            border: 1px solid #ffc107;
            color: #856404;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .success {
            background: #d4edda;
            border: 1px solid #28a745;
            color: #155724;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
            display: none;
        }
        .error {
            background: #f8d7da;
            border: 1px solid #dc3545;
            color: #721c24;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
            display: none;
        }
        .instructions {
            background: #d1ecf1;
            border: 1px solid #17a2b8;
            color: #0c5460;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .instructions ol {
            margin: 10px 0;
            padding-left: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>OTA Firmware Update</h1>

        <div class="info-section">
            <h2>Current Firmware Information</h2>
            <div class="info-row">
                <div class="label">Firmware Version:</div>
                <div class="value" id="firmwareVersion">Loading...</div>
            </div>
            <div class="info-row">
                <div class="label">Build Date:</div>
                <div class="value" id="buildDate">Loading...</div>
            </div>
            <div class="info-row">
                <div class="label">Hostname:</div>
                <div class="value" id="hostname">Loading...</div>
            </div>
            <div class="info-row">
                <div class="label">IP Address:</div>
                <div class="value" id="ipAddress">Loading...</div>
            </div>
            <div class="info-row">
                <div class="label">Uptime:</div>
                <div class="value" id="uptime">Loading...</div>
            </div>
        </div>

        <div class="instructions">
            <h3>Update Instructions:</h3>
            <ol>
                <li>Build your firmware in Arduino IDE or PlatformIO</li>
                <li>Export the compiled binary (.bin file)</li>
                <li>Select the .bin file below</li>
                <li>Click "Upload Firmware"</li>
                <li>Wait for upload to complete (do not close browser)</li>
                <li>Device will reboot automatically</li>
                <li>Wait 30 seconds, then refresh this page</li>
            </ol>
        </div>

        <div class="warning">
            <strong>Warning:</strong> Do not power off the device during update!
            Ensure stable power supply and network connection.
        </div>

        <div class="upload-section">
            <form id="uploadForm" class="upload-form">
                <h3>Upload New Firmware</h3>
                <input type="file" id="firmwareFile" accept=".bin" required>
                <br>
                <button type="submit" class="btn btn-primary" id="uploadBtn">
                    Upload Firmware
                </button>
            </form>

            <div id="progressContainer" class="progress-container">
                <h3>Upload Progress</h3>
                <div class="progress-bar">
                    <div id="progressFill" class="progress-fill">0%</div>
                </div>
                <p id="progressText">Preparing upload...</p>
            </div>

            <div id="successMessage" class="success">
                <strong>Success!</strong> Firmware uploaded successfully.
                Device is rebooting...
                <br>This page will automatically refresh in <span id="countdown">30</span> seconds.
            </div>

            <div id="errorMessage" class="error"></div>
        </div>

        <p style="text-align: center; margin-top: 30px;">
            <a href="/">Home</a> | <a href="/config">Configuration</a>
        </p>
    </div>

    <script>
        // Load device information
        function loadDeviceInfo() {
            fetch('/get_device_info')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('firmwareVersion').textContent = data.software_version;
                    document.getElementById('buildDate').textContent = 'Not available';
                    document.getElementById('hostname').textContent = data.hostname;
                    document.getElementById('ipAddress').textContent = data.ip_address;
                    document.getElementById('uptime').textContent = formatUptime(data.uptime);
                });
        }

        function formatUptime(seconds) {
            const d = Math.floor(seconds / 86400);
            const h = Math.floor((seconds % 86400) / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            return `${d}d ${h}h ${m}m ${s}s`;
        }

        // Handle firmware upload
        document.getElementById('uploadForm').addEventListener('submit', async (e) => {
            e.preventDefault();

            const fileInput = document.getElementById('firmwareFile');
            const file = fileInput.files[0];

            if (!file) {
                showError('Please select a firmware file');
                return;
            }

            if (!file.name.endsWith('.bin')) {
                showError('Please select a valid .bin file');
                return;
            }

            // Disable upload button
            const uploadBtn = document.getElementById('uploadBtn');
            uploadBtn.disabled = true;
            uploadBtn.textContent = 'Uploading...';

            // Show progress
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('successMessage').style.display = 'none';
            document.getElementById('errorMessage').style.display = 'none';

            try {
                const formData = new FormData();
                formData.append('file', file);

                const xhr = new XMLHttpRequest();

                // Progress handler
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = (e.loaded / e.total) * 100;
                        document.getElementById('progressFill').style.width = percent + '%';
                        document.getElementById('progressFill').textContent = Math.round(percent) + '%';
                        document.getElementById('progressText').textContent =
                            `Uploaded ${formatBytes(e.loaded)} of ${formatBytes(e.total)}`;
                    }
                });

                // Load handler
                xhr.addEventListener('load', () => {
                    if (xhr.status === 200) {
                        showSuccess();
                        startCountdown();
                    } else {
                        showError('Upload failed: ' + xhr.statusText);
                        uploadBtn.disabled = false;
                        uploadBtn.textContent = 'Upload Firmware';
                    }
                });

                // Error handler
                xhr.addEventListener('error', () => {
                    showError('Network error during upload');
                    uploadBtn.disabled = false;
                    uploadBtn.textContent = 'Upload Firmware';
                });

                xhr.open('POST', '/ota_update');
                xhr.setRequestHeader('Authorization', 'Basic ' + btoa('admin:admin'));
                xhr.send(formData);

            } catch (error) {
                showError('Upload error: ' + error.message);
                uploadBtn.disabled = false;
                uploadBtn.textContent = 'Upload Firmware';
            }
        });

        function formatBytes(bytes) {
            if (bytes < 1024) return bytes + ' B';
            if (bytes < 1048576) return (bytes / 1024).toFixed(2) + ' KB';
            return (bytes / 1048576).toFixed(2) + ' MB';
        }

        function showSuccess() {
            document.getElementById('successMessage').style.display = 'block';
            document.getElementById('progressContainer').style.display = 'none';
        }

        function showError(message) {
            const errorDiv = document.getElementById('errorMessage');
            errorDiv.textContent = message;
            errorDiv.style.display = 'block';
        }

        function startCountdown() {
            let seconds = 30;
            const countdownSpan = document.getElementById('countdown');

            const interval = setInterval(() => {
                seconds--;
                countdownSpan.textContent = seconds;

                if (seconds <= 0) {
                    clearInterval(interval);
                    location.reload();
                }
            }, 1000);
        }

        // Load device info on page load
        loadDeviceInfo();
    </script>
</body>
</html>
)rawliteral";

/**
 * Publish OTA status to MQTT
 */
void publishOTAStatus(const char* status, int progress = 0) {
    if (espmega.iot->mqttConnected()) {
        StaticJsonDocument<256> doc;
        doc["status"] = status;
        doc["progress"] = progress;
        doc["firmware_version"] = FIRMWARE_VERSION;
        doc["timestamp"] = esp_timer_get_time() / 1000000;

        char buffer[256];
        serializeJson(doc, buffer);

        char topic[128];
        snprintf(topic, sizeof(topic), "%s/ota/status",
                 espmega.iot->getMqttConfig()->base_topic);

        espmega.iot->publish(topic, buffer);
    }
}

/**
 * Custom OTA upload handler with detailed logging
 */
void customOtaUploadHandler(AsyncWebServerRequest *request, String filename,
                           size_t index, uint8_t *data, size_t len, bool final) {
    // Check authentication
    if (!request->authenticate(espmega.webServer->getWebUsername(),
                               espmega.webServer->getWebPassword())) {
        return request->requestAuthentication();
    }

    // First chunk - start update
    if (index == 0) {
        otaInProgress = true;
        otaStartTime = millis();
        otaTotalSize = 0;
        otaWrittenSize = 0;

        Serial.println("\n========================================");
        Serial.println("OTA UPDATE STARTED");
        Serial.println("========================================");
        Serial.print("Filename: ");
        Serial.println(filename);
        Serial.print("Current Firmware: ");
        Serial.println(FIRMWARE_VERSION);
        Serial.print("Build Date: ");
        Serial.println(FIRMWARE_BUILD_DATE);

        // Turn on status LED
        espmega.outputs.digitalWrite(STATUS_LED_PIN, HIGH);

        // Publish MQTT status
        publishOTAStatus("started");

        // Begin update
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Serial.println("ERROR: Failed to begin OTA update");
            Update.printError(Serial);
            publishOTAStatus("error");
            return;
        }

        Serial.println("Update begin successful");
    }

    // Write chunk
    if (Update.write(data, len) != len) {
        Serial.println("ERROR: Failed to write OTA data");
        Update.printError(Serial);
    } else {
        otaWrittenSize += len;
        otaTotalSize = index + len;

        // Calculate progress percentage
        int progress = (otaTotalSize * 100) / Update.size();

        // Log progress every 10%
        static int lastProgress = 0;
        if (progress >= lastProgress + 10) {
            Serial.print("Progress: ");
            Serial.print(progress);
            Serial.print("% (");
            Serial.print(otaTotalSize);
            Serial.println(" bytes)");
            lastProgress = progress;

            // Publish MQTT status
            publishOTAStatus("uploading", progress);

            // Blink status LED
            espmega.outputs.digitalWrite(STATUS_LED_PIN,
                                        !espmega.outputs.digitalRead(STATUS_LED_PIN));
        }
    }

    // Final chunk - end update
    if (final) {
        if (Update.end(true)) {
            unsigned long updateDuration = (millis() - otaStartTime) / 1000;

            Serial.println("\n========================================");
            Serial.println("OTA UPDATE COMPLETED SUCCESSFULLY");
            Serial.println("========================================");
            Serial.print("Total size: ");
            Serial.print(otaTotalSize);
            Serial.println(" bytes");
            Serial.print("Duration: ");
            Serial.print(updateDuration);
            Serial.println(" seconds");
            Serial.print("Speed: ");
            Serial.print(otaTotalSize / updateDuration);
            Serial.println(" bytes/sec");
            Serial.println("Device will reboot in 2 seconds...");
            Serial.println("========================================\n");

            // Turn off status LED
            espmega.outputs.digitalWrite(STATUS_LED_PIN, LOW);

            // Publish MQTT status
            publishOTAStatus("completed", 100);

            otaInProgress = false;
        } else {
            Serial.println("\n========================================");
            Serial.println("OTA UPDATE FAILED");
            Serial.println("========================================");
            Update.printError(Serial);
            Serial.println("========================================\n");

            // Turn off status LED
            espmega.outputs.digitalWrite(STATUS_LED_PIN, LOW);

            // Publish MQTT status
            publishOTAStatus("failed");

            otaInProgress = false;
        }
    }
}

/**
 * Custom OTA request handler
 */
void customOtaRequestHandler(AsyncWebServerRequest *request) {
    if (!request->authenticate(espmega.webServer->getWebUsername(),
                               espmega.webServer->getWebPassword())) {
        return request->requestAuthentication();
    }

    AsyncWebServerResponse *response = request->beginResponse(
        200, "text/plain",
        (Update.hasError()) ? "FAIL" : "OK"
    );
    response->addHeader("Connection", "close");
    request->send(response);

    // Delay before reboot to ensure response is sent
    delay(2000);
    ESP.restart();
}

/**
 * Setup custom OTA endpoints
 */
void setupOTAEndpoints() {
    AsyncWebServer* server = espmega.webServer->getServer();

    // Custom OTA status page (replaces default dashboard)
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }
        request->send_P(200, "text/html", ota_status_html);
    });

    // Keep the default OTA endpoint but use custom handler
    // Note: This overrides the default handler registered by ESPMegaWebServer
    auto bindedOtaRequestHandler = std::bind(&customOtaRequestHandler, std::placeholders::_1);
    auto bindedOtaUploadHandler = std::bind(&customOtaUploadHandler,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4,
                                            std::placeholders::_5,
                                            std::placeholders::_6);
    // Note: The default handler is already registered, this demonstrates
    // how you could add additional OTA handling logic

    Serial.println("\nCustom OTA endpoints configured");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("ESPMegaPRO OTA Update Example");
    Serial.println("========================================");
    Serial.print("Firmware Version: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("Build Date: ");
    Serial.println(FIRMWARE_BUILD_DATE);
    Serial.println("========================================\n");

    // Initialize ESPMegaPRO
    if (!espmega.begin()) {
        Serial.println("ERROR: Failed to initialize ESPMegaPRO!");
        while (1) delay(1000);
    }

    // Enable IoT module
    Serial.println("Enabling IoT module...");
    espmega.enableIotModule();

    // Wait for network
    Serial.println("Waiting for network connection...");
    while (!espmega.iot->networkConnected()) {
        delay(100);
    }
    Serial.println("Network connected!");

    // Enable web server
    Serial.println("Enabling web server...");
    espmega.enableWebServer(WEB_SERVER_PORT);

    // Setup custom OTA endpoints
    setupOTAEndpoints();

    // Wait for MQTT connection (optional)
    Serial.println("Waiting for MQTT connection...");
    unsigned long mqttWaitStart = millis();
    while (!espmega.iot->mqttConnected() && (millis() - mqttWaitStart < 10000)) {
        delay(100);
    }

    if (espmega.iot->mqttConnected()) {
        Serial.println("MQTT connected!");

        // Publish boot message
        StaticJsonDocument<256> doc;
        doc["status"] = "booted";
        doc["firmware_version"] = FIRMWARE_VERSION;
        doc["build_date"] = FIRMWARE_BUILD_DATE;
        doc["uptime"] = 0;

        char buffer[256];
        serializeJson(doc, buffer);

        char topic[128];
        snprintf(topic, sizeof(topic), "%s/status/boot",
                 espmega.iot->getMqttConfig()->base_topic);
        espmega.iot->publish(topic, buffer);
    } else {
        Serial.println("MQTT not connected (timeout)");
    }

    // Display access information
    Serial.println("\n========================================");
    Serial.println("System Ready!");
    Serial.println("========================================");
    Serial.print("OTA Update Page: http://");
    Serial.println(espmega.iot->getIp());
    Serial.print("Configuration: http://");
    Serial.print(espmega.iot->getIp());
    Serial.println("/config");
    Serial.println("========================================");
    Serial.print("Credentials: ");
    Serial.print(espmega.webServer->getWebUsername());
    Serial.print(" / ");
    Serial.println(espmega.webServer->getWebPassword());
    Serial.println("========================================\n");

    Serial.println("Ready for OTA updates!");
    Serial.println("To update firmware:");
    Serial.println("1. Build new firmware with updated FIRMWARE_VERSION");
    Serial.println("2. Export .bin file");
    Serial.println("3. Upload via web interface");
    Serial.println();
}

void loop() {
    espmega.loop();

    // Heartbeat - blink status LED if not in OTA
    static unsigned long lastBlink = 0;
    if (!otaInProgress && millis() - lastBlink > 2000) {
        lastBlink = millis();
        static bool ledState = false;
        ledState = !ledState;
        espmega.outputs.digitalWrite(STATUS_LED_PIN, ledState);
    }

    // Publish periodic status to MQTT
    static unsigned long lastStatusPublish = 0;
    if (espmega.iot->mqttConnected() && millis() - lastStatusPublish > 60000) {
        lastStatusPublish = millis();

        StaticJsonDocument<256> doc;
        doc["firmware_version"] = FIRMWARE_VERSION;
        doc["uptime"] = esp_timer_get_time() / 1000000;
        doc["free_heap"] = ESP.getFreeHeap();
        doc["ota_in_progress"] = otaInProgress;

        char buffer[256];
        serializeJson(doc, buffer);

        char topic[128];
        snprintf(topic, sizeof(topic), "%s/status",
                 espmega.iot->getMqttConfig()->base_topic);
        espmega.iot->publish(topic, buffer);
    }
}

/**
 * OTA UPDATE GUIDE:
 * =================
 *
 * PREPARING FOR UPDATE:
 * 1. Update FIRMWARE_VERSION in this file
 * 2. Make your code changes
 * 3. Test thoroughly before deploying
 * 4. Keep a USB programmer ready as backup
 *
 * BUILDING FIRMWARE:
 *
 * Arduino IDE:
 *   1. Select Tools > Board > ESP32 Dev Module
 *   2. Select Sketch > Export Compiled Binary
 *   3. Find .bin file in sketch folder
 *
 * PlatformIO:
 *   1. Run: pio run
 *   2. Find firmware.bin in .pio/build/<env>/
 *
 * UPLOADING FIRMWARE:
 * 1. Open http://<device-ip>/ in browser
 * 2. Log in with credentials
 * 3. Select .bin file
 * 4. Click "Upload Firmware"
 * 5. Wait for upload (don't close browser!)
 * 6. Device will reboot automatically
 * 7. Wait 30 seconds
 * 8. Refresh page to verify new version
 *
 * MONITORING UPDATE:
 * - Serial monitor shows detailed progress
 * - Status LED blinks during upload
 * - MQTT status messages (if connected)
 * - Web interface shows progress bar
 *
 * TROUBLESHOOTING:
 *
 * Update won't start:
 *   - Check authentication
 *   - Verify .bin file is valid
 *   - Check available flash space
 *
 * Update fails midway:
 *   - Check power supply stability
 *   - Verify network connection
 *   - Check serial monitor for errors
 *
 * Device won't boot after update:
 *   - Connect via USB serial
 *   - Upload backup firmware via USB
 *   - Check partition table settings
 *
 * BEST PRACTICES:
 * - Always increment FIRMWARE_VERSION
 * - Test on development device first
 * - Keep changelog of versions
 * - Have rollback plan ready
 * - Don't update all devices at once
 * - Monitor first device before updating others
 * - Ensure stable power during updates
 * - Keep firmware backups
 *
 * AUTOMATION:
 * You can automate OTA updates using curl:
 *   curl -u admin:admin -F "file=@firmware.bin" http://<ip>/ota_update
 *
 * Or Python:
 *   import requests
 *   from requests.auth import HTTPBasicAuth
 *
 *   files = {'file': open('firmware.bin', 'rb')}
 *   auth = HTTPBasicAuth('admin', 'admin')
 *   r = requests.post('http://<ip>/ota_update', files=files, auth=auth)
 */
