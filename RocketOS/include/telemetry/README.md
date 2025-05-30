# 🛰 RocketOS Telemetry Subsystem

The RocketOS Telemetry system provides a robust and flexible way to **log data to an SD card**. It supports recording sensor values, internal state, or anything else your rocket tracks.

---

## 🔍 What Is This For?

Use this module to:

- Save flight data like altitude, acceleration, motor state, etc.
- Store logs in `.csv` format for easy viewing in Excel or Python
- Dynamically name and manage log files during operation

---

## 📦 What's Included

- `DataLog` – Core class that manages variable logging to SD
- `SDFile` – Lightweight file abstraction using SdFat
- `printToBuffer` – Type-safe way to convert values to strings
- Configuration header – For setting default file names

---

## ✅ When Should I Use This?

If your rocket or project needs to **store telemetry data**—especially over time or during flight—this module is for you.

It’s great for:

- Long-term flight logs
- Debugging problems after flight
- CSV export for analysis and plotting

---

## 🔧 How to Use

### 1. Include the Telemetry System

```cpp
#include "RocketOS_Telemetry.h"
using namespace RocketOS::Telemetry;
```

### 2. Define Variables

These are the variables you want to log:

```cpp
float altitude;
uint32_t state;
```

### 3. Create a DataLog

```cpp
SdFat sd; // SD interface

DataLog<128, float, uint32_t> logger(
    sd,
    DataLogSettings<float>{altitude, "altitude"},
    DataLogSettings<uint32_t>{state, "state"}
);
```

- The first template parameter (`128`) is the file name buffer size.
- Each variable needs a name (used as the CSV header).

### 4. Start a New Log

```cpp
logger.newFile();  // creates new .csv and writes headers
```

### 5. Log a Line

```cpp
logger.logLine();  // appends a new line with current variable values
```

### 6. Optional: Save or Close

```cpp
logger.save();   // flushes file contents (safe to call often)
logger.close();  // flushes and closes file
```

---

## ⚙️ Config Options (`RocketOS_Telemetry.cfg.h`)

- `RocketOS_Telemetry_SDDefaultFileName` – Used if no name is provided
- `RocketOS_Telemetry_DefaultTelemetryFileName` – Used for `DataLog`

You can change these if needed for your project.

---

## 🧠 Tips for New Users

- Logging doesn’t block unless the SD is slow or missing—handle errors!
- File writes can be **buffered** or **recorded directly**:
  - `SDFileModes::Buffer` saves memory and speed but is less safe on crash.
  - `SDFileModes::Record` writes immediately for safety.

```cpp
logger.setFileMode(SDFileModes::Buffer);
```

- Use short, clear variable names—they become your CSV headers.

---

## 🧪 Full Example

```cpp
#include "RocketOS_Telemetry.h"
using namespace RocketOS::Telemetry;

SdFat sd;
float alt = 0;
int mode = 0;

DataLog<128, float, int> logger(
    sd,
    DataLogSettings<float>{alt, "altitude"},
    DataLogSettings<int>{mode, "mode"}
);

void setup() {
    sd.begin();
    logger.newFile();
}

void loop() {
    alt += 1.0;
    logger.logLine();
    delay(1000);
}
```

---

## 🧩 Dependencies

- `RocketOSGeneral` – Common types and error codes
- `SdFat` – SD card access library (required)
- `snprintf()` – For converting values to strings safely

---

Let me know if you want CSV parsing help, variable type support, or startup file rotation. I can also generate wrapper utilities or example scripts.
