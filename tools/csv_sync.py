"""
Automatische tweeweg-synchronisatie voor Pepper C1.

Start vanuit de projectmap met:
    py tools/csv_sync.py

Vereist:
    py -m pip install pyserial
"""

import csv
import os
import sys
import time
import tempfile
from pathlib import Path

import serial


# --------------------------------------------------
# INSTELLINGEN
# --------------------------------------------------

PORT = "COM3"
BAUD = 115200

CSV_PATH = (
    Path(__file__).resolve().parents[1]
    / "data"
    / "tags.csv"
)

POLL_SECONDS = 0.25


# --------------------------------------------------
# CSV FUNCTIES
# --------------------------------------------------

def clean(value):
    value = (value or "").strip()

    if any(ch in value for ch in (",", "\t", "\r", "\n")):
        raise ValueError(
            "Velden mogen geen komma's, tabs of regeleinden bevatten"
        )

    return value


def read_csv():
    rows = {}

    if not CSV_PATH.exists():
        CSV_PATH.parent.mkdir(parents=True, exist_ok=True)

        with CSV_PATH.open("w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(
                ["UID", "Consumption", "Generation"]
            )

    with CSV_PATH.open(
        "r",
        newline="",
        encoding="utf-8-sig"
    ) as f:

        reader = csv.DictReader(f)

        required = {
            "UID",
            "Consumption",
            "Generation"
        }

        if not required.issubset(
            set(reader.fieldnames or [])
        ):
            raise ValueError(
                "CSV moet de kolommen "
                "UID,Consumption,Generation bevatten"
            )

        for row in reader:
            uid = clean(row.get("UID", "")).upper()

            if not uid:
                continue

            rows[uid] = (
                clean(row.get("Consumption", "")),
                clean(row.get("Generation", ""))
            )

    return rows


def write_csv(rows):
    CSV_PATH.parent.mkdir(parents=True, exist_ok=True)

    fd, temp_name = tempfile.mkstemp(
        prefix="tags_",
        suffix=".csv",
        dir=str(CSV_PATH.parent)
    )

    try:
        with os.fdopen(
            fd,
            "w",
            newline="",
            encoding="utf-8"
        ) as f:

            writer = csv.writer(f)

            writer.writerow(
                ["UID", "Consumption", "Generation"]
            )

            for uid in sorted(rows):
                writer.writerow([
                    uid,
                    rows[uid][0],
                    rows[uid][1]
                ])

        os.replace(temp_name, CSV_PATH)

    finally:
        if os.path.exists(temp_name):
            os.remove(temp_name)


# --------------------------------------------------
# SERIELE COMMUNICATIE
# --------------------------------------------------

def parse_tag_line(line):
    if not line.startswith("TAG\t"):
        return None

    parts = line.split("\t")

    if len(parts) != 4:
        return None

    uid = clean(parts[1]).upper()

    if not uid:
        return None

    return (
        uid,
        clean(parts[2]),
        clean(parts[3])
    )


def collect_initial_esp32_data(port):
    """
    Leest de bestaande TAG-regels die de ESP32 na SYNC_READY
    verstuurt. Stopt wanneer er even geen nieuwe regels komen.
    """

    esp_rows = {}
    last_data_time = time.time()

    print("[SYNC] Bestaande ESP32-gegevens ophalen...")

    while time.time() - last_data_time < 0.8:
        raw = port.readline()

        if not raw:
            continue

        line = raw.decode(
            "utf-8",
            errors="replace"
        ).strip()

        if not line:
            continue

        tag = parse_tag_line(line)

        if tag:
            uid, consumption, generation = tag

            esp_rows[uid] = (
                consumption,
                generation
            )

            last_data_time = time.time()

        elif line.startswith("SYNC_") or "ERROR" in line:
            print(f"[ESP32] {line}")

    print(
        f"[SYNC] {len(esp_rows)} UID(s) ontvangen van ESP32."
    )

    return esp_rows


def merge_initial_data(pc_rows, esp_rows):
    """
    Computerwaarden hebben voorrang als ze ingevuld zijn.
    Lege computerwaarden worden aangevuld met ESP32-waarden.
    """

    merged = dict(pc_rows)

    for uid, esp_values in esp_rows.items():

        if uid not in merged:
            merged[uid] = esp_values
            continue

        pc_consumption, pc_generation = merged[uid]
        esp_consumption, esp_generation = esp_values

        consumption = (
            pc_consumption
            if pc_consumption
            else esp_consumption
        )

        generation = (
            pc_generation
            if pc_generation
            else esp_generation
        )

        merged[uid] = (
            consumption,
            generation
        )

    return merged


def send_snapshot(port, rows):
    print(
        f"[SYNC] {len(rows)} CSV-regel(s) naar ESP32 sturen..."
    )

    for uid, values in rows.items():

        consumption, generation = values

        command = (
            f"SET\t{uid}\t"
            f"{consumption}\t"
            f"{generation}\n"
        )

        port.write(command.encode("utf-8"))

        time.sleep(0.015)

    port.flush()

    print("[SYNC] Verzenden klaar.")


# --------------------------------------------------
# HOOFDPROGRAMMA
# --------------------------------------------------

def main():

    if not CSV_PATH.exists():
        write_csv({})

    print()
    print("====================================")
    print("       PEPPER C1 CSV SYNC")
    print("====================================")
    print(f"Poort: {PORT}")
    print(f"Baudrate: {BAUD}")
    print(f"CSV: {CSV_PATH}")
    print()
    print(
        "Sluit eerst de PlatformIO Serial Monitor."
    )
    print()

    try:

        with serial.Serial(
            PORT,
            BAUD,
            timeout=0.2,
            write_timeout=2
        ) as port:

            # Openen van COM3 reset meestal de ESP32.
            # Wachten totdat de firmware gereed is.

            ready = False
            deadline = time.time() + 30

            while time.time() < deadline and not ready:

                raw = port.readline()

                if not raw:
                    continue

                line = raw.decode(
                    "utf-8",
                    errors="replace"
                ).strip()

                if line:
                    print(f"[ESP32] {line}")

                if line == "SYNC_READY":
                    ready = True

            if not ready:
                raise TimeoutError(
                    "Geen SYNC_READY ontvangen. "
                    "Controleer USB, COM3 en firmware."
                )

            # Eerst de bestaande ESP32-gegevens ophalen.
            esp_rows = collect_initial_esp32_data(port)

            # Computer-CSV en ESP32-data samenvoegen.
            pc_rows = read_csv()

            merged_rows = merge_initial_data(
                pc_rows,
                esp_rows
            )

            write_csv(merged_rows)

            # De samengevoegde gegevens naar de ESP32 sturen.
            send_snapshot(port, merged_rows)

            last_mtime = CSV_PATH.stat().st_mtime_ns

            print()
            print("[SYNC] Synchronisatie actief.")
            print("[SYNC] Pas data/tags.csv aan om waarden te wijzigen.")
            print("[SYNC] Druk CTRL+C om te stoppen.")
            print()

            # Doorlopend synchroniseren.
            while True:

                raw = port.readline()

                if raw:

                    line = raw.decode(
                        "utf-8",
                        errors="replace"
                    ).strip()

                    if line:

                        tag = parse_tag_line(line)

                        if tag:
                            uid, consumption, generation = tag

                            rows = read_csv()

                            if uid not in rows:

                                rows[uid] = (
                                    consumption,
                                    generation
                                )

                                write_csv(rows)

                                print(
                                    f"[CSV] Nieuwe UID toegevoegd: {uid}"
                                )

                                last_mtime = (
                                    CSV_PATH.stat().st_mtime_ns
                                )

                            else:
                                print(
                                    f"[RFID] UID bekend: {uid}"
                                )

                        elif (
                            line.startswith("SYNC_")
                            or "ERROR" in line
                        ):
                            print(f"[ESP32] {line}")

                        elif line.startswith("["):
                            print(f"[ESP32] {line}")

                # Controleren of de computer-CSV gewijzigd is.
                current_mtime = CSV_PATH.stat().st_mtime_ns

                if current_mtime != last_mtime:

                    # Kleine pauze zodat het bestand volledig opgeslagen is.
                    time.sleep(0.15)

                    rows = read_csv()

                    send_snapshot(port, rows)

                    last_mtime = CSV_PATH.stat().st_mtime_ns

                time.sleep(POLL_SECONDS)

    except serial.SerialException as exc:

        print(f"COM-fout: {exc}")

        print(
            "Controleer of COM3 klopt en sluit de Serial Monitor."
        )

        sys.exit(1)

    except KeyboardInterrupt:

        print("\nSynchronisatie gestopt.")

    except Exception as exc:

        print(f"Fout: {exc}")

        sys.exit(1)


if __name__ == "__main__":
    main()