# Thermal Fusion PRO
[![TechMasterEvent](https://img.shields.io/badge/TechMasterEvent-Project_Page-blue)](https://techmasterevent.com/project/thermal-fusion-pro-advanced-sensor-fusion-camera-based-on-esp32-cam-esp32-c3)

Advanced Sensor Fusion Camera based on ESP32-CAM &amp; ESP32-C3
## Bill of Materials (BOM)

To build the **Thermal Fusion PRO**, you will need the following components.

| Component               | Quantity  | Description |
| :---                    | :---:     | :--- |
| **ESP32-CAM**           | 1         | Master Hub & Web Server |
| **ESP32-C3 Super Mini** | 1 | Sensor Acquisition Node |
| **MLX90640**            | 1 | 32x24 Thermal Sensor Array |
| **TP4056**              | 1 | Li-Ion Battery Charger with protection |
| **MT3608**              | 1 | 5V DC-DC Boost Converter |
| **18650 Cell**          | 1 | Lithium-Ion rechargeable battery |
| **Power Switch**        | 1 | Tactile button with LED ring |
| **Resistors**           | 4 | 2x 4.7kΩ (I2C), 2x 100kΩ (Battery Divider) |
| **Capacitors**          | 3 | 1x 100µF, 2x 100nF |

---

## 🛠 Installation & Setup

### 1. Hardware Assembly
- Follow the high-resolution **schematic** located in the `/Hardware/SCH_Schematic` file.
- Print the enclosure using the provided **.3mf files** in `/Hardware/3D_Models`. These files contain modern manufacturing data and are compatible with most slicers (PrusaSlicer, BambuStudio, Cura).

### 2. Firmware Installation
1. Install **Arduino IDE** and the **ESP32 Board Manager** (v3.0.0 or higher recommended).
2. Install the following libraries via Library Manager:
   - `Adafruit_MLX90640`
3. **Flash the Sensor Node:** Open `Firmware/ESP32_C3_Sensor/` and upload to your ESP32-C3.
4. **Flash the Master Hub:** Open `Firmware/ESP32_CAM_AP/`, select **AI Thinker ESP32-CAM** board, and upload.

### 3. Usage
- Turn on the device using the physical power button.
- Connect your smartphone/PC to the WiFi network: `PRO-THERMAL-CAM`.
- Open a web browser and go to `http://thermalcam.local` or `192.168.4.1`.

### 📱 Enhanced Mobile Experience
For a professional, full-screen diagnostic experience without browser address bars:
- **iOS (Safari):** Tap the **Share** button and select **"Add to Home Screen"**.
- **Android (Chrome):** Tap the **Three-dot menu** and select **"Install app"** or **"Add to Home screen"**.

This will launch the Thermal Fusion PRO interface as a standalone application, providing a clean and immersive HUD.

---

## 📐 3D Printing Notes
The enclosure is optimized for PLA or PETG printing.
- **File format:** .3mf 
- **Supports:** Required for the main handle body.
- **Infill:** 20%-30% recommended for durability.

---
## 🏆 Contest Entry
This project is an official entry for the **Master of Espressif** contest.
For a detailed high-level overview, photos, and community discussion, visit the project page:
 [**Thermal Fusion PRO on TechMasterEvent**](https://techmasterevent.com/project/thermal-fusion-pro-advanced-sensor-fusion-camera-based-on-esp32-cam-esp32-c3)
