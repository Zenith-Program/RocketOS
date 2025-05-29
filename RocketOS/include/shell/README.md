# 🐚 RocketOS Shell Subsystem

The Shell subsystem is RocketOS’s **interactive command-line interface**. It lets you send commands to the system using a serial terminal (like Arduino Serial Monitor or PuTTY) and have them executed in real-time.

---

## 🔍 What Is This For?

The Shell allows:

- Real-time interaction with the system
- Running debug commands or changing parameters
- Displaying system status and logs
- Sending structured commands with arguments

Think of it as the brain’s "debug console" for your rocket.

---

## 📦 What's Included

- `Interpreter` – Main interface for parsing and executing commands
- `CommandList` – Tree-based command definitions
- `Command` – Describes a single shell command and its callback
- `Token` – Represents parts of a user command input
- `SerialInput` – Buffers serial input
- Config files – Set buffer sizes, baud rate, etc.

---

## ✅ When Should I Use This?

Use the shell if you want to:

- Add an easy-to-use **debug interface**
- Control or monitor your rocket **from a computer**
- Change parameters **without reprogramming the board**
- Provide teammates with **non-programming access** to diagnostics

---

## 🔧 How to Use

### 1. Include the Shell

```cpp
#include "RocketOS_Shell.h"
```

### 2. Define Commands

A command is made of:
- Name
- Argument types
- Callback function

```cpp
void sayHello(const RocketOS::Shell::Token* args) {
    Serial.println("Hello!");
}

RocketOS::Shell::Command sayHelloCommand = {"hello", "", sayHello};
```

Argument types are passed as a string:

| Code | Type             |
|------|------------------|
| `w`  | Word             |
| `s`  | Quoted String    |
| `u`  | Unsigned Integer |
| `i`  | Signed Integer   |
| `f`  | Float            |

Example with args:

```cpp
void repeat(const RocketOS::Shell::Token* args) {
    int count = args[0].getSignedData();
    for(int i = 0; i < count; ++i) Serial.println("Hi!");
}

RocketOS::Shell::Command repeatCommand = {"repeat", "i", repeat};
```

### 3. Build the Command Tree

Commands live in a tree structure using `CommandList`s.

```cpp
RocketOS::Shell::Command rootCommands[] = {sayHelloCommand, repeatCommand};
RocketOS::Shell::CommandList rootCommandList("", rootCommands, 2, nullptr, 0);
```

### 4. Set Up the Interpreter

```cpp
RocketOS::Shell::Interpreter shell(&rootCommandList);

void setup() {
    shell.init();
}

void loop() {
    shell.handleInput();
}
```

Type `>hello` or `>repeat 3` in the serial monitor.

---

## 🛠️ Config Options (`RocketOS_Shell.cfg.h`)

These control system limits and behavior:

- `RocketOS_Shell_SerialRxBufferSize`: Input buffer size (default: 256)
- `RocketOS_Shell_BaudRate`: Serial baud rate (default: 115200)
- `RocketOS_Shell_TokenBufferSize`: Max number of arguments (default: 32)
- `RocketOS_Shell_InterpreterCommandNameBufferSize`: Max command name size (default: 64)
- `RocketOS_Shell_CommandCallbackCaptureSize`: Size limit for lambdas with capture (default: 4 bytes)

---

## 🧠 Tips for New Users

- Start command lines with `>` (e.g. `>status check`)
- You can use **quoted strings** for arguments: `>log name "mission one"`
- Each command can have subcommands, like: `>sensor read pressure`
- Use `printAllCommands()` to see a full tree of registered commands

---

## 🧪 Example

```cpp
#include "RocketOS_Shell.h"
using namespace RocketOS::Shell;

void status(const Token*) {
    Serial.println("System is OK!");
}

Command commands[] = {
    {"status", "", status}
};

CommandList root("", commands, 1, nullptr, 0);
Interpreter shell(&root);

void setup() {
    shell.init();
}

void loop() {
    shell.handleInput();
}
```

Type `>status` in the serial terminal to run.

---

## 🧩 Dependencies

- `RocketOSGeneral` – Types and error codes
- Arduino’s `Serial` interface
- [inplace_function.h] – Allows small capturing lambdas

---

Need help adding a new command or building deeper command trees? Let me know and I can generate templates or examples.
