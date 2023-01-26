#!/usr/bin/env python3
import argparse
import datetime
import os
import requests
from pathlib import Path
import sys

SESSION_COOKIE_PATH=Path(".session")
USER_AGENT = "Custom script by https://github.com/helehto"


def main() -> None:
    now = datetime.datetime.utcnow()

    parser = argparse.ArgumentParser()
    parser.add_argument("-y", "--year", type=int, required=False, default=now.year)
    parser.add_argument("-d", "--day", type=int, required=False, default=now.day)
    args = parser.parse_args()

    time = now.replace(year=args.year, day=args.day)
    path = Path(f"input-{time.year}-{time.day}.txt")

    # Try a cached copy first to avoid hammering the server on repeated runs.
    url = f"https://adventofcode.com/{time.year}/day/{time.day}/input"
    try:
        sys.stdout.write(path.read_text())
        return
    except FileNotFoundError:
        # File doesn't exist, proceed to download it.
        print(f"Downloading {url}", file=sys.stderr)
        pass

    # Read the session cookie.
    session_cookie = os.environ.get("AOC_SESSION")
    if session_cookie is None:
        session_cookie=SESSION_COOKIE_PATH.read_text()

    # Download the file.
    with requests.Session() as s:
        cookies = {"session": session_cookie}
        headers = {"User-Agent": USER_AGENT}
        response = s.get(url, cookies=cookies, headers=headers)
        response.raise_for_status()

    path.write_text(response.text)
    sys.stdout.write(response.text)


if __name__ == "__main__":
    main()
