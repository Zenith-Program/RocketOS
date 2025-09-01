# NASA Student Launch Payload – Teensy 4.1 Flight Software

This repository contains the embedded flight software for the FAMU-FSU College of Engineering's **2024-2025 NASA Student Launch Competition** payload experiment. The project was developed using [PlatformIO](https://platformio.org/) and written for the **Teensy 4.1** microcontroller. It encompasses flight tracking algorithims, sensor integration, data logging, VHF APRS messaging, software in the loop testing, and a command line interface.

## Overview

- **Target Platform**: Teensy 4.1 (ARM Cortex-M7)
- **Environment**: PlatformIO (with Arduino framework)
- **Purpose**: Autonomous flight payload built for the 2024-2025 [USLI](https://www.nasa.gov/stem/studentlaunch/home/index.html) competition.

> This repository was developed by [Nathan Hardie](https://github.com/Nate-4-4) as a member of the Zenith Program at Florida State University for participation in the NASA Student Launch Initiative.

---

## Repository Structure

```bash
.
├── include/            # Header files
├── lib/                # External libraries - Adafurit BNO055, Adafruit BNO080, Uravu LabsMS5607
├── src/                # Source code
│   ├── main.cpp        # Main entry point
│   └── ...             # Other modules
├── platformio.ini      # PlatformIO configuration (incorporates BNO055 lib using PIO library manager)
└── README.md           # You're here
```

## Required Hardware
The payload system relies on external hardware: 
- **MS5706 Barometric Altimeter**: Connected to Teensy 4.1's I2C bus #1. It an be found [here](https://www.parallax.com/product/altimeter-module-ms5607/). Library implementation found [here](https://github.com/UravuLabs/MS5607).
- **BNO055 IMU**: Connected to Teensy 4.1's I2C bus #2. It can be found [here](https://www.adafruit.com/product/2472). Library implementation found [here](https://github.com/adafruit/Adafruit_BNO055).
- **BNO080 IMU**: Four of these (one per STEMnaut) are used. Two on I2C bus #2 and one on I2C bus #1. It can be found [here](https://www.ceva-ip.com/product/fsm-9-axis-module/). Library implementation found [here](https://github.com/adafruit/Adafruit_BNO08x).
- **LightAPRS Transmitter Module**: Used as a secondary altimeter, battery monitor, GPS (not fully implemented), and VHF transmitter. Connected to Teensy 4.1's I2C bus #3. It can be found [here](https://www.qrp-labs.com/lightaprs.html).

# Future Teams' Reference
The PayloadOS program can run with just a Teensy 4.1 board with limited functionality. If you are looking for example code to get started with or use as reference, I'd reccomend you look at the [RocketOS](https://github.com/Zenith-Program/RocketOS) repository instead. It contains source code, tools, and better documentation for the airbrakes system, which in many ways is a refined version of the payload system. 





