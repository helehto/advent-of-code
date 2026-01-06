#!/usr/bin/env python3
# pylint: disable=invalid-name
# pylint: disable=line-too-long
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=missing-module-docstring

from __future__ import annotations
from pathlib import Path
import argparse
from contextlib import contextmanager
import difflib
import itertools
import json
import os
import subprocess as sp
import tempfile
import typing as T
import random
import sys
from dataclasses import dataclass

import numpy as np
import numpy.typing as npt
from tabulate import tabulate, SEPARATING_LINE

T1 = T.TypeVar("T1")
T2 = T.TypeVar("T2")
TMPDIR = Path(os.environ.get("TMPDIR", "/tmp"))


@dataclass
class AocBinary:
    path: Path
    commit: str
    description: T.Optional[str]


@contextmanager
def git_worktree(commit: str, patch: T.Optional[bytes], path: Path) -> T.Iterator[Path]:
    try:
        sp.run(
            ("git", "worktree", "add", "--force", "--quiet", path, commit), check=True
        )

        if patch is not None:
            sp.run(("git", "apply", "--allow-empty"), cwd=path, input=patch, check=True)

        yield Path(path)
    finally:
        sp.run(("git", "worktree", "remove", "--force", path), check=False)


@contextmanager
def binary_for_commit(
    commit: str, patch: T.Optional[bytes] = None
) -> T.Iterator[AocBinary]:
    # Note: we deliberately check out worktree in the same path for all
    # invocations of binary_for_commit() to maximize ccache compilation cache
    # hits.
    worktree = TMPDIR / "tmpdiff-worktree"

    with tempfile.TemporaryDirectory(prefix="aocdiff-") as d:
        exe = Path(d) / f"aoc-{commit}"

        with git_worktree(commit, patch, worktree):
            meson_cmd = "meson setup --wipe . build -Dunity=on -Dunity_size=8".split()
            sp.run(meson_cmd, cwd=worktree, stdout=sp.DEVNULL, check=True)
            sp.run("ninja", cwd=worktree / "build", check=True)
            exe.write_bytes((worktree / "build" / "aoc").read_bytes())

        exe.chmod(0o755)
        yield AocBinary(exe, commit, None)


def wrap_ansi(s: str, code: str) -> str:
    return f"\x1b[{code}m{s}\x1b[m"


def wrap_color_rgb(s: str, r: int, g: int, b: int) -> str:
    return wrap_ansi(s, f"38;2;{r};{g};{b}")


def colorize(value: T.Any, fmt: str, is_significant: T.Any = True) -> str:
    text = f"{value:{fmt}}"

    if is_significant:
        color = (255, 30, 30) if value >= 0 else (30, 180, 30)
        text = wrap_color_rgb(f"{value:{fmt}}", *color)

    return text


@dataclass
class Measurements:
    ts: list[npt.NDArray[T.Any]]

    @staticmethod
    def _massage(t: npt.ArrayLike) -> npt.NDArray[T.Any]:
        t = np.sort(np.array(t))
        n_outliers = max(len(t) // 10, 0)

        # Change units: ns -> μs
        t *= 1e-3

        # Throw away outliers.
        if n_outliers > 0:
            t = t[:-n_outliers]

        return t

    def __init__(self, ts: T.Iterable[npt.ArrayLike]) -> None:
        self.ts = [self._massage(t) for t in ts]

    @staticmethod
    def merge(
        ms1: dict[tuple[int, int], Measurements],
        ms2: dict[tuple[int, int], Measurements],
    ) -> None:
        for (y, d), m2 in ms2.items():
            m1 = ms1.get((y, d))
            assert m1 is not None
            for i, _ in enumerate(m1.ts):
                m1.ts[i] = np.sort(np.hstack((m1.ts[i], m2.ts[i])))
                assert len(m1.ts[i].shape) == 1

    def min_diff(self) -> tuple[float, float]:
        assert len(self.ts) == 2
        d = np.min(self.ts[1]) - np.min(self.ts[0])
        return (d, d / np.min(self.ts[0]))

    def mins(self) -> tuple[float, ...]:
        return tuple(map(np.min, self.ts))

    def means(self) -> tuple[float, ...]:
        return tuple(map(np.mean, self.ts))

    def quantiles(self, k: float) -> tuple[float, ...]:
        return tuple(sorted(t)[int(k * len(t))] for t in self.ts)

    def significant_change(self) -> bool:
        _, min_rel_diff = self.min_diff()
        mean_abs_diff = np.mean(self.ts[1]) - np.mean(self.ts[0])
        mean_rel_diff = mean_abs_diff / np.mean(self.ts[0])
        return abs(min_rel_diff) >= 0.1 or abs(mean_rel_diff) >= 0.1

    def format_row(self, year: int, day: int) -> tuple[T.Any, ...]:
        min_diff, min_rel_diff = self.min_diff()

        min_significant = abs(min_rel_diff) >= 0.1
        mean_abs_diff = np.mean(self.ts[1]) - np.mean(self.ts[0])
        mean_rel_diff = mean_abs_diff / np.mean(self.ts[0])

        return (
            year,
            day,
            len(self.ts[0]),
            len(self.ts[1]),
            *self.mins(),
            colorize(min_diff, ".2f", min_significant),
            colorize(min_rel_diff, "+.1%", min_significant),
            *self.means(),
            colorize(mean_abs_diff, ".2f", abs(mean_rel_diff) >= 0.1),
            colorize(mean_rel_diff, "+.1%", abs(mean_rel_diff) >= 0.1),
        )

    @staticmethod
    def header_row() -> tuple[str, ...]:
        return (
            "Year",
            "Day",
            "n0",
            "n1",
            "min t₀",
            "min t₁",
            "Δmin",
            "Δmin%",
            "avg t₀",
            "avg t₁",
            "Δavg",
            "Δavg%",
        )

    @staticmethod
    def format_summary_row(
        all_measurements: T.Collection[Measurements],
    ) -> tuple[T.Any, ...]:
        def geomean(x: npt.ArrayLike) -> T.Any:
            return np.exp(np.mean(np.log(np.array(x))))

        old_mins, new_mins = np.array(tuple(map(Measurements.mins, all_measurements))).T
        old_means, new_means = np.array(
            tuple(map(Measurements.means, all_measurements))
        ).T
        delta_min_pct_gmean = geomean(1 + (new_mins - old_mins) / old_mins) - 1
        delta_mean_pct_gmean = geomean(1 + (new_means - old_means) / old_means) - 1

        return (
            "Σ",
            "",
            sum(len(m.ts[0]) for m in all_measurements),
            sum(len(m.ts[1]) for m in all_measurements),
            sum(old_mins),
            sum(new_mins),
            sum(new_mins) - sum(old_mins),
            f"{delta_min_pct_gmean:+.1%}",
            sum(old_means),
            sum(new_means),
            sum(new_means) - sum(old_means),
            f"{delta_mean_pct_gmean:+.1%}",
        )


@dataclass
class BenchmarkArgs:
    iterations: T.Optional[int]
    jobs: T.Optional[int]
    stable_mode: bool
    target_time: T.Optional[float]
    problems: T.Collection[str]

    def to_cmdline(self) -> list[str]:
        v = ["--json"]
        if self.jobs:
            v += ["-j", str(self.jobs)]
        if self.iterations:
            v += ["-i", str(self.iterations)]
        if self.stable_mode:
            v += ["-s"]
        if self.target_time:
            v += ["-t", str(self.target_time)]

        v += [*self.problems]
        return v


def group_by(iterable: T.Iterable[T1], key: T.Callable[[T1], T2]) -> dict[T2, list[T1]]:
    groups: dict[T2, list[T1]] = {}
    for x in iterable:
        groups.setdefault(key(x), []).append(x)
    return groups


def run_binaries(
    args: BenchmarkArgs, binaries: T.Collection[AocBinary]
) -> tuple[dict[tuple[int, int], Measurements], dict[tuple[int, int], list[str]]]:
    def _run(binary: AocBinary) -> str:
        cmd = (str(binary.path.absolute()), *args.to_cmdline())
        return sp.check_output(cmd, encoding="utf-8").strip().splitlines()[-1]

    raw_output = map(_run, binaries)
    json_output = tuple(map(json.loads, raw_output))

    # Make sure all binaries output timing information for the same problems.
    problems = [[(y, d) for y, d, _, _ in binary] for binary in json_output]
    assert all(p == problems[0] for p in problems[1:])

    flattened = itertools.chain.from_iterable(json_output)
    grouped = group_by(flattened, lambda x: tuple(x[:2]))
    outputs = {(y, d): [x[3] for x in data] for (y, d), data in grouped.items()}

    measurements = {}
    for (y, d), timing_info in grouped.items():
        ts = [np.array(t, dtype=np.float64) for _, _, t, _ in timing_info]
        measurements[(y, d)] = Measurements(ts)

    return measurements, outputs


def benchmark(
    args: BenchmarkArgs, max_runs: int, binaries: T.Collection[AocBinary]
) -> dict[tuple[int, int], Measurements]:
    result, _ = run_binaries(args, binaries)

    for _ in range(1, max_runs):
        rerun_problems = [
            f"{y}/{d}" for (y, d), m in result.items() if m.significant_change()
        ]
        if not rerun_problems:
            break
        random.shuffle(rerun_problems)
        print("Rerunning problems:", rerun_problems)

        new_args = BenchmarkArgs(
            iterations=args.iterations,
            jobs=args.jobs,
            stable_mode=args.stable_mode,
            target_time=(1 / len(rerun_problems)),
            problems=rerun_problems,
        )
        new_result, _ = run_binaries(new_args, binaries)
        Measurements.merge(result, new_result)

    return result


def color_diff(diff: T.Sequence[str]) -> list[str]:
    def color_line(s: str) -> str:
        if s.startswith("---") or s.startswith("+++"):
            return wrap_ansi(s, "1")
        if s.startswith("@@"):
            return wrap_ansi(s, "36")
        if s.startswith("-"):
            return wrap_ansi(s, "31")
        if s.startswith("+"):
            return wrap_ansi(s, "32")

        return s

    return list(map(color_line, diff))


def diff_solutions(
    old_commit: str, problems: T.Collection[str], binaries: T.Collection[AocBinary]
) -> None:
    assert len(binaries) == 2

    problems = problems or ["*"]
    bargs = BenchmarkArgs(
        iterations=1, jobs=None, stable_mode=False, target_time=None, problems=problems
    )
    _, outputs = run_binaries(bargs, binaries)

    ok = True
    for (y, d), o in outputs.items():
        if o[0] != o[1]:
            a = o[0].splitlines()
            b = o[1].splitlines()
            diff = tuple(
                difflib.unified_diff(
                    a,
                    b,
                    fromfile=f"{y}/{d}: commit {old_commit}",
                    tofile=f"{y}/{d}: new (with changes in index)",
                    lineterm="",
                )
            )
            if diff:
                ok = False
                print("\n".join(color_diff(diff)))

    if not ok:
        sys.exit(1)


def one_binary(bargs: BenchmarkArgs, args: T.Any, binary: AocBinary) -> None:
    if args.diff:
        print(f"{sys.argv[0]}: no changes in index, nothing to compare to")

    if args.problems:
        measurements, _ = run_binaries(bargs, [binary])

        rows: list[T.Any] = []

        for (y, d), m in measurements.items():
            rows.append(
                [
                    y,
                    d,
                    len(m.ts[0]),
                    m.means()[0],
                    np.std(m.ts[0]),
                    m.quantiles(0.1)[0],
                    m.quantiles(0.9)[0],
                ]
            )
        if len(measurements) > 1:
            rows.append(SEPARATING_LINE)
            rows.append(
                (
                    "",
                    "",
                    sum(len(x.ts[0]) for x in measurements.values()),
                    np.mean([x.means()[0] for x in measurements.values()]),
                    0.0,
                    np.mean([x.quantiles(0.1)[0] for x in measurements.values()]),
                    np.mean([x.quantiles(0.9)[0] for x in measurements.values()]),
                )
            )

        print(
            tabulate(
                rows,
                headers=(
                    "Year",
                    "Day",
                    "Iterations",
                    "Mean (μs)",
                    "σ (μs)",
                    "10% (μs)",
                    "90% (μs)",
                ),
            )
        )


def two_binaries(
    bargs: BenchmarkArgs,
    base_commit_hash: str,
    args: T.Any,
    binaries: T.Collection[AocBinary],
) -> None:
    assert len(binaries) == 2
    diff_solutions(base_commit_hash, args.problems, binaries)

    if args.problems:
        measurements = benchmark(bargs, args.max_runs, binaries)

        rows: list[T.Any] = []
        for (y, d), m in measurements.items():
            rows.append(m.format_row(y, d))
        if len(measurements) > 1:
            rows.append(SEPARATING_LINE)
            rows.append(Measurements.format_summary_row(measurements.values()))
        print(tabulate(rows, headers=Measurements.header_row()))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-b", "--base-commit", default="HEAD", metavar="COMMIT")
    parser.add_argument("-i", "--iterations", default=1, type=int)
    parser.add_argument("-j", "--jobs", default=None, type=int)
    parser.add_argument("-n", "--max-runs", default=5, type=int)
    parser.add_argument("-t", "--target-time", default=None, type=float)
    parser.add_argument("-s", "--stable-mode", action="store_true")
    parser.add_argument("--diff", action="store_true")
    parser.add_argument("problems", nargs="*")
    args = parser.parse_args()

    base_commit_hash = sp.check_output(
        ("git", "rev-parse", args.base_commit), encoding="utf-8"
    ).strip()
    head_commit_hash = sp.check_output(
        ("git", "rev-parse", "HEAD"), encoding="utf-8"
    ).strip()
    index_patch = sp.check_output(("git", "diff"))

    bargs = BenchmarkArgs(
        iterations=args.iterations,
        jobs=args.jobs,
        stable_mode=args.stable_mode,
        target_time=args.target_time,
        problems=tuple(args.problems),
    )

    with binary_for_commit(base_commit_hash) as old_binary:
        have_changes = base_commit_hash != head_commit_hash or index_patch.strip()

        if have_changes and args.diff:
            with binary_for_commit(head_commit_hash, index_patch) as new_binary:
                two_binaries(bargs, base_commit_hash, args, [old_binary, new_binary])
        else:
            one_binary(bargs, args, old_binary)


if __name__ == "__main__":
    main()
