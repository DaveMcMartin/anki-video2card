#!/usr/bin/env python3

import csv
import os
import sqlite3
import sys
from pathlib import Path


def create_database(db_path):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS pitch_accents_formatted (
        headword         TEXT    NOT NULL,
        raw_headword     TEXT    NOT NULL,
        katakana_reading TEXT    NOT NULL,
        html_notation    TEXT    NOT NULL,
        pitch_number     TEXT    NOT NULL,
        frequency        INTEGER NOT NULL,
        source           TEXT    NOT NULL
    )
    """)

    cursor.execute("""
    CREATE INDEX IF NOT EXISTS index_pitch_accents_headword
    ON pitch_accents_formatted(headword)
    """)

    cursor.execute("""
    CREATE INDEX IF NOT EXISTS index_pitch_accents_reading
    ON pitch_accents_formatted(katakana_reading)
    """)

    cursor.execute("""
    CREATE INDEX IF NOT EXISTS index_pitch_accents_source
    ON pitch_accents_formatted(source)
    """)

    conn.commit()
    return conn


def import_csv_file(csv_path, conn, source_name):
    cursor = conn.cursor()
    entry_count = 0

    print(f"Importing {csv_path}...")

    with open(csv_path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f, delimiter="\t")

        batch = []
        batch_size = 1000

        for row in reader:
            headword = row.get("headword", "")
            raw_headword = row.get("raw_headword", headword)
            katakana_reading = row.get("katakana_reading", "")
            html_notation = row.get("html_notation", "")
            pitch_number = row.get("pitch_number", "")
            frequency = int(row.get("frequency", 0))

            batch.append(
                (
                    headword,
                    raw_headword,
                    katakana_reading,
                    html_notation,
                    pitch_number,
                    frequency,
                    source_name,
                )
            )

            entry_count += 1

            if len(batch) >= batch_size:
                cursor.executemany(
                    """
                    INSERT INTO pitch_accents_formatted
                    (headword, raw_headword, katakana_reading, html_notation, pitch_number, frequency, source)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                """,
                    batch,
                )
                batch = []
                print(f"  Imported {entry_count} entries...", end="\r")

        if batch:
            cursor.executemany(
                """
                INSERT INTO pitch_accents_formatted
                (headword, raw_headword, katakana_reading, html_notation, pitch_number, frequency, source)
                VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
                batch,
            )

    conn.commit()
    print(f"  Imported {entry_count} entries... Done!")
    return entry_count


def main():
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    csv_files = [
        (project_root / "assets" / "pitch_accents_formatted.1.csv", "bundled"),
        (project_root / "assets" / "pitch_accents_formatted.2.csv", "bundled"),
        (project_root / "assets" / "pitch_accents_formatted.3.csv", "bundled"),
    ]

    db_path = project_root / "assets" / "pitch_accent.db"

    # Check if at least one CSV file exists
    existing_files = [f for f, _ in csv_files if f.exists()]
    if not existing_files:
        print("Error: No pitch accent CSV files found in assets/")
        print("Expected files:")
        for csv_file, _ in csv_files:
            print(f"  - {csv_file}")
        print("\nPlease copy the CSV files from the ajatt-japanese addon:")
        print(
            "  ~/Library/Application Support/Anki2/addons21/1344485230/pitch_accents/res/"
        )
        sys.exit(1)

    print(f"Converting pitch accent CSV files to {db_path}...")

    if db_path.exists():
        print(f"Removing existing database at {db_path}")
        db_path.unlink()

    conn = create_database(str(db_path))

    try:
        total_entries = 0
        for csv_file, source_name in csv_files:
            if csv_file.exists():
                entry_count = import_csv_file(str(csv_file), conn, source_name)
                total_entries += entry_count
            else:
                print(f"Skipping {csv_file} (not found)")

        print(f"\nSuccessfully created database with {total_entries} entries")
        print(f"Database saved to: {db_path}")

    finally:
        conn.close()


if __name__ == "__main__":
    main()
