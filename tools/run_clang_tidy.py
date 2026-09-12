#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List, Optional, Sequence

SOURCE_EXTENSIONS = {'.c', '.cc', '.cpp', '.cxx'}
HEADER_EXTENSIONS = {'.h', '.hh', '.hpp', '.hxx'}
GENERATED_PREFIXES = ('moc_', 'ui_', 'qrc_')
GTEST_NAMES = {'gtest_main.cc', 'gtest-all.cc', 'gmock_main.cc', 'gmock-all.cc'}


def is_generated_source(path: Path) -> bool:
    name = path.name
    return name.startswith(GENERATED_PREFIXES) or name.endswith('.moc')


def header_filter() -> str:
    """Match package headers and exclude generated Qt moc_/ui_/qrc_ files.

    Headers live under include/rqt_multiplot/ (package name), not the source
    checkout directory name. Generated ui_*.h files are typically included from
    the build directory as ui_Foo.h and must not match this filter.
    """
    return r'(?:^|/)(?:include|src)/rqt_multiplot/(?!moc_|ui_|qrc_)'


def is_test_source(path: Path, source_root: Path) -> bool:
    if path.name in GTEST_NAMES:
        return True
    posix = path.as_posix()
    if '/gtest/' in posix or '/gmock/' in posix:
        return True
    try:
        relative = path.resolve().relative_to(source_root.resolve())
    except ValueError:
        return '/test/' in posix
    return relative.parts[0] == 'test'


def filter_compile_commands(entries: Sequence[dict], source_root: Path) -> List[dict]:
    filtered = []
    for entry in entries:
        path = Path(entry['file'])
        if not path.is_file():
            continue
        if is_generated_source(path):
            continue
        if is_test_source(path, source_root):
            continue
        filtered.append(entry)
    return filtered


def select_translation_units(
    changed: Sequence[str],
    translation_units: Sequence[str],
    source_root: Path,
) -> List[str]:
    if any(Path(path).suffix in HEADER_EXTENSIONS for path in changed):
        return list(translation_units)

    wanted = []
    for path in changed:
        candidate = Path(path)
        if candidate.suffix not in SOURCE_EXTENSIONS:
            continue
        if not candidate.is_absolute():
            candidate = source_root / candidate
        wanted.append(candidate.as_posix())

    selected = []
    for unit in translation_units:
        if Path(unit).as_posix() in wanted:
            selected.append(unit)
    return selected


def is_full_scan_ref(ref: Optional[str]) -> bool:
    if ref is None or ref == '':
        return True
    return set(ref) == {'0'}


def job_count(cpu_count: Optional[int], override: Optional[int]) -> int:
    if override is not None and override > 0:
        return override
    return max(1, cpu_count or 1)


def write_compile_commands(directory: Path, entries: Sequence[dict]) -> Path:
    path = directory / 'compile_commands.json'
    path.write_text(json.dumps(list(entries), indent=2))
    return path


def find_run_clang_tidy() -> Optional[str]:
    names = ['run-clang-tidy']
    names.extend(f'run-clang-tidy-{version}' for version in range(21, 13, -1))
    for name in names:
        found = shutil.which(name)
        if found:
            return found
    return None


def infer_merge_base(source_root: Path) -> Optional[str]:
    for ref in ('origin/main', 'main'):
        if subprocess.run(
            ['git', '-C', str(source_root), 'rev-parse', '--verify', ref],
            check=False,
            capture_output=True,
        ).returncode != 0:
            continue
        merge = subprocess.run(
            ['git', '-C', str(source_root), 'merge-base', 'HEAD', ref],
            check=False,
            capture_output=True,
            text=True,
        )
        if merge.returncode == 0 and merge.stdout.strip():
            return merge.stdout.strip()
    return None


def list_changed_files(source_root: Path, ref: str) -> List[str]:
    result = subprocess.run(
        ['git', '-C', str(source_root), 'diff', '--name-only', '--diff-filter=ACMR', ref],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        message = result.stderr.strip() or 'git diff failed'
        raise RuntimeError(message)
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def build_command(
    run_clang_tidy: str,
    build_dir: Path,
    jobs: int,
    source_root: Path,
    files: Sequence[str],
    config_file: Optional[Path],
) -> List[str]:
    cmd = [
        run_clang_tidy,
        '-p',
        str(build_dir),
        '-j',
        str(jobs),
        '-header-filter',
        header_filter(),
        '-quiet',
    ]
    if config_file is not None:
        cmd.extend(['-config-file', str(config_file)])
    cmd.extend(re.escape(path) for path in files)
    return cmd


def resolve_jobs(override: Optional[int]) -> int:
    env_jobs = os.environ.get('CLANG_TIDY_JOBS')
    if override is None and env_jobs:
        try:
            override = int(env_jobs)
        except ValueError:
            override = None
    return job_count(os.cpu_count(), override)


def load_compile_commands(path: Path) -> List[dict]:
    return json.loads(path.read_text())


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description='Run clang-tidy in parallel on package sources.')
    parser.add_argument('--compile-commands', required=True, type=Path)
    parser.add_argument('--source-root', required=True, type=Path)
    parser.add_argument('--config', type=Path, default=None)
    parser.add_argument('--changed-since', default=os.environ.get('CLANG_TIDY_CHANGED_SINCE'))
    parser.add_argument('--all', action='store_true', default=os.environ.get('CLANG_TIDY_ALL') == '1')
    parser.add_argument('--jobs', type=int, default=None)
    args = parser.parse_args(argv)

    compile_commands = Path(args.compile_commands)
    source_root = Path(args.source_root).resolve()
    if not compile_commands.is_file():
        print(f'compile_commands.json not found: {compile_commands}', file=sys.stderr)
        return 1

    run_clang_tidy = find_run_clang_tidy()
    if run_clang_tidy is None:
        print('run-clang-tidy not found in PATH', file=sys.stderr)
        return 1

    entries = filter_compile_commands(load_compile_commands(compile_commands), source_root)
    translation_units = [entry['file'] for entry in entries]
    if not translation_units:
        print('No translation units left after filtering compile_commands.json')
        return 0

    changed_since = args.changed_since
    if args.all or (changed_since is not None and is_full_scan_ref(changed_since)):
        selected = translation_units
        print(f'clang-tidy: full scan ({len(selected)} files)')
    else:
        if not changed_since:
            changed_since = infer_merge_base(source_root)
        if not changed_since:
            selected = translation_units
            print(f'clang-tidy: full scan ({len(selected)} files)')
        else:
            try:
                changed = list_changed_files(source_root, changed_since)
            except RuntimeError as exc:
                print(f'clang-tidy: {exc}', file=sys.stderr)
                return 1
            selected = select_translation_units(changed, translation_units, source_root)
            if not selected:
                print('clang-tidy: no C++ sources changed')
                return 0
            print(f'clang-tidy: {len(selected)} of {len(translation_units)} files since {changed_since}')

    jobs = resolve_jobs(args.jobs)
    config_file = Path(args.config).resolve() if args.config else None
    with tempfile.TemporaryDirectory(prefix='rqt_multiplot_clang_tidy_') as tmp:
        write_compile_commands(Path(tmp), entries)
        cmd = build_command(
            run_clang_tidy,
            Path(tmp),
            jobs,
            source_root,
            selected,
            config_file,
        )
        print(' '.join(cmd))
        return subprocess.run(cmd, check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
