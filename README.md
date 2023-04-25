# BPAUtils

A collection of client classes for 3D Printer status info. These allow one to get information about a printer and the state of a 3D print.

Clients are available for printers connected via:

* BPA_OctoClient: Connects to printers being controlled by [Octoprint](https://github.com/OctoPrint/OctoPrint) 
* BPA_DuetClient: Connects to printers being controlled by [RepRapFirmware by Duet3D](https://github.com/Duet3D/RepRapFirmware).
* BPA_MockPrintClient: A mock client that can be useful for testing purposes
* BPA_PrintClient: The base class for the concrete client classes

The library also contains a utility class called PrinterGroup. It encapsulates the notion of a group of printers (PrintClients) and helps to manage them more easilt. It also provides a DataSupplier that is compatible with the [WebThing framework](https://github.com/jpasqua/WebThing), but can also be used independently.

<img src="doc/images/WebThing_Logo_256.png"  width="256">
<img src="doc/images/OctoPrint.png"  width="256">
<img src="doc/images/Duet3D_Compatible_Logo_v1.0.png"  width="128">
