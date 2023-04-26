/*
 * BPA_PrinterGroup:
 *    Encapsulates the notion of a group of printers (PrintClients) and helps
 *    to manage them more easily. Also provides a DataSupplier that is compatible
 *    with the [WebThing framework](https://github.com/jpasqua/WebThing),
 *    but can also be used independently.
 * 
 */

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <Wire.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#else
  #error "Must be an ESP8266 or ESP32"
#endif
//                                  Third Party Libraries
#include <Output.h>
//                                  Local Includes
#include "BPA_OctoClient.h"
#include "BPA_MockPrintClient.h"
#include "BPA_DuetClient.h"
#include "BPA_PrinterGroup.h"
//--------------- End:    Includes ---------------------------------------------


PrinterGroup::PrinterGroup(
      uint8_t nPrintersInGroup, PrinterSettings* ps,
      uint32_t refreshInterval, std::function<void(bool)> busyCallback)
{
  _nPrintersInGroup = nPrintersInGroup;
  _ps = ps;
  _refreshInterval = refreshInterval;
  _busyCallback = busyCallback;

  _lastUpdateTime = new uint32_t[_nPrintersInGroup];
  _printer = new PrintClient*[_nPrintersInGroup];
  _printerIPs = new String[nPrintersInGroup];
  for (int i = 0; i < _nPrintersInGroup; i++) {
    _lastUpdateTime[i] = 0;
    _printer[i] = nullptr;
    Basics::resetString(_printerIPs[i]);
  }
}

void PrinterGroup::refreshPrinterData(bool force) {
  for (int i = 0; i < _nPrintersInGroup; i++) {
    if (_ps[i].isActive) {
      uint32_t threshold = UINT32_MAX;
      // Randomize the refresh times a little so we aren't do all the updates
      // at once which can cause the UI to become unresponsive
      switch (_printer[i]->getState()) {
        case PrintClient::State::Offline:
          threshold = (random(5*60, 10*60) * 1000L);  // 5 to 10 minutes
          break;
        case PrintClient::State::Operational: 
        case PrintClient::State::Complete:
          threshold = (random(1*60, 3*60) * 1000L);   // 1 to 3 minutes
          break;
        case PrintClient::State::Printing:
          threshold = (_refreshInterval * 1000L);  // Caller-specified interval
          break;
      }
      if (force || ((millis() -  _lastUpdateTime[i])) > threshold) {
        if (_busyCallback) _busyCallback(true);
        _printer[i]->updateState();
        _lastUpdateTime[i] = millis();
        _printer[i]->dumpToLog();
      }
    }
  }
  if (_busyCallback) _busyCallback(false);
}

void PrinterGroup::activatePrinter(int i) {
  PrinterSettings *ps = &_ps[i];
  if (!ps->isActive) return;

  cachePrinterIP(i);
  if (_printerIPs[i].isEmpty()) {
    Log.warning(F("Unable to resolve server address for %s"), ps->server.c_str());
    ps->isActive = false;
    return;
  }

  if (_printer[i] != NULL) {
    Log.warning(F("Trying to activate a printer this is already active: %s"), ps->server.c_str());
    return;
  }

  if (ps->mock) {
    Log.verbose(
        "Setting up a MockPrintClient of type %s for %s",
        ps->type.c_str(), ps->server.c_str());
    MockPrintClient *mc = new MockPrintClient();
    _printer[i] = mc;
  } else if (ps->type.equals(Type_Octo)) {
    Log.verbose(F("Setting up an OctoClient for %s: "), ps->server.c_str());
    OctoClient *oc = new OctoClient();
    oc->init(ps->apiKey, _printerIPs[i], ps->port, ps->user, ps->pass);
    _printer[i] = oc;
  } else if (ps->type.equals(Type_Duet)) {
    Log.verbose(F("Setting up an DuetClient for %s: "), ps->server.c_str());
    DuetClient *dc = new DuetClient();
    dc->init(_printerIPs[i], ps->port, ps->pass);
    _printer[i] = dc;
  } else {
    Log.warning(F("Bad printer type: %s"), ps->type.c_str());
    ps->isActive = false;
  }
}

PrintClient* PrinterGroup::getPrinter(uint8_t whichPrinter) {
  return _printer[whichPrinter];
}

String PrinterGroup::getDisplayName(uint8_t whichPrinter) {
  PrinterSettings *ps = &_ps[whichPrinter];
  String displayName;
  if (!ps->nickname.isEmpty()) { displayName = ps->nickname; }
  else if (!ps->server.isEmpty()) { displayName = ps->server; }
  else displayName = "Inactive";
  return displayName;
}

bool PrinterGroup::nextCompletion(uint8_t& whichPrinter, String &formattedTime, uint32_t &delta) {
  uint32_t minCompletion = UINT32_MAX;
  int printerWithNextCompletion;
  for (int i = 0; i < _nPrintersInGroup; i++) {
    if (!_ps[i].isActive) continue;
    if (_printer[i]->getState() == PrintClient::State::Printing) {
      uint32_t thisCompletion = _printer[i]->getPrintTimeLeft();
      if (thisCompletion < minCompletion) {
        minCompletion = thisCompletion;
        printerWithNextCompletion = i;
      }
    }
  }

  if (minCompletion != UINT32_MAX) {
    PrinterSettings *ps = &_ps[printerWithNextCompletion];
    whichPrinter = printerWithNextCompletion;
    delta = minCompletion;
    completionTime(formattedTime, delta);
    return true;
  }
  return false;
}

void PrinterGroup::nextCompletion(String &printer, String &formattedTime, uint32_t &delta) {
  uint8_t whichPrinter;
  if (nextCompletion(whichPrinter, formattedTime, delta)) {
    PrinterSettings *ps = &_ps[whichPrinter];
    printer =  (ps->nickname.isEmpty()) ? ps->server : ps->nickname;
  } else {
    printer = "";
    formattedTime = "";
    delta = 0;
  }
}

void PrinterGroup::dataSupplier(const String& key, String& value) {
  // Map printer related keys
  if (key.equalsIgnoreCase("next")) {
    uint32_t delta;
    String printer, formattedTime;
    nextCompletion(printer, formattedTime, delta);
    if (printer.isEmpty()) value += F("No print in progress");
    else { value += printer; value += ": "; value += formattedTime; }
    return;
  }

  // Check for printer-specific keys
  if (isDigit(key[0]) && key[1] == '.') {
    int index = (key[0] - '0') - 1;
    String subkey = key.substring(2);
    mapPrinterSpecific(subkey, value, index);
    return;
  }
}


//
// ----- Private Functions
//

void PrinterGroup::cachePrinterIP(int i) {
  IPAddress printerIP;
  int result = WiFi.hostByName(_ps[i].server.c_str(), printerIP) ;
  if (result == 1) {
    _printerIPs[i] = printerIP.toString();
  } else {
    _printerIPs[i] = "";
  }
}


//
// ----- Private Functions related to the Data Provider functionality
//

void PrinterGroup::mapPrinterSpecific(const String& key, String& value, int printerIndex) {
  if (printerIndex > _nPrintersInGroup) return;
  PrintClient *p = _printer[printerIndex];
  PrinterSettings *ps = &_ps[printerIndex];
  bool active = ps->isActive;

  if (key.equalsIgnoreCase("name")) {
    if (!ps->nickname.isEmpty()) { value += ps->nickname; }
    else if (!ps->server.isEmpty()) { value += ps->server; }
    else value += "Inactive";
    return;
  }

  if (key.equalsIgnoreCase("pct")) {
    if (active && p->getState() >= PrintClient::State::Complete) { value += (int)(p->getPctComplete()); }
    return;
  }

  if (key.equalsIgnoreCase("state")) {
    if (active) {
      switch (p->getState()) {
        case PrintClient::State::Offline: value += F("Offline"); break;
        case PrintClient::State::Operational: value += F("Online"); break;
        case PrintClient::State::Complete: value += F("Complete"); break;
        case PrintClient::State::Printing: value += F("Printing"); break;
      }
    } else value += F("Unused");
    return;
  }

  if (key.equalsIgnoreCase("status")) {
    if (active) {
      switch (p->getState()) {
        case PrintClient::State::Offline: value += F("Offline"); break;
        case PrintClient::State::Operational: value += F("Online"); break;
        case PrintClient::State::Complete: value += F("Complete"); break;
        case PrintClient::State::Printing:
          value += F("Printing|");
          value += ((int)p->getPctComplete());
          break;
      }
    } else value += F("Unused");
    return;
  }

  if (key.equalsIgnoreCase("next")) {
    if (!active) { return; }
    if (p->isPrinting()) completionTime(value, p->getPrintTimeLeft());
    return;
  }

  if (key.equalsIgnoreCase("remaining")) {
    if (active && p->getState() == PrintClient::State::Printing) {
      value = Output::formattedInterval(p->getPrintTimeLeft(), true, true);
    }
    return;
  }
}

void PrinterGroup::completionTime(String &formattedTime, uint32_t timeLeft) {
  time_t theTime = now() + timeLeft;
  formattedTime = String(dayShortStr(weekday(theTime)));
  formattedTime += " ";
  formattedTime += (Output::using24HourMode()) ? hour(theTime) : hourFormat12(theTime);
  formattedTime += ":";
  int theMinute =  minute(theTime);
  if (theMinute < 10) formattedTime += '0';
  formattedTime += theMinute;
  if (!Output::using24HourMode()) formattedTime += isAM(theTime) ? "AM" : "PM";
}
