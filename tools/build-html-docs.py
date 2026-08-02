#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import argparse
import html
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT = "libpkgsource"
DOCUMENTS = (
    ("README.md", "index.html", "libpkgsource"),
    ("HISTORY.md", "project-history.html", "Project history"),
    ("CONTRIBUTING.md", "contributing.html", "Contributing"),
    ("MAINTAINING.md", "maintaining.html", "Maintaining"),
    ("docs/architecture.md", "architecture.html", "Architecture"),
    ("docs/abi.md", "abi.html", "ELF ABI policy"),
    ("docs/code-style.md", "code-style.html", "Code style"),
    ("docs/testing.md", "testing.html", "Testing"),
    ("docs/manpage-markdown.md", "manpage-markdown.html", "Manual-page Markdown"),
    ("docs/html.md", "html-documentation.html", "HTML documentation"),
    ("docs/protocols/source-records-v1.md", "protocols/source-records-v1.html", "Source records version 1"),
    ("docs/history/3.0-authority-reset.md", "history/3.0-authority-reset.html", "3.0 authority reset"),
    ("docs/man/libpkgsource.3.md", "manual/libpkgsource.3.html", "libpkgsource(3)"),
    ("docs/man/pkgsource_model.3.md", "manual/pkgsource_model.3.html", "pkgsource_model(3)"),
    ("docs/man/pkgsource_profile.3.md", "manual/pkgsource_profile.3.html", "pkgsource_profile(3)"),
    ("docs/man/pkgsource_recipe.3.md", "manual/pkgsource_recipe.3.html", "pkgsource_recipe(3)"),
    ("docs/man/pkgsource_snapshot.3.md", "manual/pkgsource_snapshot.3.html", "pkgsource_snapshot(3)"),
    ("docs/man/pkgsource_codec.3.md", "manual/pkgsource_codec.3.html", "pkgsource_codec(3)"),
)


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"build-html-docs: {message}")


def run(command: list[str], *, cwd: Path | None = None, stdin: str | None = None) -> None:
    completed = subprocess.run(command, cwd=cwd, input=stdin, text=True, check=False)
    if completed.returncode != 0:
        fail(f"command failed ({completed.returncode}): {' '.join(command)}")


def pandoc_contract(pandoc: str) -> str:
    completed = subprocess.run([pandoc, "--version"], text=True, capture_output=True)
    first = completed.stdout.splitlines()[0] if completed.returncode == 0 and completed.stdout else ""
    match = re.fullmatch(r"pandoc (\d+)\.(\d+)(?:\..*)?", first)
    if match is None:
        fail(f"cannot parse Pandoc version: {first}")
    major, minor = (int(value) for value in match.groups())
    if major != 3 or minor < 1:
        fail(f"Pandoc 3.1 through 3.x is required; found {first.removeprefix('pandoc ')}")
    help_result = subprocess.run([pandoc, "--help"], text=True, capture_output=True)
    return "--syntax-highlighting=none" if "--syntax-highlighting" in help_result.stdout else "--no-highlight"


def relative_link(page: Path, target: str) -> str:
    return os.path.relpath(target, page.parent.as_posix()).replace(os.sep, "/")


def navigation(page: Path, version: str) -> str:
    links = (
        ("Home", "index.html"),
        ("Architecture", "architecture.html"),
        ("ABI", "abi.html"),
        ("Manual", "manual/libpkgsource.3.html"),
        ("Protocol", "protocols/source-records-v1.html"),
        ("API", "api/index.html"),
        ("History", "project-history.html"),
    )
    items = "\n".join(
        f'<a href="{html.escape(relative_link(page, target))}">{html.escape(label)}</a>'
        for label, target in links
    )
    return (
        '<nav class="house-nav">\n'
        f'<a class="project" href="{html.escape(relative_link(page, "index.html"))}">'
        f"{PROJECT} {html.escape(version)}</a>\n{items}\n</nav>\n"
    )


def render_markdown(pandoc: str, option: str, source_root: Path, output_root: Path, version: str) -> None:
    with tempfile.TemporaryDirectory(prefix="libpkgsource-html-") as temp_name:
        temp = Path(temp_name)
        for source_name, output_name, title in DOCUMENTS:
            source = source_root / source_name
            if not source.is_file():
                fail(f"missing Markdown source: {source_name}")
            output = output_root / output_name
            output.parent.mkdir(parents=True, exist_ok=True)
            page = Path(output_name)
            nav = temp / "nav.html"
            nav.write_text(navigation(page, version), encoding="utf-8", newline="\n")
            footer = temp / "footer.html"
            footer.write_text(
                f'<footer class="house-footer">Generated from {PROJECT} {html.escape(version)} authoritative sources.</footer>\n',
                encoding="utf-8",
                newline="\n",
            )
            run([
                pandoc,
                "--from=markdown-smart",
                "--to=html5",
                "--standalone",
                "--fail-if-warnings",
                "--eol=lf",
                "--wrap=none",
                option,
                f"--metadata=pagetitle:{title}",
                f"--css={relative_link(page, 'assets/house.css')}",
                f"--include-before-body={nav}",
                f"--include-after-body={footer}",
                str(source),
                "--output",
                str(output),
            ])


def render_doxygen(doxygen: str, source_root: Path, output_root: Path, version: str) -> None:
    base = source_root / "Doxyfile"
    configuration = base.read_text(encoding="utf-8") + "\n" + "\n".join([
        f"PROJECT_NUMBER = {version}",
        f"OUTPUT_DIRECTORY = {output_root}",
        f"INPUT = {source_root / 'include/libpkgsource'} {source_root / 'include/libpkgsource-codec'}",
        "FULL_PATH_NAMES = NO",
        f"STRIP_FROM_PATH = {source_root}",
        "GENERATE_HTML = YES",
        "HTML_OUTPUT = api",
        "GENERATE_LATEX = NO",
        f"HTML_EXTRA_STYLESHEET = {source_root / 'docs/assets/doxygen-extra.css'}",
    ]) + "\n"
    run([doxygen, "-"], cwd=source_root, stdin=configuration)
    if not (output_root / "api/index.html").is_file():
        fail("Doxygen did not produce api/index.html")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--project-version", required=True)
    parser.add_argument("--pandoc", required=True)
    parser.add_argument("--doxygen", required=True)
    parser.add_argument("--checker", type=Path, required=True)
    parser.add_argument("--stamp", type=Path, required=True)
    args = parser.parse_args()

    source_root = args.source_root.resolve()
    output_dir = args.output_dir.resolve()
    option = pandoc_contract(args.pandoc)
    output_dir.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{PROJECT}-{args.project_version}-", dir=output_dir.parent))
    try:
        assets = temporary / "assets"
        assets.mkdir(parents=True)
        shutil.copy2(source_root / "docs/assets/house.css", assets / "house.css")
        shutil.copy2(source_root / "docs/assets/doxygen-extra.css", assets / "doxygen-extra.css")
        legal = temporary / "legal"
        legal.mkdir()
        shutil.copy2(source_root / "COPYING", legal / "COPYING")
        shutil.copy2(source_root / "COPYRIGHT", legal / "COPYRIGHT")
        render_markdown(args.pandoc, option, source_root, temporary, args.project_version)
        render_doxygen(args.doxygen, source_root, temporary, args.project_version)
        run([sys.executable, str(args.checker), str(temporary), "--forbid-path", str(source_root), "--forbid-path", str(output_dir.parent)])
        if output_dir.exists():
            shutil.rmtree(output_dir)
        temporary.rename(output_dir)
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.write_text(f"{PROJECT} {args.project_version}\n", encoding="utf-8")
    except BaseException:
        shutil.rmtree(temporary, ignore_errors=True)
        raise
    return 0


if __name__ == "__main__":
    sys.exit(main())
