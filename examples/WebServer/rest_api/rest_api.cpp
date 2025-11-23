/**
 * @file rest_api.ino
 * @brief RESTful API Example for ESPMegaWebServer
 *
 * This example demonstrates how to create a complete RESTful API for the
 * ESPMegaPRO board. It implements standard REST principles and HTTP methods.
 *
 * RESTful API Endpoints:
 * - GET    /api/inputs          - Get all input states
 * - GET    /api/inputs/:pin     - Get single input state
 * - GET    /api/outputs         - Get all output states
 * - GET    /api/outputs/:pin    - Get single output state
 * - POST   /api/outputs/:pin    - Set output state (JSON body)
 * - PUT    /api/outputs/:pin    - Update output state (JSON body)
 * - POST   /api/outputs/batch   - Set multiple outputs at once
 * - GET    /api/device          - Get device information
 * - GET    /api/system/uptime   - Get system uptime
 * - GET    /api/system/memory   - Get memory information
 * - POST   /api/system/reboot   - Reboot device
 *
 * REST Principles Demonstrated:
 * - Resource-based URLs
 * - Standard HTTP methods (GET, POST, PUT)
 * - JSON request/response format
 * - Proper HTTP status codes
 * - Idempotent operations
 * - Stateless communication
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board
 * - Ethernet connection
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

// API versioning
#define API_VERSION "1.0"

/**
 * GET /api/inputs
 * Returns all 16 input states
 *
 * Response: 200 OK
 * {
 *   "status": "success",
 *   "data": {
 *     "inputs": [
 *       {"pin": 0, "state": 0},
 *       {"pin": 1, "state": 1},
 *       ...
 *     ]
 *   }
 * }
 */
void apiGetInputs(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    StaticJsonDocument<768> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    JsonArray inputs = data.createNestedArray("inputs");

    for (int i = 0; i < 16; i++) {
        JsonObject input = inputs.createNestedObject();
        input["pin"] = i;
        input["state"] = espmega.inputs.digitalRead(i);
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * GET /api/inputs/:pin
 * Returns single input state
 *
 * Response: 200 OK
 * {
 *   "status": "success",
 *   "data": {
 *     "pin": 5,
 *     "state": 1
 *   }
 * }
 */
void apiGetInput(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    // Extract pin from URL path
    String pinStr = request->pathArg(0);
    int pin = pinStr.toInt();

    // Validate pin number
    if (pin < 0 || pin >= 16) {
        StaticJsonDocument<128> doc;
        doc["status"] = "error";
        doc["message"] = "Invalid pin number (0-15)";
        String response;
        serializeJson(doc, response);
        request->send(400, "application/json", response);
        return;
    }

    // Get input state
    int state = espmega.inputs.digitalRead(pin);

    // Build response
    StaticJsonDocument<256> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["pin"] = pin;
    data["state"] = state;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * GET /api/outputs
 * Returns all 16 output states
 */
void apiGetOutputs(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    StaticJsonDocument<768> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    JsonArray outputs = data.createNestedArray("outputs");

    for (int i = 0; i < 16; i++) {
        JsonObject output = outputs.createNestedObject();
        output["pin"] = i;
        output["state"] = espmega.outputs.digitalRead(i);
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * GET /api/outputs/:pin
 * Returns single output state
 */
void apiGetOutput(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    String pinStr = request->pathArg(0);
    int pin = pinStr.toInt();

    if (pin < 0 || pin >= 16) {
        StaticJsonDocument<128> doc;
        doc["status"] = "error";
        doc["message"] = "Invalid pin number (0-15)";
        String response;
        serializeJson(doc, response);
        request->send(400, "application/json", response);
        return;
    }

    int state = espmega.outputs.digitalRead(pin);

    StaticJsonDocument<256> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["pin"] = pin;
    data["state"] = state;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * POST /api/outputs/:pin
 * Sets output state
 *
 * Request body:
 * {
 *   "state": 1
 * }
 *
 * Response: 200 OK
 * {
 *   "status": "success",
 *   "data": {
 *     "pin": 5,
 *     "state": 1
 *   }
 * }
 */
void setupSetOutput() {
    AsyncWebServer* server = espmega.webServer->getServer();

    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler(
        "/api/outputs/*",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            if (!espmega.webServer->checkAuthentication(request)) {
                return;
            }

            // Extract pin from URL
            String path = request->url();
            int lastSlash = path.lastIndexOf('/');
            String pinStr = path.substring(lastSlash + 1);
            int pin = pinStr.toInt();

            if (pin < 0 || pin >= 16) {
                StaticJsonDocument<128> doc;
                doc["status"] = "error";
                doc["message"] = "Invalid pin number";
                String response;
                serializeJson(doc, response);
                request->send(400, "application/json", response);
                return;
            }

            JsonObject obj = json.as<JsonObject>();
            if (!obj.containsKey("state")) {
                StaticJsonDocument<128> doc;
                doc["status"] = "error";
                doc["message"] = "Missing 'state' field";
                String response;
                serializeJson(doc, response);
                request->send(400, "application/json", response);
                return;
            }

            int state = obj["state"].as<int>();
            espmega.outputs.digitalWrite(pin, state);

            StaticJsonDocument<256> doc;
            doc["status"] = "success";
            JsonObject data = doc.createNestedObject("data");
            data["pin"] = pin;
            data["state"] = state;
            data["message"] = "Output updated successfully";

            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);

            Serial.print("API: Output ");
            Serial.print(pin);
            Serial.print(" set to ");
            Serial.println(state);
        }
    );

    server->addHandler(handler);
}

/**
 * POST /api/outputs/batch
 * Set multiple outputs at once
 *
 * Request body:
 * {
 *   "outputs": [
 *     {"pin": 0, "state": 1},
 *     {"pin": 1, "state": 0},
 *     {"pin": 5, "state": 1}
 *   ]
 * }
 */
void setupBatchOutput() {
    AsyncWebServer* server = espmega.webServer->getServer();

    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler(
        "/api/outputs/batch",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            if (!espmega.webServer->checkAuthentication(request)) {
                return;
            }

            JsonObject obj = json.as<JsonObject>();
            if (!obj.containsKey("outputs")) {
                StaticJsonDocument<128> doc;
                doc["status"] = "error";
                doc["message"] = "Missing 'outputs' array";
                String response;
                serializeJson(doc, response);
                request->send(400, "application/json", response);
                return;
            }

            JsonArray outputs = obj["outputs"].as<JsonArray>();
            int successCount = 0;
            int errorCount = 0;

            for (JsonObject output : outputs) {
                if (output.containsKey("pin") && output.containsKey("state")) {
                    int pin = output["pin"].as<int>();
                    int state = output["state"].as<int>();

                    if (pin >= 0 && pin < 16) {
                        espmega.outputs.digitalWrite(pin, state);
                        successCount++;
                    } else {
                        errorCount++;
                    }
                } else {
                    errorCount++;
                }
            }

            StaticJsonDocument<256> doc;
            doc["status"] = errorCount == 0 ? "success" : "partial";
            JsonObject data = doc.createNestedObject("data");
            data["updated"] = successCount;
            data["errors"] = errorCount;

            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);

            Serial.print("API: Batch update - ");
            Serial.print(successCount);
            Serial.print(" success, ");
            Serial.print(errorCount);
            Serial.println(" errors");
        }
    );

    server->addHandler(handler);
}

/**
 * GET /api/device
 * Returns comprehensive device information
 */
void apiGetDevice(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    StaticJsonDocument<768> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");

    data["hostname"] = espmega.iot->getNetworkConfig()->hostname;
    data["ip_address"] = espmega.iot->getIp().toString();
    data["mac_address"] = espmega.iot->getMac();
    data["model"] = BOARD_MODEL;
    data["software_version"] = SW_VERSION;
    data["sdk_version"] = SDK_VESRION;
    data["idf_version"] = IDF_VER;
    data["api_version"] = API_VERSION;
    data["uptime"] = esp_timer_get_time() / 1000000;

    JsonObject network = data.createNestedObject("network");
    NetworkConfig* netconf = espmega.iot->getNetworkConfig();
    network["ip"] = netconf->ip.toString();
    network["gateway"] = netconf->gateway.toString();
    network["subnet"] = netconf->subnet.toString();

    JsonObject mqtt = data.createNestedObject("mqtt");
    MqttConfig* mqttconf = espmega.iot->getMqttConfig();
    mqtt["server"] = mqttconf->mqtt_server;
    mqtt["port"] = mqttconf->mqtt_port;
    mqtt["base_topic"] = mqttconf->base_topic;
    mqtt["connected"] = espmega.iot->mqttConnected();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * GET /api/system/uptime
 * Returns system uptime in seconds
 */
void apiGetUptime(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    uint64_t uptimeSeconds = esp_timer_get_time() / 1000000;

    StaticJsonDocument<256> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["uptime_seconds"] = uptimeSeconds;
    data["uptime_minutes"] = uptimeSeconds / 60;
    data["uptime_hours"] = uptimeSeconds / 3600;
    data["uptime_days"] = uptimeSeconds / 86400;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * GET /api/system/memory
 * Returns memory usage information
 */
void apiGetMemory(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    StaticJsonDocument<384> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");

    data["free_heap"] = ESP.getFreeHeap();
    data["total_heap"] = ESP.getHeapSize();
    data["used_heap"] = ESP.getHeapSize() - ESP.getFreeHeap();
    data["min_free_heap"] = ESP.getMinFreeHeap();
    data["max_alloc_heap"] = ESP.getMaxAllocHeap();

    float usedPercent = ((float)(ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) * 100.0;
    data["used_percent"] = usedPercent;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * POST /api/system/reboot
 * Reboots the device
 */
void apiReboot(AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    StaticJsonDocument<256> doc;
    doc["status"] = "success";
    JsonObject data = doc.createNestedObject("data");
    data["message"] = "Device will reboot in 2 seconds";

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);

    Serial.println("API: Reboot requested");

    // Delay before reboot to ensure response is sent
    delay(2000);
    ESP.restart();
}

/**
 * API Documentation Page
 */
const char api_docs_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESPMega REST API Documentation</title>
    <style>
        body { font-family: monospace; max-width: 1000px; margin: 20px auto; padding: 20px; background: #1e1e1e; color: #d4d4d4; }
        h1 { color: #4ec9b0; }
        h2 { color: #dcdcaa; margin-top: 30px; }
        h3 { color: #9cdcfe; }
        .endpoint { background: #2d2d2d; padding: 15px; margin: 15px 0; border-left: 3px solid #4ec9b0; }
        .method { display: inline-block; padding: 3px 8px; border-radius: 3px; font-weight: bold; margin-right: 10px; }
        .get { background: #4ec9b0; color: #1e1e1e; }
        .post { background: #dcdcaa; color: #1e1e1e; }
        .put { background: #569cd6; color: #1e1e1e; }
        pre { background: #1e1e1e; padding: 10px; border: 1px solid #3e3e3e; overflow-x: auto; }
        code { color: #ce9178; }
        a { color: #569cd6; }
    </style>
</head>
<body>
    <h1>ESPMega PRO REST API Documentation</h1>
    <p>API Version: 1.0</p>
    <p>All endpoints require HTTP Basic Authentication</p>

    <h2>Input Endpoints</h2>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/inputs</h3>
        <p>Get all 16 input states</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/inputs</code></pre>
    </div>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/inputs/:pin</h3>
        <p>Get single input state (pin 0-15)</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/inputs/5</code></pre>
    </div>

    <h2>Output Endpoints</h2>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/outputs</h3>
        <p>Get all 16 output states</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/outputs</code></pre>
    </div>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/outputs/:pin</h3>
        <p>Get single output state (pin 0-15)</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/outputs/5</code></pre>
    </div>

    <div class="endpoint">
        <h3><span class="method post">POST</span>/api/outputs/:pin</h3>
        <p>Set output state (pin 0-15)</p>
        <pre><code>curl -u admin:admin -X POST -H "Content-Type: application/json" \
     -d '{"state":1}' http://&lt;ip&gt;/api/outputs/5</code></pre>
    </div>

    <div class="endpoint">
        <h3><span class="method post">POST</span>/api/outputs/batch</h3>
        <p>Set multiple outputs at once</p>
        <pre><code>curl -u admin:admin -X POST -H "Content-Type: application/json" \
     -d '{"outputs":[{"pin":0,"state":1},{"pin":1,"state":0}]}' \
     http://&lt;ip&gt;/api/outputs/batch</code></pre>
    </div>

    <h2>Device Endpoints</h2>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/device</h3>
        <p>Get comprehensive device information</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/device</code></pre>
    </div>

    <h2>System Endpoints</h2>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/system/uptime</h3>
        <p>Get system uptime</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/system/uptime</code></pre>
    </div>

    <div class="endpoint">
        <h3><span class="method get">GET</span>/api/system/memory</h3>
        <p>Get memory usage information</p>
        <pre><code>curl -u admin:admin http://&lt;ip&gt;/api/system/memory</code></pre>
    </div>

    <div class="endpoint">
        <h3><span class="method post">POST</span>/api/system/reboot</h3>
        <p>Reboot the device</p>
        <pre><code>curl -u admin:admin -X POST http://&lt;ip&gt;/api/system/reboot</code></pre>
    </div>

    <p style="margin-top: 50px; text-align: center;">
        <a href="/">Home</a> | <a href="/control">Control Panel</a> | <a href="/config">Configuration</a>
    </p>
</body>
</html>
)rawliteral";

void setupRestAPI() {
    AsyncWebServer* server = espmega.webServer->getServer();

    // Input endpoints
    server->on("/api/inputs", HTTP_GET, apiGetInputs);
    server->on("^\\/api\\/inputs\\/([0-9]+)$", HTTP_GET, apiGetInput);

    // Output endpoints
    server->on("/api/outputs", HTTP_GET, apiGetOutputs);
    server->on("^\\/api\\/outputs\\/([0-9]+)$", HTTP_GET, apiGetOutput);

    // Device endpoints
    server->on("/api/device", HTTP_GET, apiGetDevice);

    // System endpoints
    server->on("/api/system/uptime", HTTP_GET, apiGetUptime);
    server->on("/api/system/memory", HTTP_GET, apiGetMemory);
    server->on("/api/system/reboot", HTTP_POST, apiReboot);

    // API documentation
    server->on("/api", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }
        request->send_P(200, "text/html", api_docs_html);
    });

    // Setup JSON handlers
    setupSetOutput();
    setupBatchOutput();

    Serial.println("\nREST API endpoints registered:");
    Serial.println("  GET  /api                  - API documentation");
    Serial.println("  GET  /api/inputs           - Get all inputs");
    Serial.println("  GET  /api/inputs/:pin      - Get single input");
    Serial.println("  GET  /api/outputs          - Get all outputs");
    Serial.println("  GET  /api/outputs/:pin     - Get single output");
    Serial.println("  POST /api/outputs/:pin     - Set output");
    Serial.println("  POST /api/outputs/batch    - Set multiple outputs");
    Serial.println("  GET  /api/device           - Get device info");
    Serial.println("  GET  /api/system/uptime    - Get uptime");
    Serial.println("  GET  /api/system/memory    - Get memory info");
    Serial.println("  POST /api/system/reboot    - Reboot device");
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESPMegaPRO RESTful API Example");
    Serial.println("==============================");

    // Initialize ESPMegaPRO
    if (!espmega.begin()) {
        Serial.println("ERROR: Failed to initialize ESPMegaPRO!");
        while (1) delay(1000);
    }

    // Enable IoT module
    espmega.enableIotModule();

    // Wait for network
    Serial.println("Waiting for network connection...");
    while (!espmega.iot->networkConnected()) {
        delay(100);
    }
    Serial.println("Network connected!");

    // Enable web server
    espmega.enableWebServer(WEB_SERVER_PORT);

    // Setup REST API
    setupRestAPI();

    // Display access information
    Serial.println("\n==============================");
    Serial.println("REST API Ready!");
    Serial.println("==============================");
    Serial.print("API Documentation: http://");
    Serial.print(espmega.iot->getIp());
    Serial.println("/api");
    Serial.print("API Base URL: http://");
    Serial.print(espmega.iot->getIp());
    Serial.println("/api");
    Serial.println("API Version: " API_VERSION);
    Serial.println("==============================\n");
}

void loop() {
    espmega.loop();
}

/**
 * TESTING THE REST API:
 * =====================
 *
 * 1. VIEW API DOCUMENTATION:
 *    Open http://<ip>/api in your browser
 *
 * 2. CURL EXAMPLES:
 *    # Get all inputs
 *    curl -u admin:admin http://<ip>/api/inputs
 *
 *    # Get input 5
 *    curl -u admin:admin http://<ip>/api/inputs/5
 *
 *    # Get all outputs
 *    curl -u admin:admin http://<ip>/api/outputs
 *
 *    # Set output 5 to HIGH
 *    curl -u admin:admin -X POST -H "Content-Type: application/json" \
 *         -d '{"state":1}' http://<ip>/api/outputs/5
 *
 *    # Batch update
 *    curl -u admin:admin -X POST -H "Content-Type: application/json" \
 *         -d '{"outputs":[{"pin":0,"state":1},{"pin":1,"state":0},{"pin":2,"state":1}]}' \
 *         http://<ip>/api/outputs/batch
 *
 *    # Get device info
 *    curl -u admin:admin http://<ip>/api/device
 *
 *    # Get uptime
 *    curl -u admin:admin http://<ip>/api/system/uptime
 *
 *    # Get memory info
 *    curl -u admin:admin http://<ip>/api/system/memory
 *
 * 3. PYTHON EXAMPLE:
 *    import requests
 *    from requests.auth import HTTPBasicAuth
 *
 *    auth = HTTPBasicAuth('admin', 'admin')
 *    base_url = 'http://192.168.1.100/api'
 *
 *    # Get all inputs
 *    r = requests.get(f'{base_url}/inputs', auth=auth)
 *    print(r.json())
 *
 *    # Set output
 *    r = requests.post(f'{base_url}/outputs/5',
 *                      json={'state': 1},
 *                      auth=auth)
 *    print(r.json())
 *
 * 4. JAVASCRIPT/NODE.JS EXAMPLE:
 *    const axios = require('axios');
 *
 *    const client = axios.create({
 *        baseURL: 'http://192.168.1.100/api',
 *        auth: { username: 'admin', password: 'admin' }
 *    });
 *
 *    // Get all inputs
 *    client.get('/inputs')
 *        .then(r => console.log(r.data));
 *
 *    // Set output
 *    client.post('/outputs/5', { state: 1 })
 *        .then(r => console.log(r.data));
 */
