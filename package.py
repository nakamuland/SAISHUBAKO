# PlatformIO/SCons hook: create a complete flash image after a successful build.
Import("env")
from pathlib import Path
import subprocess

# Keep build-machine directory names out of embedded __FILE__ strings.
for variable, label in [
    ("$PROJECT_DIR", "."),
    ("$PROJECT_PACKAGES_DIR", "platformio-packages"),
    ("$PROJECT_LIBDEPS_DIR", "platformio-libdeps"),
]:
    directory = Path(env.subst(variable)).absolute().as_posix()
    env.Append(CCFLAGS=["-ffile-prefix-map=" + directory + "=" + label])


def package_image(source, target, env):
    build = Path(env.subst("$BUILD_DIR"))
    platform = env.PioPlatform()
    esptool = Path(platform.get_package_dir("tool-esptoolpy")) / "esptool.py"
    framework = Path(platform.get_package_dir("framework-arduinoespressif32"))
    dest = Path(env.subst("$PROJECT_DIR")) / "saishubako-cardputer-full.bin"
    subprocess.check_call([
        env.subst("$PYTHONEXE"), str(esptool), "--chip", "esp32s3", "merge_bin",
        "-o", str(dest), "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "8MB",
        "0x0", str(build / "bootloader.bin"),
        "0x8000", str(build / "partitions.bin"),
        "0xe000", str(framework / "tools" / "partitions" / "boot_app0.bin"),
        "0x10000", str(build / "firmware.bin"),
    ])
    print("Full flash image: " + str(dest))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", package_image)
