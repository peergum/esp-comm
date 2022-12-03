<img src="pictures/esp-comm-logo.png" width="200" alt="esp-comm logo"/>

This component is a collection of objects to ease Wifi comms in projects. It's using the ESP-IDF Framework from Espressif, but this code is not connected to the Espressif company otherwise. I'm a big fan and user of ESP chips, but that's all.

I created it in order not to keep reinventing the wheel each time a project needs IP features over Wifi, like a tiny web interface to set up a wifi password, TCP or UDP clients, UPnP, etc...

Most of the code is mine, possibly occasionally flawed, and UPnP is adapted from [TinyUPnP](https://github.com/ofekp/TinyUPnP) which was initially written for the Arduino framework.

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

## TO DO / Coming ASAP

- TCP Server -> 0% (should be easy though)
- UDP Server -> 0% (should be easy too)
- Web Server (with APIs/dynamic pages) -> 90% (needs some retouching)
- File Server -> 95% (needs a few changes)
- Firmware Updater -> 95% (needs a few changes)
- Captive Portal -> 90% (assemble the elements above to build a simple portal )
  
## Pending Issues

Please open relevant issues, with details. Priorities as:

1. Blocking fixes
2. Non-blocking fixes
3. Feature adjustments
4. New features

## Help, anyone?

I'm currently working alone on this, on my spare time. If you want to help, here's how:
- Help fix/extend/improve code, by submitting a pull request
- You could prefer to fork your own repo. That's fine with me, but don't come whining if this one keeps getting better and better ;-P You can be a team player, or not. Your choice, not mine. Also, sometimes people opinions and choices diverge, that's life ;)
- Star this repo to make it more "reputable"
- Donate? I haven't setup anything yet, but I will. In the meantime:
  - Listen to my music (search for _PeerGum_ on Spotify, Apple Music, Youtube Music, etc... ). Every listen brings me a tiny fraction of a cent. Better than nothing :D
  - Send ETH (or ERC-20) crypto to 0x989f7290d5227d9d6451ab813900f1feb8a11c7b (min. 0.00001 ETH)
  - Send BTC crypto to 3AAQHy983Bc9dYZ9WSrehKtg3fxGx3HEKe (Bitcoin Network, min. 0.00010 BTC)
  - Send LTC crypto to LYiDAwKN3xUeTqWNYnVThTtYgPNbKBDfcD (Litecoin Network, min. 0.01000 LTC)
  - Send me a virtual coffee (I like virtual Mint Tea or Chai too)
  - Be happy and enjoy using the code.