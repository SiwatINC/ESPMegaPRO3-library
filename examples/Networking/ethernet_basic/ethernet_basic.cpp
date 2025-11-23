/**
 * ESPMegaPRO Ethernet Basic Example
 *
 * This example demonstrates how to connect the ESPMegaPRO to a network using Ethernet.
 * It shows both static IP and DHCP configuration methods.
 *
 * Features demonstrated:
 * - Ethernet connection setup
 * - Static IP configuration
 * - DHCP configuration (commented alternative)
 * - Network status monitoring
 * - IP and MAC address retrieval
 *
 * Hardware Requirements:
 * - ESPMegaPRO R3 board
 * - Ethernet cable connected to network
 *
 * Created: 2024
 * Author: SIWAT SYSTEM
 */

#include <ESPMegaPRO.h>
#include <ETH.h>

// Create ESPMegaPRO instance
ESPMegaPRO espmega;

// Network configuration variables
const char* hostname = "espmega-eth-01";

// Static IP configuration
const IPAddress static_ip(192, 168, 1, 100);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns1(1, 1, 1, 1);      // Cloudflare DNS
const IPAddress dns2(8, 8, 8, 8);      // Google DNS

// Connection tracking
unsigned long lastStatusCheck = 0;
const unsigned long STATUS_CHECK_INTERVAL = 5000;  // Check every 5 seconds

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.println("ESPMegaPRO Ethernet Basic Example");
    Serial.println("========================================");
    Serial.println();

    // Initialize ESPMegaPRO
    Serial.println("[INIT] Initializing ESPMegaPRO...");
    espmega.begin();
    Serial.println("[INIT] ESPMegaPRO initialized successfully");
    Serial.println();

    // Configure network settings
    Serial.println("[NETWORK] Configuring network settings...");
    NetworkConfig netConfig;

    // Method 1: Static IP Configuration (Recommended for servers)
    Serial.println("[NETWORK] Using Static IP configuration");
    netConfig.ip = static_ip;
    netConfig.gateway = gateway;
    netConfig.subnet = subnet;
    netConfig.dns1 = dns1;
    netConfig.dns2 = dns2;
    strcpy(netConfig.hostname, hostname);
    netConfig.useStaticIp = true;   // Enable static IP
    netConfig.useWifi = false;      // Use Ethernet (not WiFi)

    /* Method 2: DHCP Configuration (Alternative - comment out Method 1 and uncomment this)
    Serial.println("[NETWORK] Using DHCP configuration");
    strcpy(netConfig.hostname, hostname);
    netConfig.useStaticIp = false;  // Use DHCP
    netConfig.useWifi = false;      // Use Ethernet (not WiFi)
    */

    // Apply network configuration
    espmega.iot.setNetworkConfig(netConfig);

    // Bind Ethernet interface
    espmega.iot.bindEthernetInterface(&ETH);
    Serial.println("[NETWORK] Ethernet interface bound");

    // Connect to network
    Serial.println("[NETWORK] Connecting to network...");
    espmega.iot.connectNetwork();

    // Wait for network connection
    Serial.print("[NETWORK] Waiting for connection");
    unsigned long startTime = millis();
    const unsigned long TIMEOUT = 30000;  // 30 second timeout

    while (!espmega.iot.networkConnected()) {
        delay(500);
        Serial.print(".");

        // Check for timeout
        if (millis() - startTime > TIMEOUT) {
            Serial.println();
            Serial.println("[ERROR] Network connection timeout!");
            Serial.println("[ERROR] Please check:");
            Serial.println("  - Ethernet cable is connected");
            Serial.println("  - Network switch/router is powered on");
            Serial.println("  - Static IP settings are correct");
            Serial.println();
            Serial.println("[INFO] System will continue, but network is unavailable");
            return;
        }
    }

    Serial.println();
    Serial.println("[SUCCESS] Network connected!");
    Serial.println();

    // Display connection information
    printNetworkInfo();

    Serial.println();
    Serial.println("========================================");
    Serial.println("Setup Complete - Network is Ready");
    Serial.println("========================================");
    Serial.println();
}

void loop() {
    // Update ESPMegaPRO (handles network operations)
    espmega.loop();

    // Periodically check and display network status
    if (millis() - lastStatusCheck > STATUS_CHECK_INTERVAL) {
        lastStatusCheck = millis();

        if (espmega.iot.networkConnected()) {
            Serial.println("[STATUS] Network: CONNECTED");

            // You can add periodic status updates here
            // For example, print uptime
            unsigned long uptime = millis() / 1000;
            Serial.print("[INFO] Uptime: ");
            Serial.print(uptime);
            Serial.println(" seconds");
        } else {
            Serial.println("[WARNING] Network: DISCONNECTED");
            Serial.println("[INFO] Check Ethernet cable connection");
        }

        Serial.println();
    }

    // Your application code here
    // The network is available for other operations
    // such as HTTP requests, MQTT, etc.
}

/**
 * Print detailed network information to serial console
 */
void printNetworkInfo() {
    Serial.println("Network Information:");
    Serial.println("-------------------");

    // IP Address
    Serial.print("IP Address:   ");
    Serial.println(espmega.iot.getETHIp());

    // MAC Address
    Serial.print("MAC Address:  ");
    Serial.println(espmega.iot.getETHMac());

    // Configuration details
    NetworkConfig* config = espmega.iot.getNetworkConfig();

    Serial.print("Hostname:     ");
    Serial.println(config->hostname);

    Serial.print("Gateway:      ");
    Serial.println(config->gateway);

    Serial.print("Subnet:       ");
    Serial.println(config->subnet);

    Serial.print("DNS1:         ");
    Serial.println(config->dns1);

    Serial.print("DNS2:         ");
    Serial.println(config->dns2);

    Serial.print("IP Mode:      ");
    Serial.println(config->useStaticIp ? "Static IP" : "DHCP");
}

/**
 * Additional Helper Functions
 */

// Check if network is available
bool isNetworkAvailable() {
    return espmega.iot.networkConnected();
}

// Get current IP address
IPAddress getCurrentIP() {
    return espmega.iot.getETHIp();
}

// Get MAC address as string
String getMACAddress() {
    return espmega.iot.getETHMac();
}
