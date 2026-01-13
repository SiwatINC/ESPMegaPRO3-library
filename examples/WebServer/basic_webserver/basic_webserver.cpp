/**
 * @file basic_webserver.ino
 * @brief Basic Web Server Example for ESPMegaPRO
 *
 * This example demonstrates the minimal setup required to enable the web server
 * on an ESPMegaPRO board. It provides:
 * - Built-in web interface for OTA updates
 * - Configuration page for network and MQTT settings
 * - Basic authentication
 * - Device information API
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board
 * - Ethernet connection (or WiFi if configured)
 * - Power supply (12V or 24V DC)
 *
 * Default Access:
 * - URL: http://<device-ip>/ (Check serial monitor for IP address)
 * - Username: admin
 * - Password: admin
 *
 * IMPORTANT SECURITY NOTE:
 * The default credentials are "admin"/"admin". For production use, you MUST
 * change these credentials using the web interface or in code (see below).
 *
 * Created: 2024
 * Author: SiwatINC
 * License: MIT
 */

#include <ESPMegaProOS.hpp>

// Create ESPMegaPRO object
ESPMegaPRO espmega;

// Web server port (80 is standard HTTP port)
#define WEB_SERVER_PORT 80

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    Serial.println("ESPMegaPRO Basic Web Server Example");
    Serial.println("====================================");

    // Initialize the ESPMegaPRO board
    // This initializes FRAM, I/O cards, and other hardware components
    if (!espmega.begin()) {
        Serial.println("ERROR: Failed to initialize ESPMegaPRO!");
        Serial.println("Please check your hardware and try again.");
        while (1) {
            delay(1000);
        }
    }
    Serial.println("ESPMegaPRO initialized successfully");

    // Enable the IoT module
    // This initializes network connectivity (Ethernet/WiFi) and MQTT
    // REQUIRED before enabling web server
    Serial.println("Enabling IoT module...");
    espmega.enableIotModule();

    // Wait for network connection
    Serial.println("Waiting for network connection...");
    while (!espmega.iot->networkConnected()) {
        delay(100);
    }
    Serial.println("Network connected!");

    // Enable the web server on specified port
    // This starts the AsyncWebServer and registers all built-in endpoints
    Serial.println("Enabling web server...");
    espmega.enableWebServer(WEB_SERVER_PORT);

    // --- OPTIONAL: Set custom credentials ---
    // Uncomment the lines below to set custom username/password
    // IMPORTANT: After first boot with new credentials, comment these lines out
    // to prevent credentials from being reset on every boot

    /*
    espmega.webServer->setWebUsername("myuser");
    espmega.webServer->setWebPassword("mySecurePassword123");
    espmega.webServer->saveCredentialsToFRAM();
    Serial.println("Custom credentials set and saved to FRAM");
    */

    // Display connection information
    Serial.println("\n====================================");
    Serial.println("Web Server Started Successfully!");
    Serial.println("====================================");
    Serial.print("IP Address: ");
    Serial.println(espmega.iot->getIp());
    Serial.print("MAC Address: ");
    Serial.println(espmega.iot->getMac());
    Serial.print("Hostname: ");
    Serial.println(espmega.iot->getNetworkConfig()->hostname);
    Serial.println("------------------------------------");
    Serial.print("Web Interface: http://");
    Serial.println(espmega.iot->getIp());
    Serial.print("Configuration: http://");
    Serial.print(espmega.iot->getIp());
    Serial.println("/config");
    Serial.println("------------------------------------");
    Serial.print("Username: ");
    Serial.println(espmega.webServer->getWebUsername());
    Serial.print("Password: ");
    Serial.println(espmega.webServer->getWebPassword());
    Serial.println("====================================");
    Serial.println("\nAvailable Endpoints:");
    Serial.println("  GET  /              - Dashboard (OTA update page)");
    Serial.println("  GET  /config        - Configuration page");
    Serial.println("  GET  /get_config    - Get current configuration (JSON)");
    Serial.println("  POST /save_config   - Save configuration (JSON)");
    Serial.println("  GET  /get_device_info - Get device information (JSON)");
    Serial.println("  POST /ota_update    - Upload firmware for OTA update");
    Serial.println("  GET  /reboot        - Reboot device");
    Serial.println("\nAll endpoints require authentication.");
    Serial.println("====================================\n");
}

void loop() {
    // Main loop - handles all ESPMegaPRO operations
    // This includes:
    // - IoT/MQTT communication
    // - Web server requests (handled asynchronously)
    // - I/O card updates
    // - Internal display updates (if enabled)
    espmega.loop();

    // The web server is fully asynchronous, so no additional code is needed
    // in the loop() function for basic web server operation.

    // You can add your custom application logic here:
    // - Read inputs
    // - Control outputs
    // - Process data
    // - Communicate with other systems
    // etc.

    // Example: Print IP address every 30 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 30000) {
        lastPrint = millis();
        Serial.print("Web server running at http://");
        Serial.println(espmega.iot->getIp());
    }
}

/**
 * USAGE INSTRUCTIONS:
 * ===================
 *
 * 1. HARDWARE SETUP
 *    - Connect ESPMegaPRO to power supply
 *    - Connect Ethernet cable (or configure WiFi)
 *    - Connect USB cable for programming and serial monitoring
 *
 * 2. SOFTWARE SETUP
 *    - Install ESPMegaPRO library in Arduino IDE or PlatformIO
 *    - Select correct board: ESP32 Dev Module
 *    - Upload this sketch
 *
 * 3. FIRST RUN
 *    - Open Serial Monitor (115200 baud)
 *    - Wait for "Network connected!" message
 *    - Note the IP address displayed
 *
 * 4. ACCESS WEB INTERFACE
 *    - Open web browser
 *    - Navigate to http://<ip-address>/
 *    - Login with username: admin, password: admin
 *    - You will see the OTA update page (dashboard)
 *
 * 5. CONFIGURATION
 *    - Navigate to http://<ip-address>/config
 *    - Configure network settings (IP, gateway, DNS)
 *    - Configure MQTT broker settings
 *    - CHANGE default credentials (important!)
 *    - Click "Save" - device will reboot with new settings
 *
 * 6. OTA FIRMWARE UPDATE
 *    - Compile your updated firmware
 *    - Export compiled binary (.bin file)
 *    - Navigate to http://<ip-address>/
 *    - Click "Choose File" and select .bin file
 *    - Click "Update"
 *    - Wait for upload to complete
 *    - Device will reboot with new firmware
 *
 * 7. API USAGE
 *    - Get device info: GET /get_device_info
 *    - Get config: GET /get_config
 *    - All API endpoints return JSON
 *    - All endpoints require HTTP Basic Authentication
 *
 * TROUBLESHOOTING:
 * ================
 *
 * Cannot access web interface:
 *    - Check serial monitor for IP address
 *    - Verify network connection
 *    - Try pinging the device
 *    - Check firewall settings
 *
 * Authentication fails:
 *    - Default is admin/admin
 *    - If changed, check credentials
 *    - Try resetting credentials in code
 *
 * Network won't connect:
 *    - Check Ethernet cable
 *    - Verify network configuration in FRAM
 *    - Check serial monitor for error messages
 *
 * OTA update fails:
 *    - Ensure .bin file is valid
 *    - Check available flash space
 *    - Verify stable power supply
 *    - Try uploading via USB if OTA fails
 *
 * NEXT STEPS:
 * ===========
 *
 * Now that you have a basic web server running, you can:
 * - Explore other examples (custom_endpoints, rest_api, ota_update)
 * - Add custom web pages and APIs
 * - Integrate with your application logic
 * - Build a custom control interface
 *
 * See the documentation at docs_new/ESPMegaWebServer.md for complete
 * API reference and advanced usage examples.
 */
