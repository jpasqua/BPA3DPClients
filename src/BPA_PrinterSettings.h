//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoJson.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------

#ifndef PrinterSettings_h
#define PrinterSettings_h

static constexpr const char* Type_Octo = "OctoPrint";
static constexpr const char* Type_Duet = "Duet3D";

class PrinterSettings {
public:
  PrinterSettings();
  void init();

  void fromJSON(const JsonObjectConst settings);
  void toJSON(JsonObject settings) const;
  void logSettings();

  String type;    // Must be either "OctoPrint" or "Duet3D"
  String apiKey;
  String server;
  int port;
  String user;
  String pass;
  String nickname;
  bool isActive;
  bool mock;
};

#endif // PrinterSettings_h