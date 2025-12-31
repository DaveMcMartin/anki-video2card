#!/usr/bin/env python3

import os
import sqlite3
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def create_database(db_path):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS entries (
        id INTEGER PRIMARY KEY,
        entry_seq INTEGER UNIQUE NOT NULL
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS kanji_elements (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        entry_id INTEGER NOT NULL,
        keb TEXT NOT NULL,
        FOREIGN KEY (entry_id) REFERENCES entries(id)
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS reading_elements (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        entry_id INTEGER NOT NULL,
        reb TEXT NOT NULL,
        FOREIGN KEY (entry_id) REFERENCES entries(id)
    )
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS senses (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        entry_id INTEGER NOT NULL,
        pos TEXT,
        gloss TEXT,
        FOREIGN KEY (entry_id) REFERENCES entries(id)
    )
    """)

    cursor.execute("CREATE INDEX IF NOT EXISTS idx_kanji_keb ON kanji_elements(keb)")
    cursor.execute(
        "CREATE INDEX IF NOT EXISTS idx_reading_reb ON reading_elements(reb)"
    )
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_entry_seq ON entries(entry_seq)")

    conn.commit()
    return conn


def parse_jmdict(xml_path, db_conn):
    cursor = db_conn.cursor()

    context = ET.iterparse(xml_path, events=("start", "end"))
    context = iter(context)
    event, root = next(context)

    entry_count = 0
    batch_size = 1000
    entries_batch = []
    kanji_batch = []
    reading_batch = []
    senses_batch = []

    for event, elem in context:
        if event == "end" and elem.tag == "entry":
            entry_seq = None
            kanji_elements = []
            reading_elements = []
            senses = []

            for child in elem:
                if child.tag == "ent_seq":
                    entry_seq = int(child.text)
                elif child.tag == "k_ele":
                    for keb in child.findall("keb"):
                        if keb.text:
                            kanji_elements.append(keb.text)
                elif child.tag == "r_ele":
                    for reb in child.findall("reb"):
                        if reb.text:
                            reading_elements.append(reb.text)
                elif child.tag == "sense":
                    pos_list = []
                    gloss_list = []

                    for sense_child in child:
                        if sense_child.tag == "pos":
                            if sense_child.text:
                                pos_list.append(sense_child.text)
                        elif sense_child.tag == "gloss":
                            if sense_child.text:
                                gloss_list.append(sense_child.text)

                    pos_text = "; ".join(pos_list) if pos_list else None
                    gloss_text = "; ".join(gloss_list) if gloss_list else None

                    if gloss_text:
                        senses.append((pos_text, gloss_text))

            if entry_seq:
                entries_batch.append((entry_seq, entry_seq))

                for keb in kanji_elements:
                    kanji_batch.append((entry_seq, keb))

                for reb in reading_elements:
                    reading_batch.append((entry_seq, reb))

                for pos, gloss in senses:
                    senses_batch.append((entry_seq, pos, gloss))

                entry_count += 1

                if entry_count % batch_size == 0:
                    cursor.executemany(
                        "INSERT OR IGNORE INTO entries (id, entry_seq) VALUES (?, ?)",
                        entries_batch,
                    )
                    cursor.executemany(
                        "INSERT INTO kanji_elements (entry_id, keb) VALUES (?, ?)",
                        kanji_batch,
                    )
                    cursor.executemany(
                        "INSERT INTO reading_elements (entry_id, reb) VALUES (?, ?)",
                        reading_batch,
                    )
                    cursor.executemany(
                        "INSERT INTO senses (entry_id, pos, gloss) VALUES (?, ?, ?)",
                        senses_batch,
                    )
                    db_conn.commit()

                    entries_batch = []
                    kanji_batch = []
                    reading_batch = []
                    senses_batch = []

                    print(f"Processed {entry_count} entries...", flush=True)

            elem.clear()
            root.clear()

    if entries_batch:
        cursor.executemany(
            "INSERT OR IGNORE INTO entries (id, entry_seq) VALUES (?, ?)", entries_batch
        )
        cursor.executemany(
            "INSERT INTO kanji_elements (entry_id, keb) VALUES (?, ?)", kanji_batch
        )
        cursor.executemany(
            "INSERT INTO reading_elements (entry_id, reb) VALUES (?, ?)", reading_batch
        )
        cursor.executemany(
            "INSERT INTO senses (entry_id, pos, gloss) VALUES (?, ?, ?)", senses_batch
        )
        db_conn.commit()

    print(f"Total entries processed: {entry_count}")
    return entry_count


def main():
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    xml_path = project_root / "assets" / "JMdict_e.xml"
    db_path = project_root / "assets" / "jmdict.db"

    if not xml_path.exists():
        print(f"Error: JMdict_e.xml not found at {xml_path}")
        sys.exit(1)

    print(f"Converting {xml_path} to {db_path}...")
    print("This may take several minutes...")

    if db_path.exists():
        print(f"Removing existing database at {db_path}")
        db_path.unlink()

    conn = create_database(str(db_path))

    try:
        entry_count = parse_jmdict(str(xml_path), conn)
        print(f"\nSuccessfully created database with {entry_count} entries")
        print(f"Database saved to: {db_path}")
    except Exception as e:
        print(f"Error processing JMdict: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
    finally:
        conn.close()


if __name__ == "__main__":
    main()
