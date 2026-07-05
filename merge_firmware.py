#
# Post-build: merge bootloader + partitions + app into a single flashable
# image at offset 0, dropped into docs/firmware/TerminalX.bin for the web flasher.
#
Import("env")
import os


def merge_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    proj = env.subst("$PROJECT_DIR")

    out_dir = os.path.join(proj, "docs", "firmware")
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, "TerminalX.bin")

    fw = os.path.join(build_dir, "firmware.bin")
    boot = os.path.join(build_dir, "bootloader.bin")
    part = os.path.join(build_dir, "partitions.bin")

    py = env.subst("$PYTHONEXE")
    esptool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
    esptool_py = os.path.join(esptool_dir, "esptool.py")

    if os.path.isfile(esptool_py):
        base = '"%s" "%s"' % (py, esptool_py)
    else:
        base = '"%s" -m esptool' % py

    flash_mode = env.subst("$BOARD_FLASH_MODE") or "qio"

    cmd = (
        '%s --chip esp32s3 merge_bin -o "%s" '
        "--flash_mode %s --flash_freq 80m --flash_size 8MB "
        '0x0 "%s" 0x8000 "%s" 0x10000 "%s"'
        % (base, out, flash_mode, boot, part, fw)
    )

    print("[TerminalX] merging web-flasher image -> " + out)
    env.Execute(cmd)

    write_build_info(proj, out_dir, out)


def write_build_info(proj, out_dir, out):
    import json, datetime, subprocess

    version = "?"
    try:
        with open(os.path.join(proj, "src", "config.h")) as f:
            for ln in f:
                if "TERMINALX_VERSION" in ln and '"' in ln:
                    version = ln.split('"')[1]
                    break
    except Exception:
        pass

    commit = ""
    try:
        commit = (
            subprocess.check_output(["git", "-C", proj, "rev-parse", "--short", "HEAD"])
            .decode()
            .strip()
        )
    except Exception:
        pass

    info = {
        "version": version,
        "date": datetime.datetime.now().strftime("%Y-%m-%d %H:%M"),
        "commit": commit,
        "size": os.path.getsize(out) if os.path.isfile(out) else 0,
    }
    with open(os.path.join(out_dir, "build-info.json"), "w") as f:
        json.dump(info, f, indent=2)
    print("[TerminalX] build v%s (%s) %s" % (version, info["date"], commit))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)
