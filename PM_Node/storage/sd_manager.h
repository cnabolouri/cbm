#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <vector>
#include "../types.h"

std::vector<SessionInfo> listSessions();
String listSessionsTableHtml();
class SDManager {
public:
  bool begin();
  bool isReady() const;

  bool exists(const String& path);
  bool removeFile(const String& path);
  bool renameFile(const String& from, const String& to);

  File openRead(const String& path);
  File openWrite(const String& path);

  String nextRecordingBaseName();
  std::vector<SessionInfo> listSessions();
  String listSessionsCardsHtml();
  bool saveSessionMeta(const String& baseName, const String& tag, const String& status, const String& note);
  bool loadSessionMeta(const String& baseName, SessionInfo& info);
  bool loadSessionSummary(const String& baseName, SessionSummary& summary);
  String listFilesTableHtml();

private:
  bool ready = false;
};
