"""PlatformIO pre-script: inject GIT_HASH and BUILD_DATE as preprocessor defines.

These show up in firmware as the strings returned by util_version_string()
(see src/util.cpp). The script runs on every `pio run` so BUILD_DATE always
reflects the actual build moment; changing CPPDEFINES forces a rebuild of any
translation unit that references the macros.
"""
import subprocess
from datetime import datetime

Import("env")  # noqa: F821 — provided by SCons / PlatformIO


def _git_short_hash() -> str:
    try:
        commit = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL,
        ).strip().decode()
        dirty = subprocess.check_output(
            ["git", "status", "--porcelain"],
            stderr=subprocess.DEVNULL,
        ).strip()
        return commit + ("-dirty" if dirty else "")
    except Exception:
        return "unknown"


git_hash = _git_short_hash()
build_date = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

env.Append(CPPDEFINES=[                       # noqa: F821
    ("GIT_HASH", env.StringifyMacro(git_hash)),
    ("BUILD_DATE", env.StringifyMacro(build_date)),
])

print(f"[inject_build_info] GIT_HASH={git_hash}  BUILD_DATE={build_date}")
