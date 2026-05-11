#pragma once
#include "../types.h"

class DiagnosticsEngine {
public:
  void update(LiveData& live);

private:
  static const int HISTORY_LEN = 120;

  float vibHist[HISTORY_LEN] = {0};
  float deltaHist[HISTORY_LEN] = {0};
  float dbHist[HISTORY_LEN] = {0};

  int vibHead = 0;
  int deltaHead = 0;
  int dbHead = 0;
  int sampleCount = 0;

  void push(float* arr, int len, int& head, float value);
  float avg(const float* arr, int len) const;
  float maxv(const float* arr, int len) const;
};
