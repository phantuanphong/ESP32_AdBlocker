
// Assist setup for new app installations
// original provided by gemi254
// 
// s60sc 2023

#include "appGlobals.h"

#if (!INCLUDE_CERTS)
const char* git_rootCACertificate = "";
#endif

static fs::FS fp = STORAGE;

static bool wgetFile(const char* filePath) {
  // download required data file from github repository and store
  bool res = false;
  if (fp.exists(filePath)) {
    // if file exists but is empty, delete it to allow re-download
    File f = fp.open(filePath, FILE_READ);
    size_t fSize = f.size();
    f.close();
    if (!fSize) fp.remove(filePath);
  }
  if (!fp.exists(filePath)) {
    char downloadURL[150];
    snprintf(downloadURL, 150, "%s%s", GITHUB_PATH, filePath);
    File f = fp.open(filePath, FILE_WRITE);
    if (f) {
      WiFiClientSecure wclient;
      if (remoteServerConnect(wclient, GITHUB_HOST, HTTPS_PORT, git_rootCACertificate, SETASSIST)) {
        HTTPClient https;
        if (https.begin(wclient, GITHUB_HOST, HTTPS_PORT, downloadURL, true)) {
          LOG_INF("Downloading %s from %s", filePath, downloadURL);    
          int httpCode = https.GET();
          int fileSize = 0;
          if (httpCode == HTTP_CODE_OK) {
            fileSize = https.writeToStream(&f);
            if (fileSize <= 0) {
              LOG_WRN("Download failed: writeToStream - %s", https.errorToString(fileSize).c_str());
              httpCode = 0;
            } else LOG_INF("Downloaded %s, size %s", filePath, fmtSize(fileSize));       
          } else LOG_WRN("Download failed, error: %s", https.errorToString(httpCode).c_str());    
          https.end();
          f.close();
          if (httpCode == HTTP_CODE_OK) {
            if (!strcmp(filePath, CONFIG_FILE_PATH)) doRestart("Config file downloaded");
            res = true;
          } else {
            LOG_WRN("HTTP Get failed with code: %d", httpCode);
            fp.remove(filePath);
          }
        }
      } 
      remoteServerClose(wclient);
    } else LOG_WRN("Open failed: %s", filePath);
  } else res = true;
  return res;
}

bool checkDataFiles() {
  // Download any missing data files
  bool res = false;
  if (strlen(GITHUB_PATH)) {
    res = wgetFile(COMMON_JS_PATH); 
    if (res) res = wgetFile(INDEX_PAGE_PATH); 
    //if (res) res = wgetFile(OTA_FILE_PATH); 
    if (res) res = appDataFiles(); 
  } else res = true; // no download needed
  return res;
}

const char* setupPage_html = R"~(
<!doctype html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Application setup</title> 
</head>
<script>
function Config(){
  if (!window.confirm('This will reboot the device to activate new settings.'))  return false; 
  fetch('/control?ST_SSID=' + encodeURI(document.getElementById('ST_SSID').value))
  .then(r => { console.log(r); return fetch('/control?ST_Pass=' + encodeURI(document.getElementById('ST_Pass').value)) })
  .then(r => { console.log(r); return fetch('/control?save=1') })     
  .then(r => { console.log(r); return fetch('/control?reset=1') })
  .then(r => { console.log(r); }); 
  return false;
}
</script>
<body style="font-size:18px">
<br>
<center>
  <table border="0">
    <tr><th colspan="3">Wifi setup..</th></tr>
    <tr><td colspan="3"></td></tr>
    <tr>
    <td>SSID</td>
    <td>&nbsp;</td>
    <td><input id="ST_SSID" name="ST_SSID" length=32 placeholder="Router SSID" class="input"></td>
  </tr>
    <tr>
    <td>Password</td>
    <td>&nbsp;</td>
    <td><input id="ST_Pass" name="ST_Pass" length=64 placeholder="Router password" class="input"></td>
  </tr>
  <tr><td colspan="3"></td></tr>
    <tr><td colspan="3" align="center">
        <button type="button" onClick="return Config()">Connect</button>&nbsp;<button type="button" onclick="window.location.reload;">Cancel</button>
    </td></tr>
  </table>
</center>      
</body>
</html>
)~";