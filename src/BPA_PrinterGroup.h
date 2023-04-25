#ifndef BPA_PrinterGroup_h
#define BPA_PrinterGroup_h

#include <BPABasics.h>
#include "BPA_PrinterSettings.h"
#include "BPA_PrintClient.h"

class PrinterGroup {
public:
  static constexpr char DataProviderPrefix = 'P';

  PrinterGroup(
        uint8_t nPrintersInGroup, PrinterSettings* ps,
        uint32_t refreshInterval, std::function<void(bool)> busyCallback);

  void activatePrinter(int i);

  void refreshPrinterData(bool force);

  String getDisplayName(uint8_t whichPrinter);
  PrintClient* getPrinter(uint8_t whichPrinter);
  PrinterSettings* getSettings(uint8_t whichPrinter);

  void nextCompletion(String &printer, String &formattedTime, uint32_t &delta);
  void dataSupplier(const String& key, String& value);

private:
  uint8_t _nPrintersInGroup;
  PrinterSettings* _ps;       // Size == _nPrintersInGroup
  uint32_t _refreshInterval;
  std::function< void(bool)> _busyCallback;

  PrintClient** _printer;     // Size == _nPrintersInGroup
  uint32_t* _lastUpdateTime;  // Size == _nPrintersInGroup
  String* _printerIPs;        // Size == _nPrintersInGroup


  void cachePrinterIP(int i);
  void mapPrinterSpecific(const String& key, String& value, int printerIndex);
  void completionTime(String &formattedTime, uint32_t timeLeft);

};

#endif  // BPA_PrinterGroup_h