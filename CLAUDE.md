# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Development Commands

### Core PlatformIO Commands
- **Build**: `pio run` - Compiles the firmware
- **Flash**: `pio run --target upload` - Uploads firmware to connected ESP32-S3 device
- **Erase and Flash**: `pio run --target erase` then `pio run --target upload` - Use when device won't boot to correct partition
- **Monitor**: `pio device monitor` - Serial monitor for debugging (use RTS=0, DTR=0)

### Pre-build Scripts
The build automatically runs these scripts:
- `htmlconvert.py` - Inlines HTML/CSS/JS from `html/` directory 
- `ttf2c.py` - Converts TTF fonts to C arrays
- `png2c.py` - Converts PNG sprites to C arrays

### Hardware Reset Sequence
If device is unresponsive:
1. Hold **▼ (Page down/D0)** button
2. Press **Reset** button
3. Release **▼ (Page down/D0)**
Device enters bootloader mode for reflashing.

## Architecture Overview

### Hardware Platform
- **Target**: Adafruit ESP32-S3 Reverse TFT Feather
- **Display**: 240x135 pixel TFT with LVGL v9.2.2
- **Input**: 3 buttons (Up, Down/Boot, Center)
- **Storage**: Dual-partition OTA system with NVS config storage

### Multi-Core Task Architecture
**CRITICAL**: Strict core isolation prevents crashes. UI can ONLY be updated from Core 1.

**Core 0 (Protocol CPU)**:
- `wifiTaskFunction` - WiFi operations
- `portalTaskFunction` - Web server
- `insightTaskFunction` - PostHog API calls
- `neoPixelTaskFunction` - LED control
- `rssTaskFunction` - RSS feed parsing

**Core 1 (Application CPU)**:
- `DisplayInterface::tickTask` - LVGL timing/animations
- `lvglHandlerTask` - UI rendering and input handling

### Event-Driven Communication
- **EventQueue**: Thread-safe messaging between cores
- **Rule**: Never call LVGL functions from Core 0 tasks
- **Pattern**: Core 0 publishes events → Core 1 consumes and updates UI

### Card System Architecture
- **CardNavigationStack**: Manages scrolling navigation with animations
- **CardController**: Handles card lifecycle and configuration
- **Card Types**: Extensible system with factory pattern registration
- **Configuration**: Dynamic card instances managed through web UI

## Key Components

### ConfigManager
- Persistent storage using ESP32 NVS (Non-Volatile Storage)
- Stores WiFi credentials, PostHog API keys, card configurations
- Thread-safe with event notifications

### Web Portal (CaptivePortal)
- Served from Core 0 at device IP address
- Budget: Max 100KB total size (currently ~18KB)
- All assets must be local (no external dependencies)
- Accessed via QR code on first boot or device IP

### Built-in Card Types
- **ProvisioningCard**: WiFi setup and device status
- **InsightCard**: PostHog analytics visualization  
- **FriendCard**: Max the hedgehog mascot
- **NewsletterCard**: RSS feed reader (recent addition)

### Adding New Card Types
1. Add enum to `CardType` in `src/ui/CardController.h`
2. Create card class implementing LVGL UI
3. Register in `CardController::initializeCardTypes()` with factory lambda
4. Web UI automatically discovers and presents new types

## Development Guidelines

### Core Isolation Rules
- **UI Updates**: Only from Core 1 (UI task)
- **Cross-Core Communication**: Always use EventQueue
- **Data Sharing**: Deep copy data when passing between cores
- **LVGL Calls**: Never from Core 0 tasks

### Memory Management
- Device has PSRAM enabled (`-DARDUINOJSON_ENABLE_PSRAM=1`)
- Use PSRAM for large allocations
- Watch for memory leaks in card lifecycle

### AI Development Considerations
- LLMs often miss multi-threaded embedded constraints
- Always verify UI updates happen on correct core
- Review for "LLM slop" (unused variables, unnecessary delays)
- Follow existing patterns for EventQueue usage

## File Structure

### Core System
- `src/main.cpp` - Entry point and task setup
- `src/SystemController.*` - System state management
- `src/ConfigManager.*` - Persistent configuration
- `src/EventQueue.*` - Cross-core communication

### Hardware Abstraction
- `src/hardware/DisplayInterface.*` - LVGL display management
- `src/hardware/WifiInterface.*` - WiFi operations
- `src/hardware/Input.h` - Button handling
- `src/hardware/NeoPixelController.*` - LED control

### UI System
- `src/ui/CardController.*` - Card management
- `src/ui/CardNavigationStack.*` - Navigation and animations
- `src/ui/CaptivePortal.*` - Web configuration interface
- `src/ui/[CardName]Card.*` - Individual card implementations

### PostHog Integration
- `src/posthog/PostHogClient.*` - API client
- `src/posthog/RssClient.*` - RSS feed parsing  
- `src/posthog/parsers/InsightParser.*` - Data parsing

### Build Assets
- `include/fonts/` - Embedded font files
- `include/sprites/` - Graphics assets
- `html/` - Web portal source files
- `partitions.csv` - Flash memory layout

## Testing and Validation

### Power Management
- Deep sleep: Hold ● + ▼ for 2 seconds
- Wake: Press reset tab

### Common Issues
- **Partition Boot Failure**: Use "Erase Flash and Upload" from PlatformIO
- **UI Crashes**: Check for LVGL calls from wrong core
- **Memory Issues**: Monitor PSRAM usage and object lifecycle

### Serial Debugging
- Monitor filters include `esp32_exception_decoder`
- Debug level set to 5 (`CORE_DEBUG_LEVEL=5`)
- Use monitor_rts=0, monitor_dtr=0 for stable connection