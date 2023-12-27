from pathlib import Path
import posixpath
# For more details on these options, run 'fbt -h'
FIRMWARE_ORIGIN = "Haisenteck"
# Default hardware target
TARGET_HW = 7
# Optimization flags
## Optimize for size
COMPACT = 1
## Optimize for debugging experience
DEBUG = 0
WORKFLOW_BRANCH_OR_TAG = "dev"
# Aggiungi il percorso relativo del file che contiene la versione
VERSION_FILE = "versione.txt"
# Controlla se il file esiste nella stessa cartella del firmware
if Path(VERSION_FILE).exists():
    # Leggi il contenuto del file e assegna il valore a UPDATE_VERSION_STRING
    with open(VERSION_FILE, 'r') as version_file:
        UPDATE_VERSION_STRING = version_file.read().strip()
else:
    # Imposta un valore predefinito nel caso il file non esista
    UPDATE_VERSION_STRING = "DEBUG"
# Suffix to add to files when building distribution
# If OS environment has DIST_SUFFIX set, it will be used instead
DIST_SUFFIX = "Haisenteck_" + UPDATE_VERSION_STRING
# Coprocessor firmware
#COPRO_OB_DATA = "scripts/ob.data"
COPRO_OB_DATA = "scripts/ob_custradio.data"
# Must match lib/stm32wb_copro version
COPRO_CUBE_VERSION = "1.18.0"
COPRO_CUBE_DIR = "lib/stm32wb_copro"
# Default radio stack
#COPRO_STACK_BIN = "stm32wb5x_BLE_Stack_light_fw.bin"
COPRO_STACK_BIN = "stm32wb5x_BLE_Stack_full_fw.bin"
# Firmware also supports "ble_full", but it might not fit into debug builds
COPRO_STACK_TYPE = "ble_full"
#COPRO_STACK_TYPE = "ble_light"
# Leave 0 to let scripts automatically calculate it
COPRO_STACK_ADDR = "0"
# If you override COPRO_CUBE_DIR on commandline, override this as well
COPRO_STACK_BIN_DIR = posixpath.join(COPRO_CUBE_DIR, "firmware")
# Supported toolchain versions
FBT_TOOLCHAIN_VERSIONS = (" 10.3.",)
OPENOCD_OPTS = [
    "-f",
    "interface/stlink.cfg",
    "-c",
    "transport select hla_swd",
    "-f",
    "${FBT_DEBUG_DIR}/stm32wbx.cfg",
    "-c",
    "stm32wbx.cpu configure -rtos auto",
]
SVD_FILE = "${FBT_DEBUG_DIR}/STM32WB55_CM4.svd"
# Look for blackmagic probe on serial ports and local network
BLACKMAGIC = "auto"
# Application to start on boot
LOADER_AUTOSTART = ""
FIRMWARE_APPS = {
    "default": [
        # Svc
        "basic_services",
        # Apps
        "main_apps",
        "system_apps",
        # Settings
        "settings_apps",
    ],
    "unit_tests": [
        "basic_services",
        "updater_app",
        "radio_device_cc1101_ext",
        "unit_tests",
    ],
}
FIRMWARE_APP_SET = "default"
custom_options_fn = "fbt_options_local.py"
if Path(custom_options_fn).exists():
    exec(compile(Path(custom_options_fn).read_text(), custom_options_fn, "exec"))