# FlexiCAN
The FlexiCAN device, developed by VinCSS, is an innovative automotive CAN solution designed for flexibility and ease of use. It features a unique software-controlled hot-swappable semiconductor switching matrix, enabling users to dynamically reconfigure the CAN_H and CAN_L pins of the CAN bus to any pin on the OBD-II port. This eliminates the need for proprietary connection cables, allowing seamless routing to non-standard or hidden CAN bus ports present on the OBD-II interface. Key highlights include software-configurable termination resistors (120Ω), support for CANFD up to 5 Mbit/s, and compatibility with both 11-bit (CAN 2.0A) and 29-bit (CAN 2.0B active) identifiers. The device supports high-speed CAN (ISO 11898-2 compliant) up to 1 Mbit/s and is fully compatible with Linux and CANSocket for robust integration. Its plug-and-play installation and fast, flexible command execution make it user-friendly, while the upcoming silent mode ensures non-intrusive analysis capabilities, making it ideal for automotive diagnostics and research applications.

Full detail please check the document: [FlexiCAN-User-Manual-English.pdf](docs/FlexiCAN-User-Manual-English.pdf)

![FlexiCAN](https://github.com/VinCSS-Public-Projects/FlexiCAN/blob/main/HW%20design/FlexiCAN_logo.png?raw=true)

# Hardware

![Schema](https://github.com/VinCSS-Public-Projects/FlexiCAN/blob/main/HW%20design/Schema.png?raw=true)

# Firmware

There are two controllers in FlexiCAN: RP2040 and STM32.

RP2040 is to control FlexiCAN & OBD-II switch.

STM32 is to control CAN interface

Source code of STM32 we use from this link https://github.com/candle-usb/candleLight_fw

# License 

FlexiCAN is fully open source.

All software, unless otherwise noted, is dual licensed under Apache 2.0 and MIT. You may use FlexiCAN software under the terms of either the Apache 2.0 license or MIT license.

All hardware, unless otherwise noted, is dual licensed under CERN and CC-BY-SA. You may use FlexiCAN hardware under the terms of either the CERN 1.2 license or CC-BY-SA 4.0 license.

