#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512

// AP Server network credentials
const char* ssid = "NodeMCUServer";
const char* password = "p@55w0rd";
const char* http_username = "admin";
const char* http_password = "admin";

// WiFi Router parameters to be configured
String wifi_name;
String wifi_pwd;

// API data
String API_SERVER_IP;
String API_URI;
String API_KEY;
String TARGET_DEVICE_ID;
String TARGET_SENSOR_ID;

// EEPROM addresses (Extra 1 byte for store length)
const int addr_is_configured = 0;       // 1 byte
const int addr_wifi_name = 4;           // 32 bytes + 1 byte
const int addr_wifi_pwd = 37;           // 32 bytes + 1 byte
const int addr_api_server_ip = 70;      // 64 bytes + 1 byte
const int addr_api_uri = 135;           // 64 bytes + 1 byte
const int addr_api_key = 200;           // 256 bytes + 1 byte
const int addr_target_device_id = 457;  // 8 bytes + 1 byte
const int addr_target_sensor_id = 466;  // 8 bytes + 1 byte

// function protypes
void WriteStringToEEPROM(int addrOffset, const String &strToWrite);
String ReadStringFromEEPROM(int addrOffset);
void InitConfigData();
void ClearEEPROM();

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Main web page HTML
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html style="background: #f2f2f2;">
<head>
  <title>Arduino WiFi Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.6rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 10px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .input-text{display:flex;align-items:left;padding:.375rem .75rem;font-size:1rem;font-weight:400;line-height:1.5;color:#212529;text-align:left;white-space:nowrap;background-color:#fff;border:1px solid #007ffd;border-radius:.25rem}
    .input-text:focus{outline: none; box-shadow: 0px 0px 3px #007ffd;;}
    td{padding:5px;}
    .btn{display:inline-block;font-weight:400;line-height:1.5;color:#212529;text-align:center;text-decoration:none;vertical-align:middle;cursor:pointer;-webkit-user-select:none;-moz-user-select:none;user-select:none;background-color:transparent;border:1px solid transparent;padding:.375rem .75rem;font-size:1rem;border-radius:.25rem;transition:color .15s ease-in-out,background-color .15s ease-in-out,border-color .15s ease-in-out,box-shadow .15s ease-in-out}
    .btn:hover{color:#212529}
    .btn-primary{color:#fff;background-color:#0d6efd;border-color:#0d6efd}.btn-primary:hover{color:#fff;background-color:#0b5ed7;border-color:#0a58ca}
    .btn-warning{color:#fff;background-color:orange;border-color:orange}.btn-warning:hover{color:#fff;background-color:#ff8c00;border-color:#ff8c00}
    .btn-danger{color:#fff;background-color:#dc3545;border-color:#dc3545}.btn-danger:hover{color:#fff;background-color:#bb2d3b;border-color:#b02a37}
    .alert{position:relative;padding:1rem 1rem;margin-bottom:1rem;border:1px solid transparent;border-radius:.25rem}
    .alert-success{color:#0f5132;background-color:#d1e7dd;border-color:#badbcc}
    .alert-danger{color:#842029;background-color:#f8d7da;border-color:#f5c2c7}
    @-webkit-keyframes spinner-border{to{transform:rotate(360deg)}}@keyframes spinner-border{to{transform:rotate(360deg)}}.spinner-border{display:inline-block;width:2rem;height:2rem;border:.25em solid currentColor;border-right-color:transparent;border-radius:50px;-webkit-animation:.75s linear infinite spinner-border;animation:.75s linear infinite spinner-border}.spinner-border-sm{width:1rem;height:1rem;border-width:.2em}
    span{vertical-align:middle;}
  </style>
</head>
<body>
  <h1 style="text-transform: uppercase;">Arduino WiFi Server</h2>
  <h3>Configuration</h4>
  <center>
  <table style="border: none;" >
        <tr>
            <td style="text-align: right;">WiFi Name</td>
            <td style="text-align: left;"><input id="txtWifiName" class="input-text" type="text" value="%WIFI_NAME%" maxlength="32" autofocus /></td>
        </tr>
        <tr>
            <td style="text-align: right;">WiFi Password</td>
            <td style="text-align: left;"><input id="txtWifiPwd" class="input-text" type="password" value="%WIFI_PWD%" maxlength="32" /></td>
        </tr>
        <tr>
            <td style="text-align: right;">API Server IP</td>
            <td style="text-align: left;"><input id="txtApiServerIp" class="input-text" type="text" value="%API_IP_ADDR%" maxlength="64" /></td>
        </tr>
        <tr>
            <td style="text-align: right;">API URI</td>
            <td style="text-align: left;"><input id="txtApiUri" class="input-text" type="text" value="%API_URI%" style="width: 300px;" maxlength="64" /></td>
        </tr>
        <tr>
            <td style="text-align: right;">API Key</td>
            <td style="text-align: left;"><textarea id="txtApiKey" class="input-text" type="text" style="width: 400px; height: 45px;" maxlength="255" >%API_KEY%</textarea></td>
        </tr>
        <tr>
            <td style="text-align: right;">Target Device ID</td>
            <td style="text-align: left;"><input id="txtTargetDeviceId" class="input-text" type="number" value="%TGT_DEVICE_ID%" min="0" max="99999" onKeyPress="if(this.value.length==5) return false;" /></td>
        </tr>
        <tr>
            <td style="text-align: right;">Target Sensor ID</td>
            <td style="text-align: left;"><input id="txtTargetSensorId" class="input-text" type="number" value="%TGT_SENSOR_ID%" min="0" max="99999" onKeyPress="if(this.value.length==5) return false;" /></td>
        </tr>
  </table>
  </center>
  <div style="margin-top: 5px;">
    <button class="btn btn-primary" style="width: 95px;" onclick="Save()">
        <span id="spanSaveSpinner" class="spinner-border spinner-border-sm" role="status" style="display: none;"></span>
        <span>Save</span>
    </button>
    <button class="btn btn-warning" style="width: 95px;" onclick="Clear()">
        <span id="spanClearSpinner" class="spinner-border spinner-border-sm" role="status" style="display: none;"></span>
        <span>Clear</span>
    </button>
    <button class="btn btn-danger" style="width: 95px;" onclick="logoutButton()">Logout</button>
  </div>
  <div id="divAlertArea" class="alert alert-success" role="alert" style="margin-top: 10px; opacity: 0;"></div>
<script>

    function logoutButton() {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/logout", true);
        xhr.send();
        setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
    }

    function Save(){
        if(ValidateForSave()){
            document.getElementById("spanSaveSpinner").style.display = "inline-block";

            var txtWifiName = document.getElementById("txtWifiName");
            var txtWifiPwd = document.getElementById("txtWifiPwd");
            var txtApiServerIp = document.getElementById("txtApiServerIp");
            var txtApiUri = document.getElementById("txtApiUri");
            var txtApiKey = document.getElementById("txtApiKey");
            var txtTargetDeviceId = document.getElementById("txtTargetDeviceId");
            var txtTargetSensorId = document.getElementById("txtTargetSensorId");

            var url = "/save-config?WifiName=" + txtWifiName.value;
            url += "&WifiPwd=" + txtWifiPwd.value;
            url += "&ApiServerIp=" + txtApiServerIp.value;
            url += "&ApiUri=" + txtApiUri.value;
            url += "&ApiKey=" + txtApiKey.value;
            url += "&TargetDeviceId=" + txtTargetDeviceId.value;
            url += "&TargetSensorId=" + txtTargetSensorId.value;
            
            const xhr = new XMLHttpRequest();
            xhr.timeout = 15000;
            xhr.onreadystatechange = function(e) {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        ShowAlert("Configuration saved successfully.", "SUCCESS");
                        txtWifiName.focus();
                    } else {
                        ShowAlert("Failed to save configuration.", "ERROR");
                    }
                    document.getElementById("spanSaveSpinner").style.display = "none";
                }
            }
            xhr.ontimeout = function () {
                ShowAlert("Timeout occurred.", "ERROR");
                document.getElementById("spanSaveSpinner").style.display = "none";
            }
            xhr.open('GET', url, true)
            xhr.send();
        }
    }

    function Clear(){
        if (confirm('WARNING\nThis will clear all stored data in the device. Are you sure?')) {
            document.getElementById("spanClearSpinner").style.display = "inline-block";
            var xhr = new XMLHttpRequest();
            xhr.timeout = 15000;
            xhr.onreadystatechange = function(e) {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        ShowAlert("Configuration cleared successfully.", "SUCCESS");
                        ClearUIData();
                        document.getElementById("txtWifiName").focus();
                    } else {
                        ShowAlert("Failed to clear configuration.", "ERROR");
                    }
                    document.getElementById("spanClearSpinner").style.display = "none";
                }
            }
            xhr.ontimeout = function () {
                ShowAlert("Timeout occurred.", "ERROR");
                document.getElementById("spanClearSpinner").style.display = "none";
            }
            xhr.open("GET", "/clear-config", true); 
            xhr.send();
        }
    }

    function ClearUIData(){
        document.getElementById("txtWifiName").value = "";
        document.getElementById("txtWifiPwd").value = "";
        document.getElementById("txtApiServerIp").value = "";
        document.getElementById("txtApiUri").value = "";
        document.getElementById("txtApiKey").value = "";
        document.getElementById("txtTargetDeviceId").value = "";
        document.getElementById("txtTargetSensorId").value = "";
    }

    function ValidateForSave(){var e=document.getElementById("txtWifiName"),t=document.getElementById("txtWifiPwd"),r=document.getElementById("txtApiServerIp"),l=document.getElementById("txtApiUri"),n=document.getElementById("txtApiKey"),a=document.getElementById("txtTargetDeviceId"),o=document.getElementById("txtTargetSensorId");return""===e.value?(ShowAlert("Please enter WiFi name.","ERROR"),e.focus(),!1):""===t.value?(ShowAlert("Please enter WiFi password.","ERROR"),t.focus(),!1):""===r.value?(ShowAlert("Please enter API Server IP address.","ERROR"),r.focus(),!1):""===l.value?(ShowAlert("Please enter API URI.","ERROR"),l.focus(),!1):""===n.value?(ShowAlert("Please enter API KEY.","ERROR"),n.focus(),!1):""===a.value?(ShowAlert("Please enter the target device ID.","ERROR"),a.focus(),!1):""!==o.value||(ShowAlert("Please enter the target sensor ID.","ERROR"),o.focus(),!1)}

    function ShowAlert(e,a){var r=document.getElementById("divAlertArea");"SUCCESS"==a&&(r.className="alert alert-success"),"ERROR"==a&&(r.className="alert alert-danger"),r.innerHTML=e,fadeIn(r,1e3)}

    function fadeIn(e,t){e.style.opacity=0;var a=+new Date,n=function(){e.style.opacity=+e.style.opacity+(new Date-a)/t,a=+new Date,+e.style.opacity<1&&(window.requestAnimationFrame&&requestAnimationFrame(n)||setTimeout(n,16))};n()}
</script>
</body>
<footer>
    <p style="font-size: xx-small; right: 10px; bottom: 0; position: fixed;">&copy; USHI Tech. 2021</p>
</footer>
</html>
)rawliteral";

// Logout page HTML
const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <p>Logged out or <a href="/">return to homepage</a>.</p>
</body>
</html>
)rawliteral";

// Replaces placeholder with initial data in your web page
String processor(const String& var){
  if(var == "WIFI_NAME"){
    return wifi_name;
  }
  if(var == "WIFI_PWD"){
    return wifi_pwd;
  }
  if(var == "API_IP_ADDR"){
    return API_SERVER_IP;
  }
  if(var == "API_URI"){
    return API_URI;
  }
  if(var == "API_KEY"){
    return API_KEY;
  }
  if(var == "TGT_DEVICE_ID"){
    return TARGET_DEVICE_ID;
  }
  if(var == "TGT_SENSOR_ID"){
    return TARGET_SENSOR_ID;
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Begin EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // If not configured, clear EEPROM data
  int isConfigured = EEPROM.read(addr_is_configured);
  if(isConfigured != 1){
    ClearEEPROM();
    Serial.println("EEPROM Initialized.");
  }

  // Initialize configuration data from EEPROM
  InitConfigData();  
  
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
  });
    
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", logout_html, processor);
  });

  // Route for save-config
  server.on("/save-config", HTTP_GET, [] (AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    if (request->hasParam("WifiName")){
      wifi_name = request->getParam("WifiName")->value();
    }
    if (request->hasParam("WifiPwd")){
      wifi_pwd = request->getParam("WifiPwd")->value();
    }
    if (request->hasParam("ApiServerIp")){
      API_SERVER_IP = request->getParam("ApiServerIp")->value();
    }
    if (request->hasParam("ApiUri")){
      API_URI = request->getParam("ApiUri")->value();
    }
    if (request->hasParam("ApiKey")){
      API_KEY = request->getParam("ApiKey")->value();
    }
    if (request->hasParam("TargetDeviceId")){
      TARGET_DEVICE_ID = request->getParam("TargetDeviceId")->value();
    }
    if (request->hasParam("TargetSensorId")){
      TARGET_SENSOR_ID = request->getParam("TargetSensorId")->value();
    }

    Serial.println("wifi_name: " + wifi_name);
    Serial.println("wifi_pwd: " + wifi_pwd);
    Serial.println("API_SERVER_IP: " + API_SERVER_IP);
    Serial.println("API_URI: " + API_URI);
    Serial.println("API_KEY: " + API_KEY);
    Serial.println("TARGET_DEVICE_ID: " + TARGET_DEVICE_ID);
    Serial.println("TARGET_SENSOR_ID: " + TARGET_SENSOR_ID);

    // Write configuration data to EEPROM
    EEPROM.write(addr_is_configured, 1);
    WriteStringToEEPROM(addr_wifi_name, wifi_name);
    WriteStringToEEPROM(addr_wifi_pwd, wifi_pwd);
    WriteStringToEEPROM(addr_api_server_ip, API_SERVER_IP);
    WriteStringToEEPROM(addr_api_uri, API_URI);
    WriteStringToEEPROM(addr_api_key, API_KEY);
    WriteStringToEEPROM(addr_target_device_id, TARGET_DEVICE_ID);
    WriteStringToEEPROM(addr_target_sensor_id, TARGET_SENSOR_ID);
    EEPROM.commit();
    Serial.println("[SUCCESS] Write configuration data to EEPROM.");

    request->send(200, "text/plain", "OK");
  });

  // Route to clear-config
  server.on("/clear-config", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    
    // Clear EEPROM
    ClearEEPROM();
    Serial.println("[SUCCESS] EEPROM is cleared.");
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();
}
  
void loop() {
}

// Initialize configuration data
void InitConfigData()
{
  wifi_name = ReadStringFromEEPROM(addr_wifi_name);
  wifi_pwd = ReadStringFromEEPROM(addr_wifi_pwd);
  API_SERVER_IP = ReadStringFromEEPROM(addr_api_server_ip);
  API_URI = ReadStringFromEEPROM(addr_api_uri);
  API_KEY = ReadStringFromEEPROM(addr_api_key);
  TARGET_DEVICE_ID = ReadStringFromEEPROM(addr_target_device_id);
  TARGET_SENSOR_ID = ReadStringFromEEPROM(addr_target_sensor_id);
}

// Write string to EEPROM
void WriteStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

// Read string from EEPROM
String ReadStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0'; 
  return String(data);
}

// Clear EEPROM
void ClearEEPROM()
{
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }

  wifi_name = "";
  wifi_pwd = "";
  API_SERVER_IP = "";
  API_URI = "";
  API_KEY = "";
  TARGET_DEVICE_ID = "";
  TARGET_SENSOR_ID = "";

  EEPROM.commit();
}