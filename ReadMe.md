# FlexiCAN
The FlexiCAN device, developed by VinCSS, is an innovative automotive CAN solution designed for flexibility and ease of use. It features a unique software-controlled hot-swappable semiconductor switching matrix, enabling users to dynamically reconfigure the CAN_H and CAN_L pins of the CAN bus to any pin on the OBD-II port. This eliminates the need for proprietary connection cables, allowing seamless routing to non-standard or hidden CAN bus ports present on the OBD-II interface. Key highlights include software-configurable termination resistors (120Ω), support for CANFD up to 5 Mbit/s, and compatibility with both 11-bit (CAN 2.0A) and 29-bit (CAN 2.0B active) identifiers. The device supports high-speed CAN (ISO 11898-2 compliant) up to 1 Mbit/s and is fully compatible with Linux and CANSocket for robust integration. Its plug-and-play installation and fast, flexible command execution make it user-friendly, while the upcoming silent mode ensures non-intrusive analysis capabilities, making it ideal for automotive diagnostics and research applications.

![FlexiCAN](https://github.com/VinCSS-Public-Projects/FlexiCAN/blob/main/2.hardware/FlexiCAN_logo.png?raw=true)

# Key Features

• Flexibly Configure the Connection Between OBD-II and the CAN Device

• Software-configurable termination resistor (120Ω).

• Fast and flexible access to commands through COM

• Quick and straightforward plug-and-play installation.

• Supports CAN FD, with speeds up to 5 Mbit/s.

• Compatible with both 11-bit (CAN 2.0A) and 29-bit (CAN 2.0B active) identifiers.

• Fully compatible with Linux and CAN Socket.

• High-speed CAN connection (ISO 11898-2 compliant) with data rates up to 1 Mbit/s.

• Silent mode support for analysis tools – allows monitoring of the bus without interference (coming soon).

Full detail please check the document: [FlexiCAN-User-Manual-English.pdf](4.docs/FlexiCAN-User-Manual-English.pdf)
# Community & Support

We welcome contributions from the community! Please follow these steps:

1. Fork the repository.

2. Create a new branch (git checkout -b feature-branch-name).

3. Commit your changes (git commit -m "Description of changes").

4. Push to the branch (git push origin feature-branch-name).

5. Open a pull request.

If you encounter any problems, please open an issue. We will check and response as fast as possible.

## Hardware

![Schema](https://github.com/VinCSS-Public-Projects/FlexiCAN/blob/main/2.hardware/Schema.png?raw=true)

## Firmware

There are two controllers in FlexiCAN: RP2040 and STM32.

RP2040 is to control FlexiCAN & OBD-II switch.

STM32 is to control CAN interface

Source code of STM32 we use from this link https://github.com/candle-usb/candleLight_fw


# License 

FlexiCAN is fully open source.

All software, unless otherwise noted, is dual licensed under Apache 2.0 and MIT. You may use FlexiCAN software under the terms of either the Apache 2.0 license or MIT license.

All hardware, unless otherwise noted, is dual licensed under CERN and CC-BY-SA. You may use FlexiCAN hardware under the terms of either the CERN 1.2 license or CC-BY-SA 4.0 license.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Acknowledgment

FlexiCAN is built upon the foundation of many other excellent open-source projects. We would like to express our gratitude to the following contributors:

Thank you [@sakumisu](https://github.com/sakumisu) for the projects:
- [CherryShell](https://github.com/cherry-embedded/CherrySH), a tiny shell specifically designed for embedded applications.
- [CherryUSB](https://github.com/cherry-embedded/CherryUSB), a tiny, beautiful, and portable USB host and device stack for embedded systems with USB IP.

Thank you [@Egahp](https://github.com/Egahp) for the [CherryRingBuffer](https://github.com/cherry-embedded/CherryRB) project, an efficient and easy-to-use ring buffer, especially with DMA support

Thank you [@marckleinebudde](https://github.com/marckleinebudde) and the [candleLight](https://github.com/candle-usb/candleLight_fw) development team for creating such an incredibly useful project



# Ethical and Responsible Use

This tool, FlexiCAN, is developed for educational and ethical testing purposes only.  Any misuse or illegal use of this tool is strictly prohibited. The creator of FlexiCAN  assumes no liability and is not responsible for any misuse or damage caused by this tool. 
Users are required to comply with all applicable laws and regulations in their jurisdiction regarding network testing and ethical hacking.
