#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Esp.h>
#include <time.h>

const char* ssid = "REDACTED";
const char* pass = "REDACTED";

AsyncWebServer server(80);

const IPAddress allowedIP(1, 1, 1, 1); // Replace with your trusted IP for status page access

unsigned long bootTime;

void connectWiFi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.printf("Wi-Fi Connected: %s\n", WiFi.localIP().toString().c_str());
  bootTime = millis();
}

void handleRoot(AsyncWebServerRequest* req) {
  String ipStr = req->client()->remoteIP().toString();
  String country = "UNK";
  String userAgent = "Unknown";

  for (size_t i = 0; i < req->headers(); i++) {
    const AsyncWebHeader* h = req->getHeader(i);
    if (h->name().equalsIgnoreCase("CF-Connecting-IP")) ipStr = h->value();
    if (h->name().equalsIgnoreCase("CF-IPCountry")) country = h->value();
    if (h->name().equalsIgnoreCase("User-Agent")) userAgent = h->value();
  }

  String response = "{";
  response += "\"ip\":\"" + ipStr + "\",";
  response += "\"country\":\"" + country + "\",";
  response += "\"user_agent\":\"" + userAgent + "\"";
  response += "}";

  req->send(200, "application/json", response);
}

void handleStatus(AsyncWebServerRequest* req) {
  String cfIP;
  for (size_t i = 0; i < req->headers(); i++) {
    const AsyncWebHeader* h = req->getHeader(i);
    if (h->name().equalsIgnoreCase("CF-Connecting-IP")) {
      cfIP = h->value();
      break;
    }
  }

  if (cfIP != allowedIP.toString()) {
    req->send(403, "application/json", "{\"error\":\"Forbidden\"}");
    return;
  }

  float voltage = analogRead(A0) * (3.3 / 1023.0);
  unsigned long uptimeSec = millis() / 1000;

  String json = "{";
  json += "\"chip_id\":" + String(ESP.getChipId()) + ",";
  json += "\"flash_chip_id\":" + String(ESP.getFlashChipId()) + ",";
  json += "\"flash_chip_size\":" + String(ESP.getFlashChipSize()) + ",";
  json += "\"flash_chip_speed\":" + String(ESP.getFlashChipSpeed()) + ",";
  json += "\"flash_chip_real_size\":" + String(ESP.getFlashChipRealSize()) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_fragmentation\":" + String(ESP.getHeapFragmentation()) + ",";
  json += "\"max_free_block_size\":" + String(ESP.getMaxFreeBlockSize()) + ",";
  json += "\"sketch_size\":" + String(ESP.getSketchSize()) + ",";
  json += "\"free_sketch_space\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"cycle_count\":" + String(ESP.getCycleCount()) + ",";
  json += "\"reset_reason\":\"" + String(ESP.getResetReason()) + "\",";
  json += "\"sdk_version\":\"" + String(ESP.getSdkVersion()) + "\",";
  json += "\"uptime_seconds\":" + String(uptimeSec) + ",";
  json += "\"analog_input_voltage\":" + String(voltage, 2);
  json += "}";

  req->send(200, "application/json", json);
}

void handleUI(AsyncWebServerRequest* req) {
  String html = "<html><head><title>IP Info</title><style>body{font-family:sans-serif;padding:2em;}pre{background:#f4f4f4;padding:1em;}</style></head><body>";
  html += "<h1>Requester Info</h1><pre id='ip'></pre><script>fetch('/')\n.then(r=>r.json()).then(j=>{document.getElementById('ip').textContent=JSON.stringify(j,null,2);});</script></body></html>";
  req->send(200, "text/html", html);
}

void handleStatusUI(AsyncWebServerRequest* req) {
  String html = "<html><head><title>Status</title><style>body{font-family:sans-serif;padding:2em;}pre{background:#f4f4f4;padding:1em;}</style></head><body>";
  html += "<h1>ESP Status</h1><pre id='status'></pre><script>fetch('/status')\n.then(r=>r.json()).then(j=>{document.getElementById('status').textContent=JSON.stringify(j,null,2);});</script></body></html>";
  req->send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/ui", HTTP_GET, handleUI);
  server.on("/status-ui", HTTP_GET, handleStatusUI);
  server.begin();
  Serial.println("Ready.");
}

void loop() { /* async server handles everything */ }
