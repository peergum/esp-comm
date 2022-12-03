# esp-comm

This component is a collection of objects to ease Wifi comms in projects.

I created it in order not to keep reinventing the wheel each time a project needs IP features over Wifi, like a tiny web interface to set up a wifi password, TCP or UDP clients, UPnP, etc...

Most of the code is mine, possibly occasionally flawed, and UPnP is adapted from TinyUPnP.

## Setup and usage

### Component installation

Clone the repository in the _components_ folder in your project, or add it there as a submodule, then add _esp-comm_ under the _REQUIRES_ directives

### Usage

Use the following classes as required:
- TCPClient
- UDPClient
- UPnP
- Wifi
  
Additionally these classes can be of use, depending:
- IPAddress
- Timer

## Manual/Documentation

(Coming soon)

## Examples

You can compile the example project right inside the example/basic folder (it uses a relative link to the component itself, using the _extra_component_dirs_ directive in the top _CMakeLists.txt_ file)
