#!/usr/bin/env python3
"""Create a game-root install ZIP and a matching source ZIP."""

import hashlib
import struct
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
LOADER_HASH = "fa266e3513d02c08a1b808f28c10538a489eaffaa4b0707f7cc1066e71b5afd7"


def assert_x64_dll(path: Path) -> None:
    data = path.read_bytes()
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[:2] == b"MZ" and data[pe_offset:pe_offset + 4] == b"PE\0\0", path
    assert struct.unpack_from("<H", data, pe_offset + 4)[0] == 0x8664, path


def main() -> None:
    plugin = DIST / "InstantPrepare.asi"
    loader = DIST / "dinput8.dll"
    assert_x64_dll(plugin)
    assert_x64_dll(loader)
    if hashlib.sha256(loader.read_bytes()).hexdigest() != LOADER_HASH:
        raise ValueError("ASI loader differs from the checked x64 release")

    runtime = DIST / "InstantPrepare-15693-mod.zip"
    with ZipFile(runtime, "w", ZIP_DEFLATED) as out:
        binary_dir = "Bin/Win64MasterMasterSteamPGO/"
        out.write(plugin, binary_dir + plugin.name)
        out.write(loader, binary_dir + loader.name)
        out.write(ROOT / "README.md", "InstantPrepare-docs/README.md")
        out.write(ROOT / "LICENSE", "InstantPrepare-docs/LICENSE")
        out.write(ROOT / "third_party/minhook/LICENSE.txt",
                  "InstantPrepare-docs/MinHook-LICENSE.txt")
        out.write(ROOT / "third_party/Ultimate-ASI-Loader-LICENSE",
                  "InstantPrepare-docs/Ultimate-ASI-Loader-LICENSE.txt")

    source = DIST / "InstantPrepare-15693-source.zip"
    with ZipFile(source, "w", ZIP_DEFLATED) as out:
        for path in sorted(ROOT.rglob("*")):
            if (path.is_file() and DIST not in path.parents and
                    "__pycache__" not in path.parts and path.suffix != ".pyc"):
                out.write(path, "InstantPrepare-source/" + str(path.relative_to(ROOT)))

    for archive in (runtime, source):
        with ZipFile(archive) as check:
            if check.testzip() is not None:
                raise ValueError(f"Corrupt ZIP: {archive}")
        print(f"{archive}: {archive.stat().st_size} bytes")


if __name__ == "__main__":
    main()
