#!/usr/bin/env python3

# The shipped fonts are byte-verbatim upstream releases, so the gates are the
# ones that apply to any redistributed third-party file:
#
#   1. Every file in fonts/ matches the digest and size its manifest records,
#      and its manifest says the file was not modified.
#   2. Every file is described by exactly one manifest record, and every record
#      describes a file that exists.
#   3. Every licence text carried in LICENSES/ matches its recorded digest.
#   4. THIRD_PARTY_NOTICES.md names every font, every upstream revision or
#      archive, every source URL and every licence file the manifests record.
#
# The family renaming is not checked here: nothing in this repository is
# renamed. The name a font registers under is produced at load time by
# src/vnm_font_namespace.cpp, and tests/vnm_font_namespace_tests.cpp is what
# proves that.
#
#   python tests/test_font_manifest.py

from __future__ import annotations

import hashlib
from pathlib import Path
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


REPOSITORY_ROOT   = Path(__file__).resolve().parents[1]
MANIFEST_DIRECTORY = REPOSITORY_ROOT / "THIRD_PARTY"
FONT_DIRECTORY     = REPOSITORY_ROOT / "fonts"
NOTICES_PATH       = REPOSITORY_ROOT / "THIRD_PARTY_NOTICES.md"


def sha256_of(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_manifests():
    manifests = {}
    for path in sorted(MANIFEST_DIRECTORY.glob("*.toml")):
        manifests[path] = toml_reader.loads(path.read_text(encoding="utf-8"))
    return manifests


def check_artifacts(manifests, failures):
    claimed = {}
    for path, manifest in manifests.items():
        for artifact in manifest["artifact"]:
            font_path = REPOSITORY_ROOT / artifact["path"]
            key       = artifact["key"]

            claimed.setdefault(artifact["path"], []).append("{}:{}".format(path.name, key))

            if not font_path.exists():
                failures.append("{}: {} does not exist".format(key, artifact["path"]))
                continue
            if sha256_of(font_path) != artifact["sha256"]:
                failures.append("{}: {} does not match its recorded digest".format(
                    key, artifact["path"]))
            if font_path.stat().st_size != artifact["size"]:
                failures.append("{}: {} does not match its recorded size".format(
                    key, artifact["path"]))
            if artifact["modifications"] != "none":
                failures.append(
                    "{}: modifications is {!r}; the shipped files are byte-verbatim and "
                    "nothing here may change them".format(key, artifact["modifications"]))

    for relative_path, keys in sorted(claimed.items()):
        if len(keys) > 1:
            failures.append("{} is claimed by {}".format(relative_path, ", ".join(keys)))

    present = {"fonts/" + path.name for path in FONT_DIRECTORY.iterdir() if path.is_file()}
    for relative_path in sorted(present - set(claimed)):
        failures.append("{} is present but described by no manifest record".format(relative_path))


def check_licences(manifests, failures):
    for path, manifest in manifests.items():
        licence_path = REPOSITORY_ROOT / manifest["license_text"]
        if not licence_path.exists():
            failures.append("{}: {} does not exist".format(path.name, manifest["license_text"]))
            continue
        if sha256_of(licence_path) != manifest["license_text_sha256"]:
            failures.append("{}: {} does not match its recorded digest".format(
                path.name, manifest["license_text"]))

    carried  = {"LICENSES/" + path.name for path in (REPOSITORY_ROOT / "LICENSES").iterdir()}
    recorded = {manifest["license_text"] for manifest in manifests.values()}
    for relative_path in sorted(carried - recorded):
        failures.append("{} is carried but no manifest names it".format(relative_path))


def check_notices(manifests, failures):
    notices = NOTICES_PATH.read_text(encoding="utf-8")
    for path, manifest in manifests.items():
        required = [manifest["name"], manifest["license_text"]]
        for field in ("upstream_repository", "upstream_revision",
                      "distribution_repository", "distribution_revision",
                      "distribution_archive_url"):
            if field in manifest:
                required.append(manifest[field])
        for artifact in manifest["artifact"]:
            required.append(Path(artifact["path"]).name)
            if "source_url" in artifact:
                required.append(artifact["source_url"])

        for value in required:
            if value not in notices:
                failures.append("THIRD_PARTY_NOTICES.md does not mention {!r}, recorded in {}"
                                .format(value, path.name))


def main():
    failures  = []
    manifests = load_manifests()
    if not manifests:
        failures.append("no manifest found under THIRD_PARTY/")
    else:
        check_artifacts(manifests, failures)
        check_licences(manifests, failures)
        check_notices(manifests, failures)

    for failure in failures:
        sys.stderr.write("FAIL {}\n".format(failure))
    if failures:
        sys.stderr.write("{} check(s) failed.\n".format(len(failures)))
        return 1

    artifacts = sum(len(manifest["artifact"]) for manifest in manifests.values())
    print("PASS {} byte-verbatim fonts match their manifests.".format(artifacts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
