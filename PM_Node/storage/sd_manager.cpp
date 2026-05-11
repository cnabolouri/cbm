#include "sd_manager.h"
#include "../config.h"
#include <math.h>

static String jsonEscape(const String& s) {
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c != '\r') out += c;
  }
  return out;
}

static String jsonExtract(const String& json, const String& key) {
  String token = "\"" + key + "\"";
  int p = json.indexOf(token);
  if (p < 0) return "";
  p = json.indexOf(':', p);
  if (p < 0) return "";
  p = json.indexOf('"', p);
  if (p < 0) return "";

  String val;
  bool escaping = false;
  for (int i = p + 1; i < (int)json.length(); i++) {
    char c = json[i];
    if (escaping) {
      if (c == 'n') val += '\n';
      else val += c;
      escaping = false;
    } else if (c == '\\') {
      escaping = true;
    } else if (c == '"') {
      break;
    } else {
      val += c;
    }
  }
  return val;
}

static String htmlEscape(const String& s) {
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

bool SDManager::begin() {
  Serial.println("Setting SD_MMC pins...");
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

  Serial.println("Trying Freenove SD_MMC begin...");
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("Card Mount Failed");
    ready = false;
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    ready = false;
    return false;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  ready = true;
  return true;
}

bool SDManager::isReady() const {
  return ready;
}

bool SDManager::exists(const String& path) {
  return ready && SD_MMC.exists(path);
}

bool SDManager::removeFile(const String& path) {
  return ready && SD_MMC.remove(path);
}

bool SDManager::renameFile(const String& from, const String& to) {
  return ready && SD_MMC.rename(from, to);
}

File SDManager::openRead(const String& path) {
  if (!ready) return File();
  return SD_MMC.open(path, FILE_READ);
}

File SDManager::openWrite(const String& path) {
  if (!ready) return File();
  return SD_MMC.open(path, FILE_WRITE);
}

String SDManager::nextRecordingBaseName() {
  for (int i = 1; i < 10000; i++) {
    char name[20];
    snprintf(name, sizeof(name), "/REC_%04d", i);
    String wav = String(name) + ".wav";
    if (!SD_MMC.exists(wav)) return String(name);
  }
  return "/REC_9999";
}

static String fileSizeString(size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  if (bytes < 1024 * 1024) return String(bytes / 1024.0f, 1) + " KB";
  return String(bytes / 1024.0f / 1024.0f, 2) + " MB";
}

static String formatDurationMs(unsigned long ms) {
  float sec = ms / 1000.0f;
  if (sec < 60.0f) return String(sec, 1) + " s";

  int minPart = (int)(sec / 60.0f);
  float secPart = sec - (minPart * 60.0f);
  String out = String(minPart) + "m ";
  if (secPart < 10.0f) out += "0";
  out += String(secPart, 1) + "s";
  return out;
}

bool SDManager::saveSessionMeta(const String& baseName, const String& tag, const String& status, const String& note) {
  if (!ready) return false;

  String metaName = baseName;
  if (!metaName.startsWith("/")) metaName = "/" + metaName;
  metaName += ".meta.json";

  if (SD_MMC.exists(metaName)) {
    SD_MMC.remove(metaName);
  }

  File f = SD_MMC.open(metaName, FILE_WRITE);
  if (!f) return false;

  String json = "{";
  json += "\"tag\":\"" + jsonEscape(tag) + "\",";
  json += "\"status\":\"" + jsonEscape(status) + "\",";
  json += "\"note\":\"" + jsonEscape(note) + "\"";
  json += "}";

  f.print(json);
  f.close();
  return true;
}

bool SDManager::loadSessionMeta(const String& baseName, SessionInfo& info) {
  if (!ready) return false;

  String metaName = baseName;
  if (!metaName.startsWith("/")) metaName = "/" + metaName;
  metaName += ".meta.json";

  if (!SD_MMC.exists(metaName)) return false;

  File f = SD_MMC.open(metaName, FILE_READ);
  if (!f) return false;

  String json = f.readString();
  f.close();

  info.hasMeta = true;
  info.tag = jsonExtract(json, "tag");
  info.status = jsonExtract(json, "status");
  info.note = jsonExtract(json, "note");
  return true;
}

bool SDManager::loadSessionSummary(const String& baseName, SessionSummary& summary) {
  if (!ready) return false;

  String csvName = baseName;
  if (!csvName.startsWith("/")) csvName = "/" + csvName;
  csvName += ".csv";

  File f = SD_MMC.open(csvName, FILE_READ);
  if (!f) return false;

  String line = f.readStringUntil('\n');
  if (!line.length()) {
    f.close();
    return false;
  }
  bool hasTrustColumns = line.indexOf("trusted") >= 0;

  float sumVib = 0.0f;
  float sumDT = 0.0f;
  float sumDb = 0.0f;
  float maxVib = 0.0f;
  float maxDT = 0.0f;
  float maxDb = 0.0f;
  float maxHz = 0.0f;
  float maxAbsX = 0.0f;
  float maxAbsY = 0.0f;
  float maxAbsZ = 0.0f;
  int count = 0;
  int totalCount = 0;
  int trustedCount = 0;
  bool firstTrustedSeen = false;
  unsigned long firstTrustedMs = 0;
  unsigned long lastTrustedMs = 0;

  while (f.available()) {
    line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;

    float vals[15] = {0};
    int idx = 0;
    int start = 0;

    for (int i = 0; i <= (int)line.length() && idx < 15; i++) {
      if (i == (int)line.length() || line[i] == ',') {
        vals[idx++] = line.substring(start, i).toFloat();
        start = i + 1;
      }
    }

    if (hasTrustColumns && idx < 15) continue;
    if (!hasTrustColumns && idx < 12) continue;

    totalCount++;
    bool trusted = !hasTrustColumns || ((int)vals[1] == 1);
    if (!trusted) continue;

    trustedCount++;

    unsigned long ms = (unsigned long)vals[0];
    if (!firstTrustedSeen) {
      firstTrustedSeen = true;
      firstTrustedMs = ms;
    }
    lastTrustedMs = ms;

    float vx = hasTrustColumns ? vals[4] : vals[1];
    float vy = hasTrustColumns ? vals[5] : vals[2];
    float vz = hasTrustColumns ? vals[6] : vals[3];
    float vt = hasTrustColumns ? vals[7] : vals[4];
    float dT = hasTrustColumns ? vals[11] : vals[8];
    float dB = hasTrustColumns ? vals[13] : vals[10];
    float hz = hasTrustColumns ? vals[14] : vals[11];

    float ax = fabs(vx);
    float ay = fabs(vy);
    float az = fabs(vz);
    float avt = fabs(vt);

    if (ax > maxAbsX) maxAbsX = ax;
    if (ay > maxAbsY) maxAbsY = ay;
    if (az > maxAbsZ) maxAbsZ = az;
    if (avt > maxVib) maxVib = avt;
    if (dT > maxDT) maxDT = dT;
    if (dB > maxDb) maxDb = dB;
    if (hz > maxHz) maxHz = hz;

    sumVib += avt;
    sumDT += dT;
    sumDb += dB;
    count++;
  }

  f.close();
  summary.totalSamples = totalCount;
  summary.trustedSamples = trustedCount;
  summary.firstTrustedMs = firstTrustedSeen ? firstTrustedMs : 0;
  summary.lastTrustedMs = firstTrustedSeen ? lastTrustedMs : 0;
  summary.validDurationMs = (firstTrustedSeen && lastTrustedMs >= firstTrustedMs) ? (lastTrustedMs - firstTrustedMs) : 0;
  summary.trustedRatio = (totalCount > 0) ? (100.0f * trustedCount / totalCount) : 0.0f;
  if (count == 0) {
    summary.valid = false;
    summary.overallAlert = "Untrusted";
    return false;
  }

  summary.valid = true;
  summary.maxTotalVib = maxVib;
  summary.avgTotalVib = sumVib / count;

  if (maxAbsX >= maxAbsY && maxAbsX >= maxAbsZ) summary.dominantAxis = "X";
  else if (maxAbsY >= maxAbsX && maxAbsY >= maxAbsZ) summary.dominantAxis = "Y";
  else summary.dominantAxis = "Z";

  if (summary.maxTotalVib >= 0.15f) summary.vibrationAlert = "High";
  else if (summary.maxTotalVib >= 0.05f) summary.vibrationAlert = "Watch";
  else summary.vibrationAlert = "Normal";

  summary.maxDeltaTemp = maxDT;
  summary.avgDeltaTemp = sumDT / count;

  if (summary.maxDeltaTemp >= 15.0f) summary.temperatureAlert = "High";
  else if (summary.maxDeltaTemp >= 5.0f) summary.temperatureAlert = "Watch";
  else summary.temperatureAlert = "Normal";

  summary.maxDb = maxDb;
  summary.avgDb = sumDb / count;
  summary.maxHz = maxHz;

  if (summary.maxDb >= 55.0f) summary.soundAlert = "High";
  else if (summary.maxDb >= 40.0f) summary.soundAlert = "Watch";
  else summary.soundAlert = "Normal";

  summary.overallAlert = "Normal";
  if (summary.vibrationAlert == "High" || summary.temperatureAlert == "High" || summary.soundAlert == "High") {
    summary.overallAlert = "High";
  } else if (summary.vibrationAlert == "Watch" || summary.temperatureAlert == "Watch" || summary.soundAlert == "Watch") {
    summary.overallAlert = "Watch";
  }

  return true;
}

static int findSessionIndex(const std::vector<SessionInfo>& sessions, const String& baseName) {
  for (size_t i = 0; i < sessions.size(); i++) {
    if (sessions[i].baseName == baseName) return (int)i;
  }
  return -1;
}

std::vector<SessionInfo> SDManager::listSessions() {
  std::vector<SessionInfo> sessions;
  if (!ready) return sessions;

  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) return sessions;

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      if (!name.startsWith("/")) name = "/" + name;

      if (name.endsWith(".meta.json")) {
        file = root.openNextFile();
        continue;
      }

      bool isCsv = name.endsWith(".csv");
      bool isWav = name.endsWith(".wav");
      if (isCsv || isWav) {
        String baseName = name;
        baseName.remove(baseName.length() - 4);

        int idx = findSessionIndex(sessions, baseName);
        if (idx < 0) {
          SessionInfo info;
          info.baseName = baseName;
          sessions.push_back(info);
          idx = sessions.size() - 1;
        }

        if (isCsv) {
          sessions[idx].hasCsv = true;
          sessions[idx].csvSize = file.size();
        } else {
          sessions[idx].hasWav = true;
          sessions[idx].wavSize = file.size();
        }
      }
    }
    file = root.openNextFile();
  }

  for (size_t i = 0; i < sessions.size(); i++) {
    loadSessionMeta(sessions[i].baseName, sessions[i]);
    if (sessions[i].hasCsv) {
      loadSessionSummary(sessions[i].baseName, sessions[i].summary);
    }
  }

  return sessions;
}

std::vector<SessionInfo> listSessions() {
  std::vector<SessionInfo> sessions;

  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) return sessions;

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      if (!name.startsWith("/")) name = "/" + name;

      if (name.endsWith(".meta.json")) {
        file = root.openNextFile();
        continue;
      }

      bool isCsv = name.endsWith(".csv");
      bool isWav = name.endsWith(".wav");
      if (isCsv || isWav) {
        String baseName = name;
        baseName.remove(baseName.length() - 4);

        int idx = findSessionIndex(sessions, baseName);
        if (idx < 0) {
          SessionInfo info;
          info.baseName = baseName;
          sessions.push_back(info);
          idx = sessions.size() - 1;
        }

        if (isCsv) {
          sessions[idx].hasCsv = true;
          sessions[idx].csvSize = file.size();
        } else {
          sessions[idx].hasWav = true;
          sessions[idx].wavSize = file.size();
        }
      }
    }
    file = root.openNextFile();
  }

  return sessions;
}

String SDManager::listSessionsCardsHtml() {
  std::vector<SessionInfo> sessions = listSessions();
  String s;

  if (!ready) {
    s += "<div class='card'>SD not ready</div>";
    return s;
  }

  if (sessions.empty()) {
    s += "<div class='card'>No sessions found</div>";
    return s;
  }

  s += "<div class='grid'>";

  for (size_t i = 0; i < sessions.size(); i++) {
    String base = sessions[i].baseName;
    String csv = base + ".csv";
    String wav = base + ".wav";
    String encBase = base;
    String encCsv = csv;
    String encWav = wav;
    encBase.replace(" ", "%20");
    encCsv.replace(" ", "%20");
    encWav.replace(" ", "%20");

    String overallColor = "#238636";
    if (sessions[i].summary.overallAlert == "High") overallColor = "#b42318";
    else if (sessions[i].summary.overallAlert == "Watch") overallColor = "#9a6700";

    s += "<div class='card'>";
    s += "<div style='display:flex;justify-content:space-between;align-items:center;gap:8px;'>";
    s += "<h2 style='margin-top:0;margin-bottom:8px;'>" + htmlEscape(base) + "</h2>";
    if (sessions[i].summary.valid) {
      s += "<span style='background:" + overallColor + ";color:#fff;padding:4px 8px;border-radius:999px;font-size:12px;'>" + sessions[i].summary.overallAlert + "</span>";
    }
    s += "</div>";

    if (sessions[i].hasMeta) {
      if (sessions[i].status.length()) {
        s += "<div class='muted'><b>Status:</b> " + htmlEscape(sessions[i].status) + "</div>";
      }
      if (sessions[i].tag.length()) {
        s += "<div class='muted'><b>Tag:</b> " + htmlEscape(sessions[i].tag) + "</div>";
      }
      if (sessions[i].note.length()) {
        s += "<div class='muted'><b>Note:</b> " + htmlEscape(sessions[i].note) + "</div>";
      }
    }
    s += "<div class='muted'>CSV: ";
    s += sessions[i].hasCsv ? (String("Yes (") + fileSizeString(sessions[i].csvSize) + ")") : String("No");
    s += "</div>";
    s += "<div class='muted'>WAV: ";
    s += sessions[i].hasWav ? (String("Yes (") + fileSizeString(sessions[i].wavSize) + ")") : String("No");
    s += "</div>";

    if (sessions[i].summary.totalSamples > 0) {
      s += "<div class='muted'><b>Trusted:</b> " + String(sessions[i].summary.trustedSamples) + " / " + String(sessions[i].summary.totalSamples) + " (" + String(sessions[i].summary.trustedRatio, 1) + "%)</div>";
      s += "<div class='muted'><b>Valid window:</b> " + formatDurationMs(sessions[i].summary.validDurationMs) + "</div>";
    }

    if (sessions[i].summary.valid) {
      s += "<hr style='border-color:#333;margin:12px 0;'>";
      s += "<div class='muted'><b>Vib max:</b> " + String(sessions[i].summary.maxTotalVib, 5) + " in/s</div>";
      s += "<div class='muted'><b>Vib avg:</b> " + String(sessions[i].summary.avgTotalVib, 5) + " in/s</div>";
      s += "<div class='muted'><b>Axis:</b> " + sessions[i].summary.dominantAxis + " | <b>Alert:</b> " + sessions[i].summary.vibrationAlert + "</div>";
      s += "<div class='muted'><b>dT max:</b> " + String(sessions[i].summary.maxDeltaTemp, 2) + " F | <b>Avg:</b> " + String(sessions[i].summary.avgDeltaTemp, 2) + " F</div>";
      s += "<div class='muted'><b>Temp alert:</b> " + sessions[i].summary.temperatureAlert + "</div>";
      s += "<div class='muted'><b>dB max:</b> " + String(sessions[i].summary.maxDb, 2) + " | <b>Avg:</b> " + String(sessions[i].summary.avgDb, 2) + "</div>";
      s += "<div class='muted'><b>Hz max:</b> " + String(sessions[i].summary.maxHz, 1) + " | <b>Sound alert:</b> " + sessions[i].summary.soundAlert + "</div>";
    }

    s += "<div style='margin-top:12px;display:flex;gap:8px;flex-wrap:wrap;'>";

    if (sessions[i].hasCsv) {
      s += "<a class='btn' href='/view?name=" + encCsv + "'>View</a>";
      s += "<a class='btn' href='/compare?a=" + encCsv + "&b=" + encCsv + "'>Compare</a>";
      s += "<a class='btn' href='/download?name=" + encCsv + "'>CSV</a>";
      s += "<a class='btn' href='/edit_session?base=" + encBase + "'>Edit</a>";
    }

    if (sessions[i].hasWav) {
      s += "<a class='btn' href='/download?name=" + encWav + "'>WAV</a>";
    }

    s += "<a class='btn' style='background:#b42318;' href='/delete_session?base=" + encBase + "' onclick=\"return confirm('Delete session?');\">Delete</a>";
    s += "</div></div>";
  }

  s += "</div>";
  return s;
}

String listSessionsTableHtml() {
  std::vector<SessionInfo> sessions = listSessions();
  String s;
  s += "<table><tr><th>Session</th><th>CSV</th><th>WAV</th><th>Actions</th></tr>";

  if (sessions.empty()) {
    s += "<tr><td colspan='4'>No sessions found</td></tr></table>";
    return s;
  }

  for (size_t i = 0; i < sessions.size(); i++) {
    String base = sessions[i].baseName;
    String csv = base + ".csv";
    String wav = base + ".wav";
    String encCsv = csv;
    String encWav = wav;
    encCsv.replace(" ", "%20");
    encWav.replace(" ", "%20");

    String csvSize = sessions[i].hasCsv ? fileSizeString(sessions[i].csvSize) : String("-");
    String wavSize = sessions[i].hasWav ? fileSizeString(sessions[i].wavSize) : String("-");

    s += "<tr><td>" + base + "</td>";
    s += "<td>" + csvSize + "</td>";
    s += "<td>" + wavSize + "</td><td>";

    if (sessions[i].hasCsv) {
      s += "<a href='/view?name=" + encCsv + "'>View</a> ";
      s += "<a href='/download?name=" + encCsv + "'>CSV</a> ";
    }
    if (sessions[i].hasWav) {
      s += "<a href='/download?name=" + encWav + "'>WAV</a>";
    }

    s += "</td></tr>";
  }

  s += "</table>";
  return s;
}

String SDManager::listFilesTableHtml() {
  String s;
  s += "<table><tr><th>Name</th><th>Size</th><th>Actions</th></tr>";

  if (!ready) {
    s += "<tr><td colspan='3'>SD not ready</td></tr></table>";
    return s;
  }

  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    s += "<tr><td colspan='3'>Failed to open root</td></tr></table>";
    return s;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      String enc = name;
      enc.replace(" ", "%20");

      s += "<tr><td>" + name + "</td>";
      s += "<td>" + fileSizeString(file.size()) + "</td>";
      s += "<td>";

      if (name.endsWith(".csv")) {
        s += "<a href='/view?name=" + enc + "'>View</a> ";
      }

      s += "<a href='/download?name=" + enc + "'>Download</a> ";
      s += "<a href='/delete?name=" + enc + "' onclick=\"return confirm('Delete?');\">Delete</a>";
      s += "</td></tr>";
    }
    file = root.openNextFile();
  }

  s += "</table>";
  return s;
}
