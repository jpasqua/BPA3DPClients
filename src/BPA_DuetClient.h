
#ifndef BPA_DuetClient_h
#define BPA_DuetClient_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <JSONService.h>
//                                  Local Includes
#include "BPA_PrintClient.h"
//--------------- End:    Includes ---------------------------------------------


class FileInfo {
public:
  FileInfo() { reset(); }
  int      err;                     // Error code (response.err - Uses -1 to indicate not set)
  // ----- File
  String   name;                    // Name of file (response.fileName - stripped of path)
  uint32_t size;                    // Size of the file being printed (response.size)
  String   generatedBy;             // Program that generated the file (response.generatedBy)
  String   lastModified;            // Last mod date in file system (response.lastModified)
  // ----- Model data
  float    height;                  // Overall height of the print when complete (response.height)
  uint32_t printTime;               // Total time (seconds) to print this file (response.printTime)
  uint32_t filament;                // Total filament (mm) to print this file (sum of response.filament[])
  // ----- Print-settings
  float    firstLayerHeight;        // Height of first layer (response.firstLayerHeight)
  float    layerHeight;             // Normal layer height (response.layerHeight)

  void reset() {
    err = -1;
    size = 0;
    lastModified = "";
    height = 0.0;
    firstLayerHeight = 0.0;
    layerHeight = 0.0;
    printTime = 0;
    filament = 0;
    name = "";
    generatedBy = "";
  }

  void dumpToLog() {
    if (err) Log.verbose(F("----- FileInfo: Values have not been set, err = %d"), err);
    else {
      Log.verbose(F("----- FileInfo -----"));
      Log.verbose(F("File"));
      Log.verbose(F("  name: %s"), name.c_str());
      Log.verbose(F("  size: %d"), size);
      Log.verbose(F("  Generated by: %s"), generatedBy.c_str());
      Log.verbose(F("  Modified: %s"), lastModified.c_str());
      Log.verbose(F("Model Data"));
      Log.verbose(F("  Overall model height: %F"), height);
      Log.verbose(F("  Total time to print: %d (sec)"), printTime);
      Log.verbose(F("  Total filament for print: %d (mm)"), filament);
      Log.verbose(F("Print Settings"));
      Log.verbose(F("  First layer height: %F"), firstLayerHeight);
      Log.verbose(F("  Normal layer height: %F"), layerHeight);
      Log.verbose(F("----------"));
    }
  }
};

class RRState {
public:
  RRState() { reset(); }

  String status;                    // Status indicator (response.status)
    // If status is empty, it means that we have not successfully retrieved job status
    // The following values are all states which should be considered "Printing"
    //   D (decelerating, pausing a running print)
    //   S (stopped, live print has been paused)
    //   R (resuming a paused print)
    //   P (printing a file)
    //   M (what is this? Used in DWC)
    // The following values are all states which should be considered "Operational"
    //   C (configuration file is being processed)
    //   I (idle, no movement or code is being performed)
    //   B (busy, live movement is in progress or a macro file is being run)
    //   H (halted, after emergency stop)
    //   F (flashing new firmware)
    //   T (changing tool, new in 1.17b)

  struct {
    float actual;                   // response.temps.current[1]
    float target;                   // response.temps.tools.active[0][0]
  } toolTemp;
  struct {
    float actual;                   // response.temps.bed.current
    float target;                   // response.temps.bed.active
  } bedTemp;

  float printDuration;              // How long the print has been going including warmup (response.printDuration)
  float warmupDuration;             // Warmup period (response.warmUpDuration)
  float remaining[3];               // Estimated time left based on: 0: file, 1: filament, 2: Layer
                                    // (response.timesLeft.{file,filament,layer})

  void reset() {
    status = "";  // An empty status means that we have no status
    toolTemp.actual = 0.0;
    toolTemp.target = 0.0;
    bedTemp.actual = 0.0;
    bedTemp.target = 0.0;
    warmupDuration = 0.0;
    printDuration = 0.0;
    remaining[0] = remaining[1] = remaining[2] = 0.0;
  }

  void dumpToLog() {
    if (status.isEmpty()) Log.verbose(F("RRState: Values have not been set"));
    else {
      Log.verbose(F("----- RRState: %s"), status.c_str());
      Log.verbose(F("  Tool Temp: %F (C)"), toolTemp.actual);
      Log.verbose(F("  Tool Target Temp: %F (C)"), toolTemp.target);
      Log.verbose(F("  Tool Temp: %F (C)"), bedTemp.actual);
      Log.verbose(F("  Tool Target Temp: %F (C)"), bedTemp.target);
      Log.verbose(F("  printDuration: %F (sec)"), printDuration);
      Log.verbose(F("  warmupDuration: %F (sec)"), warmupDuration);
      Log.verbose(
          "  Remaining (sec): File: %F, Filament: %F, Layer: %F",
          remaining[0], remaining[1], remaining[2]);
      Log.verbose(F("----------"));
    }
  }
};


class DuetClient : public PrintClient {
public:
  // ----- Constructors and initialization
  void init(String server, int port, String pass="");

  // ----- Interrogate the Printer
  void updateState();

  // ----- Getters
  bool isPrinting();
  State getState();
  float getPctComplete();
  uint32_t getPrintTimeLeft();
  uint32_t getElapsedTime();
  String getFilename();
  void getBedTemps(float &actual, float &target);
  void getToolTemps(float &actual, float &target);

  // ----- Utility Functions
  void dumpToLog();
  void acknowledgeCompletion();


private:
  ServiceDetails  details;
  JSONService     *service = NULL;

  // ----- State from the printer
  FileInfo        fileInfo;
  RRState         rrState;
  // ----- State derived from info from the printer
  uint32_t        printTimeEstimate = 0;  // printTimeEstimate is always >= elapsed
  float           elapsed = 0.0f;
  PrintClient::State printerState = PrintClient::State::Offline;

  bool connect();
  bool disconnect();

  void getFileInfo();
  void getRRState();
  void updateDerivedValues();
};

#endif // BPA_DuetClient_h
