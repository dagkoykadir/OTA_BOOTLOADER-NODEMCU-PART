#ifndef SPIFFS_UTILS_H
#define SPIFFS_UTILS_H

#include <Arduino.h>
#include <FS.h>

struct SpiffsInfo {
  size_t totalBytes = 0;
  size_t usedBytes = 0;
  size_t freeBytes = 0;
};

class SpiffsUtils {
public:
  static const size_t DEFAULT_CHUNK_SIZE = 128; // Changed from 1024 to 128 bytes
  static void setup();
  static void checkSpiffsStatus();
  static void clearSpiffs();
  static void testSpiffs();
  static String listSpiffsFiles();
  static SpiffsInfo getSpiffsInfo();
  static bool splitFileIntoChunks(const char* sourceFile, size_t chunkSize = DEFAULT_CHUNK_SIZE);
  static size_t getChunkCount(size_t* totalSize);

private:
  static SpiffsInfo spiffsInfo;
  static const char* updateFilePath;
};

#endif