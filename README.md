# Control Base - ESP32 (FreeRTOS)

Repository for the control base of the game board project, using the ESP32 with FreeRTOS.

## Getting Started

### Prerequisites
To work with this project you need to install the following VSCode extensions:
- [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide): 
    Quick start guide can be found [here](https://docs.platformio.org/en/latest/integration/ide/vscode.html#quick-start).

- [Wokwi Simulator](https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode):
    Quick start guide can be found [here](https://docs.wokwi.com/vscode/getting-started/).

### Import the project

- Clone this repository.
- Click on the PlatformIO icon in the left sidebar.
- Click on the "Open Project" button and select the peoject (folder tha contains the `platformio.ini` file).

### Install the dependencies
Dependencies will be automatically installed when you build the project for the first time, if not, you can install them manually:
- Click on the PlatformIO icon in the left sidebar. Or the home icon in the bottom bar (appears after starting the extension).

- Click on the "Library" button in the PlatformIO sidebar.
- Search for the following libraries and install them:
  - `LiquidCrystal_I2C` by Frank de Brabander

### Wifi Configuration
- When cloning the project, a file `src/config_example.h` will be created.
- Create a copy of this file and rename it to `src/config.h`.
- Fill in the `WIFI_SSID` and `WIFI_PASSWORD` fields with your wifi credentials.
- This new file is already included in the `.gitignore` file, so it will not be commited to the repository.
- If you want to use the Wokwi Simulator, you can use the following credentials:
    - `"Wokwi-GUEST"`
    - `""`

## Running the project
- Click on the "Build" button (checkmark icon) to compile your project.

- If using a physical ESP32:
    - Connect your ESP32 board to your computer.
    - Click on the "Upload" button (arrow pointing to the right) to upload the firmware to your board.

- If using Wokwi Simulator:
    - Press F1 and choose "Wokwi: Sart Simulator" to start the simulation. 

        Or you can open the `diagram.json` file and click "Start Simulation" on the top left corner (play icon).


## Project Structure

- `src/main.cpp`: Main source code file
- `diagram.json`: Wokwi diagram configuration. It's used to make the graphical simulation.
- `wokwi.toml`: Wokwi project configuration.


## Modifying the Wokwi Diagram

To modify the `diagram.json` file, which requires a paid plan in Wokwi for VS Code:

- Open the shared [Wokwi project](https://wokwi.com/projects/412546736065333249)
- Make your desired changes to the diagram in this web-based Wokwi project.
- Copy the updated `diagram.json` content.
- Paste the copied content into the `diagram.json` file in your local project.