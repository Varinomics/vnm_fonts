#!/usr/bin/env python3

# Installs this repository into a staging directory and asserts the licences
# arrived. All ten shipped faces are OFL-1.1, Apache-2.0 or the Ubuntu Font
# Licence, and every one of those requires the licence and copyright notice to
# accompany copies. A product that ships the fonts and not the texts is the
# failure this checks for, and it is the state the consumer repositories were
# left in when the fonts moved here and their own install rules went away.
#
#   python tests/test_installed_licenses.py --build-dir <dir> --prefix <dir>
#
# Every licence the manifests name must be present under the install doc
# directory with its recorded digest, and THIRD_PARTY_NOTICES.md must be there
# and match the source, since it is what points a reader at the texts.

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys

try:
    import tomllib as toml_reader
except ModuleNotFoundError:  # Python 3.10 and earlier
    try:
        import tomli as toml_reader
    except ModuleNotFoundError:
        sys.stderr.write(
            "FAIL this check needs a TOML reader: Python 3.11 or later provides tomllib, "
            "otherwise install tomli.\n")
        raise SystemExit(1)


REPOSITORY_ROOT    = Path(__file__).resolve().parents[1]
MANIFEST_DIRECTORY = REPOSITORY_ROOT / "THIRD_PARTY"
NOTICES_PATH       = REPOSITORY_ROOT / "THIRD_PARTY_NOTICES.md"


def sha256_of(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def install(cmake, build_directory, prefix, failures):
    if prefix.exists():
        shutil.rmtree(prefix)
    completed = subprocess.run(
        [cmake, "--install", str(build_directory), "--prefix", str(prefix)],
        capture_output=True, text=True)
    if completed.returncode != 0:
        failures.append("cmake --install failed:\n{}{}".format(completed.stdout, completed.stderr))
        return False
    return True


def check_installed_licences(document_directory, failures):
    # The manifests are the record of which texts have to travel, so they are
    # what the check reads. A licence added there without an install rule fails
    # here rather than shipping without its text.
    recorded = {}
    for path in sorted(MANIFEST_DIRECTORY.glob("*.toml")):
        manifest = toml_reader.loads(path.read_text(encoding="utf-8"))
        recorded[manifest["license_text"]] = manifest["license_text_sha256"]

    if not recorded:
        failures.append("no manifest named a licence text, so nothing was checked")

    for relative_path, digest in sorted(recorded.items()):
        installed = document_directory / Path(relative_path).name
        if not installed.exists():
            failures.append("{} is named by a manifest but was not installed to {}".format(
                relative_path, installed))
            continue
        if sha256_of(installed) != digest:
            failures.append("{} was installed but does not match its recorded digest".format(
                installed))

    notices = document_directory / NOTICES_PATH.name
    if not notices.exists():
        failures.append("{} was not installed to {}".format(NOTICES_PATH.name, notices))
    elif sha256_of(notices) != sha256_of(NOTICES_PATH):
        failures.append("the installed {} differs from the source".format(NOTICES_PATH.name))


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Install this repository and check the licence texts arrived.")
    parser.add_argument("--cmake", default="cmake", help="the cmake executable to install with")
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--prefix", required=True, type=Path)
    parser.add_argument(
        "--docdir", default="share/doc/vnm_fonts",
        help="install destination of the licences, relative to the prefix")
    return parser.parse_args()


def main():
    arguments = parse_arguments()
    failures  = []

    if install(arguments.cmake, arguments.build_dir, arguments.prefix, failures):
        check_installed_licences(arguments.prefix / arguments.docdir, failures)

    for failure in failures:
        sys.stderr.write("FAIL {}\n".format(failure))
    if failures:
        sys.stderr.write("{} check(s) failed.\n".format(len(failures)))
        return 1

    print("PASS the licence texts and notices install to {}.".format(arguments.docdir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
