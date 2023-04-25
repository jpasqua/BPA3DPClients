//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoJson.h>
#include <ArduinoLog.h>
//                                  Local Includes
#include "BPA_PrinterSettings.h"
//--------------- End:    Includes ---------------------------------------------

PrinterSettings::PrinterSettings() {
  init();
}

void PrinterSettings::init() {
  apiKey = "";
  server = "octopi.local";
  port = 80;
  user = "";
  pass = "";
  String nickname = "";
  isActive = false;
  mock = false;
}

void PrinterSettings::fromJSON(JsonObjectConst settings) {
  type = settings[F("type")].as<String>();
  apiKey = settings[F("apiKey")].as<String>();
  server = settings[F("server")].as<String>();
  port = settings[F("port")];
  user = settings[F("user")].as<String>();
  pass = settings[F("pass")].as<String>();
  nickname = settings[F("nickname")].as<String>();
  isActive = settings[F("isActive")];
  mock = settings[F("mock")];
}

void PrinterSettings::toJSON(JsonObject settings) const {
  settings[F("type")] = type;
  settings[F("apiKey")] = apiKey;
  settings[F("server")] = server;
  settings[F("port")] = port;
  settings[F("user")] = user;
  settings[F("pass")] = pass;
  settings[F("nickname")] = nickname;
  settings[F("isActive")] = isActive;
  settings[F("mock")] = mock;
}

void PrinterSettings::logSettings() {
  Log.verbose(F("  ----- %s: %s"), nickname.c_str(), type.c_str());
  Log.verbose(F("  isActive: %T"), isActive);
  Log.verbose(F("  server: %s"), server.c_str());
  Log.verbose(F("  port: %d"), port);
  Log.verbose(F("  apiKey: %s"), apiKey.c_str());
  Log.verbose(F("  user: %s"), user.c_str());
  Log.verbose(F("  pass: %s"), pass.c_str());
  Log.verbose(F("  mock: %T"), mock);
}