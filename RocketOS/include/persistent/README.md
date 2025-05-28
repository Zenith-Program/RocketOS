# 🚀 RocketOS Persistent Data System

This module handles **persistent storage** in RocketOS—meaning it saves key variables in non-volatile memory (EEPROM), so data can be restored even after the rocket is powered off or reset.

---

## 🔍 What Is This For?

When your rocket turns off (intentionally or due to an error), **you don’t want to lose important info** like flight state, last known altitude, or log filenames.

This system:

- Automatically **stores** values in EEPROM.
- **Restores** them when the rocket starts again.
- **Detects changes** to what’s being saved and resets stored values if needed (to avoid reading mismatched data).

---

## 📦 What's Included

This module includes the following core components:

- `EEPROMBackup` – The main class that manages persistent variables.
- `EEPROMSettings` – A helper structure to set up each variable to be stored.
- `EEPROMValue` – Manages reading/writing a single variable.
- `RocketOS_Persistent.cfg.h` – Configuration file to set memory alignment, size, etc.

---

## ✅ When Should I Use This?

You should use this module if you want to **save and restore** any of the following:

- Rocket state (`e.g.`, ready, armed, coasting)
- Last known sensor values
- Log file names
- User-defined settings (like thresholds)

---

## 🔧 How to Use

### 1. Include the Header

In your `.cpp` file:

```cpp
#include "RocketOS_Persistent.h"
```

---

### 2. Define Your Variables

These are the variables you want to persist:

```cpp
uint32_t programState;
float maxAltitude;
std::array<char, 32> logFileName;
```

---

### 3. Create a Backup Object

Set up the variables for persistence:

```cpp
using namespace RocketOS::Persistent;

EEPROMBackup<uint32_t, float, std::array<char, 32>> eeprom{
    EEPROMSettings<uint32_t>{programState, 0, "state"},
    EEPROMSettings<float>{maxAltitude, 0.0f, "altitude"},
    EEPROMSettings<std::array<char, 32>>{logFileName, "flight1.csv", "log file"}
};
```

---

### 4. Restore on Startup

Call this once when the program starts:

```cpp
auto result = eeprom.restore();
if(result.data) {
    // Memory layout changed, using defaults
}
```

---

### 5. Save When Needed

You can save values any time:

```cpp
eeprom.save();
```

It only writes to EEPROM if the value changed, so you can call it freely without wearing out the memory.

---

## ⚙️ How It Works Internally

- Each variable is stored at an **aligned EEPROM address**.
- A **hash** is computed from the ID strings of all variables.
- If the stored hash doesn’t match the new one on boot, defaults are used instead of invalid old data.
- `EEPROM.put` and `EEPROM.get` handle low-level writing/reading.

---

## 🧠 Tips for New Users

- 🟢 Use **trivially copyable types**: `int`, `float`, `std::array`, etc. Don’t use things like `std::string` or custom classes.
- 🟡 Be careful with array sizes: if you change the size or type, the hash will change (which is good—it keeps memory safe).
- 🔴 Don’t forget to call `restore()` at startup!

---

## 🧪 Example

Here’s a simple full example:

```cpp
#include "RocketOS_Persistent.h"

using namespace RocketOS::Persistent;

// variables
uint32_t mode;
float lastAltitude;
std::array<char, 20> filename;

// persistent backup setup
EEPROMBackup<uint32_t, float, std::array<char, 20>> eeprom{
    EEPROMSettings<uint32_t>{mode, 0, "mode"},
    EEPROMSettings<float>{lastAltitude, 0.0f, "last alt"},
    EEPROMSettings<std::array<char, 20>>{filename, "log.csv", "file name"}
};

void setup() {
    auto result = eeprom.restore();
    if(result.data) {
        // Memory layout changed: using defaults
    }
}

void loop() {
    // maybe after landing or after significant updates
    eeprom.save();
}
```

---

## 🛠️ Config Options (`RocketOS_Persistent.cfg.h`)

- `RocketOS_Persistent_EEPROMBaseAdress`: Where EEPROM storage starts (default: 0)
- `RocketOS_Persistent_EEPROMAlignment`: Align variables (usually 4 bytes for Teensy)
- `RocketOS_Persistent_EEPROMMaxSize`: Total EEPROM bytes available (default: 1024)

Change these only if you know what you're doing.

---

## 🧩 Dependencies

This module uses:

- `RocketOSGeneral` – Defines data types and error handling
- Arduino's `EEPROM.h` – Hardware abstraction for EEPROM access
