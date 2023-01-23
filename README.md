<img src="pictures/esp-comm-logo.png" width="200" alt="esp-comm logo"/>

**Disclaimer:** **This code is not connected to (and even less endorsed by) the Espressif company** - I'm a big fan and user of ESP chips, but that's all.

This component is a collection of objects to ease Wifi comms in projects, and it relies on the ESP-IDF Framework from Espressif. This code targets professional projects, or hobbyists who came to appreciate the ESP-IDF Framework.

I created it in order not to keep reinventing the wheel each time a project needs IP features over Wifi, like a tiny web interface to set up a wifi password, TCP or UDP clients, UPnP, etc...

Most of the code is mine, possibly occasionally flawed, and UPnP is adapted from [TinyUPnP](https://github.com/ofekp/TinyUPnP) which was initially written for the Arduino framework. If you like Arduino, great, I started on it too, but the obvious ease of use for newbies also has a bunch of downsides. Each one their own... Just don't come whining about a port to Arduino, I'm not interested: I suffered enough porting that UPnP part from Arduino to ESP-IDF, and I tend to walk forward, not backwards.

<hr/>

## Sponsors

A big thank you to our sponsors, with a reminder you guys get top priority on new requests or bug fixes...
- [3DprintNerd](https://github.com/3DprintNerd)

<hr/>

## Preamble

**This component is written for the ESP-IDF framework, version 5.0 and above**.

Please don't ask to make it work for versions under 5.0, I have no time for that at this point. If anyone wants to create and maintain a branch for previous versions of the framework, feel free to do it, but it will be a constant work porting the new features and kind of backward IMHO.

<hr/>

## Setup and usage

### Component installation

Clone the repository in the _components_ folder in your project, or add it there as a submodule, then add _esp-comm_ under the _REQUIRES_ directives

### Usage

Use the following classes as required:
- Wifi
- CaptivePortal
- WebServer (includes APIs, Firmware Updates...)
- TCPClient
- UDPClient
- UPnP (through Wifi class)
  
Additionally these classes can be of use, depending:
- IPAddress
- Timer

## Manual/Documentation

See [wiki](https://github.com/peergum/esp-comm/wiki)

## Examples

You can compile the example project right inside the example/basic folder (it uses a relative link to the component itself, using the _extra_component_dirs_ directive in the top _CMakeLists.txt_ file)

## To Do / Coming Soon...

- TCP Server -> 0% (should be easy though)
- UDP Server -> 0% (should be easy too)
- Web Server (with APIs/dynamic pages) -> done
- File Server -> done
- Firmware Updater -> 95% (needs a few changes)
- Captive Portal -> 90% (assemble the elements above to build a simple portal )
- Documentation
  
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
  - Send crypto (confirm rates before depositing - **values under minimum crypto will be lost**):

    <table>
    <tr><th>Crypto</th><th>Address</th><th align="center">Min.Amount</th><th align="center">~ Rates Dec.28, 2022</tr>
    <tr><td>ETH/ERC-20</td><td>0x989f7290d5227d9d6451ab813900f1feb8a11c7b</td><td align="center">0.00001</td><td align="center">1 ETH ~ USD 1,190</td></tr>
    <tr><td>BTC</td><td>3AAQHy983Bc9dYZ9WSrehKtg3fxGx3HEKe</td><td align="center">0.0001</td><td align="center">1 BTC ~ USD 16,528</td></tr>
    <tr><td>LTC</td><td>LYiDAwKN3xUeTqWNYnVThTtYgPNbKBDfcD</td><td align="center">0.01</td><td align="center">1 LTC ~ USD 66</td></tr>
    <tr><td>ATOM</td><td>cosmos1p8actjzzmxxr6dvwx3q05ykvpxyqjd2j4jn8pe</td><td align="center">1.00</td><td align="center">1 ATOM ~ USD 9</td></tr>
    <tr><td>DASH</td><td>XoxPsd6U2xVG7gBzirpuWDbDtq5bfHdDbE</td><td align="center">0.01</td><td align="center">1 DASH ~ USD 43</td></tr>
    </table>

  - Send me a virtual coffee (I like virtual Mint Tea or Chai too)
  - Be happy and enjoy using the code.
