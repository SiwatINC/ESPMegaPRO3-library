/**
 * ESPMegaPRO WiFi Basic Example
 *
 * This example demonstrates how to connect the ESPMegaPRO to a network using WiFi.
 * It shows both password-protected and open network connections.
 *
 * Features demonstrated:
 * - WiFi connection with password
 * - WiFi connection without password (open network)
 * - Static IP configuration
 * - DHCP configuration
 * - WiFi status monitoring
 * - Signal strength (RSSI) monitoring
 *
 * Hardware Requirements:
 * - ESPMegaPRO board with WiFi capability
 *
 * Configuration:
 * - Update WIFI_SSID and WIFI_PASSWORD with your network credentials
 * - Choose static IP or DHCP by commenting/uncommenting sections
 *
 * Created: 2024
 * Author: SIWAT SYSTEM
 */

#include <ESPMegaPRO.h>
#include <WiFi.h>

// Create ESPMegaPRO instance
ESPMegaPRO espmega;

// ===== WIFI CONFIGURATION =====
// Update these with your WiFi credentials
const char* WIFI_SSID = "YourWiFiSSID";           // Change this to your WiFi SSID
const char* WIFI_PASSWORD = "YourPassword";       // Change this to your WiFi password
const bool WIFI_USE_PASSWORD = true;             // Set to false for open networks

// Device configuration
const char* hostname = "espmega-wifi-01";

// Static IP configuration (optional)
const IPAddress static_ip(192, 168, 1, 101);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns1(1, 1, 1, 1);      // Cloudflare DNS
const IPAddress dns2(8, 8, 8, 8);      // Google DNS

// Choose IP configuration method
const bool USE_STATIC_IP = false;      // Set to true for static IP, false for DHCP

// Connection tracking
unsigned long lastStatusCheck = 0;
const unsigned long STATUS_CHECK_INTERVAL = 10000;  // Check every 10 seconds

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.println("ESPMegaPRO WiFi Basic Example");
    Serial.println("========================================");
    Serial.println();

    // Initialize ESPMegaPRO
    Serial.println("[INIT] Initializing ESPMegaPRO...");
    espmega.begin();
    Serial.println("[INIT] ESPMegaPRO initialized successfully");
    Serial.println();

    // Configure network settings
    Serial.println("[WIFI] Configuring WiFi settings...");
    NetworkConfig netConfig;

    // Basic WiFi configuration
    strcpy(netConfig.hostname, hostname);
    strcpy(netConfig.ssid, WIFI_SSID);
    strcpy(netConfig.password, WIFI_PASSWORD);
    netConfig.wifiUseAuth = WIFI_USE_PASSWORD;
    netConfig.useWifi = true;      // Use WiFi (not Ethernet)

    // IP configuration
    if (USE_STATIC_IP) {
        Serial.println("[WIFI] Using Static IP configuration");
        netConfig.ip = static_ip;
        netConfig.gateway = gateway;
        netConfig.subnet = subnet;
        netConfig.dns1 = dns1;
        netConfig.dns2 = dns2;
        netConfig.useStaticIp = true;
    } else {
        Serial.println("[WIFI] Using DHCP configuration");
        netConfig.useStaticIp = false;
    }

    // Apply network configuration
    espmega.iot.setNetworkConfig(netConfig);

    // Display connection attempt info
    Serial.println();
    Serial.println("Connection Details:");
    Serial.print("  SSID:     ");
    Serial.println(WIFI_SSID);
    Serial.print("  Security: ");
    Serial.println(WIFI_USE_PASSWORD ? "WPA/WPA2" : "Open");
    Serial.print("  Hostname: ");
    Serial.println(hostname);
    Serial.print("  IP Mode:  ");
    Serial.println(USE_STATIC_IP ? "Static" : "DHCP");
    Serial.println();

    // Connect to WiFi network
    Serial.println("[WIFI] Connecting to WiFi...");
    espmega.iot.connectNetwork();

    // Wait for WiFi connection
    Serial.print("[WIFI] Waiting for connection");
    unsigned long startTime = millis();
    const unsigned long TIMEOUT = 30000;  // 30 second timeout

    while (!espmega.iot.wifiConnected()) {
        delay(500);
        Serial.print(".");

        // Check for timeout
        if (millis() - startTime > TIMEOUT) {
            Serial.println();
            Serial.println("[ERROR] WiFi connection timeout!");
            Serial.println("[ERROR] Please check:");
            Serial.println("  - SSID is correct");
            Serial.println("  - Password is correct (if using security)");
            Serial.println("  - WiFi router is powered on and in range");
            Serial.println("  - WiFi signal strength is adequate");
            Serial.println();
            Serial.println("[INFO] System will continue, but WiFi is unavailable");
            return;
        }
    }

    Serial.println();
    Serial.println("[SUCCESS] WiFi connected!");
    Serial.println();

    // Display connection information
    printWiFiInfo();

    Serial.println();
    Serial.println("========================================");
    Serial.println("Setup Complete - WiFi is Ready");
    Serial.println("========================================");
    Serial.println();
}

void loop() {
    // Update ESPMegaPRO (handles network operations)
    espmega.loop();

    // Periodically check and display WiFi status
    if (millis() - lastStatusCheck > STATUS_CHECK_INTERVAL) {
        lastStatusCheck = millis();

        if (espmega.iot.wifiConnected()) {
            Serial.println("[STATUS] WiFi: CONNECTED");

            // Display signal strength
            int rssi = WiFi.RSSI();
            Serial.print("[INFO] Signal Strength: ");
            Serial.print(rssi);
            Serial.print(" dBm (");
            Serial.print(getSignalQuality(rssi));
            Serial.println(")");

            // Display uptime
            unsigned long uptime = millis() / 1000;
            Serial.print("[INFO] Uptime: ");
            Serial.print(uptime);
            Serial.println(" seconds");
        } else {
            Serial.println("[WARNING] WiFi: DISCONNECTED");
            Serial.println("[INFO] Attempting to reconnect...");
            // The ESPMegaIoT will handle reconnection automatically
        }

        Serial.println();
    }

    // Your application code here
    // The WiFi connection is available for other operations
}

/**
 * Print detailed WiFi information to serial console
 */
void printWiFiInfo() {
    Serial.println("WiFi Information:");
    Serial.println("----------------");

    // SSID
    Serial.print("SSID:         ");
    Serial.println(WiFi.SSID());

    // IP Address
    Serial.print("IP Address:   ");
    Serial.println(espmega.iot.getWifiIp());

    // MAC Address
    Serial.print("MAC Address:  ");
    Serial.println(espmega.iot.getWifiMac());

    // Signal Strength
    int rssi = WiFi.RSSI();
    Serial.print("RSSI:         ");
    Serial.print(rssi);
    Serial.print(" dBm (");
    Serial.print(getSignalQuality(rssi));
    Serial.println(")");

    // Configuration details
    NetworkConfig* config = espmega.iot.getNetworkConfig();

    Serial.print("Hostname:     ");
    Serial.println(config->hostname);

    if (config->useStaticIp) {
        Serial.print("Gateway:      ");
        Serial.println(config->gateway);

        Serial.print("Subnet:       ");
        Serial.println(config->subnet);

        Serial.print("DNS1:         ");
        Serial.println(config->dns1);

        Serial.print("DNS2:         ");
        Serial.println(config->dns2);
    }

    Serial.print("IP Mode:      ");
    Serial.println(config->useStaticIp ? "Static IP" : "DHCP");
}

/**
 * Convert RSSI to human-readable signal quality
 *
 * @param rssi Signal strength in dBm
 * @return String describing signal quality
 */
String getSignalQuality(int rssi) {
    if (rssi >= -50) {
        return "Excellent";
    } else if (rssi >= -60) {
        return "Good";
    } else if (rssi >= -70) {
        return "Fair";
    } else if (rssi >= -80) {
        return "Weak";
    } else {
        return "Very Weak";
    }
}

/**
 * Get signal strength as percentage (0-100)
 *
 * @param rssi Signal strength in dBm
 * @return Signal strength as percentage
 */
int getSignalPercent(int rssi) {
    // Convert RSSI to percentage (approximation)
    // -30 dBm = 100%, -90 dBm = 0%
    int percent = map(rssi, -90, -30, 0, 100);
    return constrain(percent, 0, 100);
}

/**
 * Additional Helper Functions
 */

// Check if WiFi is available
bool isWiFiAvailable() {
    return espmega.iot.wifiConnected();
}

// Get current IP address
IPAddress getCurrentIP() {
    return espmega.iot.getWifiIp();
}

// Get MAC address as string
String getMACAddress() {
    return espmega.iot.getWifiMac();
}

// Reconnect WiFi (manual trigger)
void reconnectWiFi() {
    Serial.println("[WIFI] Manual reconnection triggered...");
    espmega.iot.disconnectFromWifi();
    delay(1000);
    espmega.iot.connectNetwork();
}
