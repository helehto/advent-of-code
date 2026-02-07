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
import json
import os
import shutil
import sqlite3
import subprocess as sp
import sys
import tempfile
import typing as T
from dataclasses import dataclass

import polars as pl
import numpy as np
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
def jj_workspace(rev: str, path: Path) -> T.Iterator[Path]:
    try:
        sp.check_call(("jj", "--quiet", "workspace", "add", "--revision", rev, path))
        yield Path(path)
    finally:
        sp.check_call(("jj", "workspace", "forget", path.name))
        assert TMPDIR in path.parents  # avoid accidents
        shutil.rmtree(path, ignore_errors=True)


@contextmanager
def binary_for_commit(commit: str) -> T.Iterator[AocBinary]:
    # Note: we deliberately check out the workspace in the same path for all
    # invocations of binary_for_commit() to maximize ccache compilation cache
    # hits.
    workspace = TMPDIR / "tmpdiff-jj-workspace"

    with tempfile.TemporaryDirectory(prefix="aocdiff-") as d:
        exe = Path(d) / f"aoc-{commit}"

        with jj_workspace(commit, workspace):
            meson_cmd = "meson setup --wipe . build -Dunity=on -Dunity_size=8".split()
            sp.run(meson_cmd, cwd=workspace, stdout=sp.DEVNULL, check=True)
            sp.run("ninja", cwd=workspace / "build", check=True)
            exe.write_bytes((workspace / "build" / "aoc").read_bytes())

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


def diff_solutions(solutions_by_commit: dict[str, dict[tuple[int, int], str]]) -> bool:
    first_commit_hash, first_solutions = next(iter(solutions_by_commit.items()))
    for commit_hash, solutions in solutions_by_commit.items():
        if set(solutions.keys()) != set(first_solutions.keys()):
            print(
                f"Different set of solutions between commits {first_commit_hash} and {commit_hash}!?"
            )
            return False

        for (y, d), sol in first_solutions.items():
            other_sol = solutions[(y, d)]
            if other_sol != sol:
                a = sol.splitlines()
                b = other_sol.splitlines()
                diff = tuple(
                    difflib.unified_diff(
                        a,
                        b,
                        fromfile=f"{y}/{d}: commit {first_commit_hash}",
                        tofile=f"{y}/{d}: commit {commit_hash}",
                        lineterm="",
                    )
                )
                if diff:
                    print("\n".join(color_diff(diff)))
                    return False

    return True


def two_binaries(
    db: sqlite3.Connection,
    bargs: BenchmarkArgs,
    binaries: T.Collection[AocBinary],
) -> int:
    json_outputs = {}

    for binary in binaries:
        cmd = (str(binary.path.absolute()), *bargs.to_cmdline())
        data = sp.check_output(cmd, encoding="utf-8").strip().splitlines()[-1]
        json_outputs[binary.commit] = json.loads(data)

    # Make sure all solutions are the same.
    solutions_by_commit = {
        commit_hash: {(y, d): sol for y, d, _, sol in d}
        for commit_hash, d in json_outputs.items()
    }
    if not diff_solutions(solutions_by_commit):
        sys.exit(1)

    with db:
        cur = db.execute("INSERT INTO runs DEFAULT VALUES")
        run_id = cur.lastrowid
        assert run_id is not None

        for commit_hash, json_output in json_outputs.items():
            cur = db.execute(
                "INSERT INTO commit_runs (run_id, commit_hash) VALUES (?, ?)",
                (run_id, commit_hash),
            )
            commit_run_id = cur.lastrowid

            for year, day, durations, output in json_output:
                durations = np.array(durations, dtype=np.int64)

                db.execute(
                    """
                INSERT INTO commit_run_stats (
                    commit_run_id,
                    year,
                    day,
                    iterations,
                    min_ns,
                    p10_ns,
                    p25_ns,
                    p50_ns,
                    p75_ns,
                    p90_ns,
                    max_ns,
                    mean_ns
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                    (
                        commit_run_id,
                        year,
                        day,
                        len(durations),
                        int(np.min(durations)),
                        int(np.percentile(durations, 10)),
                        int(np.percentile(durations, 25)),
                        int(np.percentile(durations, 50)),
                        int(np.percentile(durations, 75)),
                        int(np.percentile(durations, 90)),
                        int(np.max(durations)),
                        int(np.mean(durations)),
                    ),
                )

    return run_id


def print_timing_diff(db: sqlite3.Connection, run_id: int) -> None:
    commits = db.execute(
        "SELECT commit_hash FROM commit_runs WHERE run_id = ?", (run_id,)
    ).fetchall()
    commits = [c for (c,) in commits]
    assert len(commits) == 2

    df = pl.read_database(
        """
    SELECT
        year,
        day,
        min_ns,
        p50_ns,
        mean_ns,
        commit_hash
    FROM commit_run_stats
    JOIN commit_runs ON id = commit_run_id
    WHERE run_id = :run_id
    """,
        db,
        execute_options={"parameters": {"run_id": run_id}},
    )

    dfs = df.partition_by("commit_hash", include_key=False, as_dict=True)
    df0 = dfs[(commits[0],)]
    df1 = dfs[(commits[1],)]
    df0 = df0.sort(["year", "day"])
    df1 = df1.sort(["year", "day"])

    delta = df0.select(pl.col(["year", "day"])).with_columns(
        [
            df0["min_ns"].alias("min0_ns"),
            df1["min_ns"].alias("min1_ns"),
            (df1["min_ns"] - df0["min_ns"]).alias("delta_min_ns"),
            ((df1["min_ns"] - df0["min_ns"]) / df0["min_ns"]).alias("delta_min_rel"),
            df0["mean_ns"].alias("mean0_ns"),
            df1["mean_ns"].alias("mean1_ns"),
            (df1["mean_ns"] - df0["mean_ns"]).alias("delta_mean_ns"),
            ((df1["mean_ns"] - df0["mean_ns"]) / df0["mean_ns"]).alias(
                "delta_mean_rel"
            ),
        ]
    )

    headers = (
        "",
        "Year",
        "Day",
        "min t₀",
        "min t₁",
        "Δmin",
        "Δmin%",
        "avg t₀",
        "avg t₁",
        "Δavg",
        "Δavg%",
    )

    rows = []
    for (
        year,
        day,
        min0_ns,
        min1_ns,
        delta_min_ns,
        delta_min_rel,
        mean0_ns,
        mean1_ns,
        delta_mean_ns,
        delta_mean_rel,
    ) in delta.iter_rows():
        min_significant = abs(delta_min_rel) >= 0.1
        mean_significant = abs(delta_mean_rel) >= 0.1

        rows.append(
            (
                "",
                year,
                day,
                min0_ns / 1e3,
                min1_ns / 1e3,
                colorize(delta_min_ns / 1e3, ".2f", min_significant),
                colorize(delta_min_rel, "+.1%", min_significant),
                mean0_ns / 1e3,
                mean1_ns / 1e3,
                colorize(delta_mean_ns / 1e6, ".2f", mean_significant),
                colorize(delta_mean_rel, "+.1%", mean_significant),
            )
        )

    if len(rows) > 1:
        rows.append(SEPARATING_LINE)
        delta_sum = delta["min1_ns"].sum() - delta["min0_ns"].sum()
        rows.append(
            (
                "Σ",
                "",
                "",
                delta["min0_ns"].sum() / 1e3,
                delta["min1_ns"].sum() / 1e3,
                colorize(
                    delta_sum / 1e3,
                    ".2f",
                    abs(delta_sum / delta["min0_ns"].sum()) >= 0.1,
                ),
                colorize(
                    delta_sum / delta["min0_ns"].sum(),
                    "+.1%",
                    abs(delta_sum / delta["min0_ns"].sum()) >= 0.1,
                ),
                delta["mean0_ns"].sum() / 1e3,
                delta["mean1_ns"].sum() / 1e3,
                colorize(
                    (delta["mean1_ns"].sum() - delta["mean0_ns"].sum()) / 1e6,
                    ".2f",
                    abs(
                        (delta["mean1_ns"].sum() - delta["mean0_ns"].sum())
                        / delta["mean0_ns"].sum()
                    )
                    >= 0.1,
                ),
                colorize(
                    (delta["mean1_ns"].sum() - delta["mean0_ns"].sum())
                    / delta["mean0_ns"].sum(),
                    "+.1%",
                    abs(
                        (delta["mean1_ns"].sum() - delta["mean0_ns"].sum())
                        / delta["mean0_ns"].sum()
                    )
                    >= 0.1,
                ),
            )
        )

    print(tabulate(rows, headers=headers))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-b", "--base-change", default="@-", metavar="CHANGE-ID")
    parser.add_argument("-i", "--iterations", default=1, type=int)
    parser.add_argument("-j", "--jobs", default=None, type=int)
    parser.add_argument("-n", "--max-runs", default=5, type=int)
    parser.add_argument("-t", "--target-time", default=None, type=float)
    parser.add_argument("-s", "--stable-mode", action="store_true")
    parser.add_argument("--diff", action="store_true")
    parser.add_argument("problems", nargs="*")
    args = parser.parse_args()

    base_commit_hash = sp.check_output(
        (
            "jj",
            "log",
            "--no-graph",
            "-r",
            args.base_change,
            "-T",
            "self.commit_id() ++ '\n'",
        ),
        encoding="utf-8",
    ).strip()
    head_commit_hash = sp.check_output(
        ("jj", "log", "--no-graph", "-r", "@", "-T", "self.commit_id() ++ '\n'"),
        encoding="utf-8",
    ).strip()

    conn = sqlite3.connect("../scratch/aoc.db", autocommit=False)

    with conn:
        conn.executescript(
            """
        CREATE TABLE IF NOT EXISTS runs (
            id INTEGER PRIMARY KEY,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS commit_runs (
            id INTEGER PRIMARY KEY,
            run_id INTEGER NOT NULL,
            commit_hash TEXT NOT NULL,
            FOREIGN KEY(run_id) REFERENCES runs(id)
        );
        CREATE TABLE IF NOT EXISTS commit_run_stats (
            commit_run_id INTEGER NOT NULL,
            year INTEGER NOT NULL,
            day INTEGER NOT NULL,
            iterations INTEGER NOT NULL,
            min_ns INTEGER NOT NULL,
            p10_ns INTEGER NOT NULL,
            p25_ns INTEGER NOT NULL,
            p50_ns INTEGER NOT NULL,
            p75_ns INTEGER NOT NULL,
            p90_ns INTEGER NOT NULL,
            max_ns INTEGER NOT NULL,
            mean_ns INTEGER NOT NULL,
            FOREIGN KEY(commit_run_id) REFERENCES commit_runs(id)
        );
        """
        )

    bargs = BenchmarkArgs(
        iterations=args.iterations,
        jobs=args.jobs,
        stable_mode=args.stable_mode,
        target_time=args.target_time,
        problems=tuple(args.problems),
    )

    with (
        binary_for_commit(base_commit_hash) as old_binary,
        binary_for_commit(head_commit_hash) as new_binary,
    ):
        run_id = two_binaries(conn, bargs, [old_binary, new_binary])
        print_timing_diff(conn, run_id)


if __name__ == "__main__":
    main()
