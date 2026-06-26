"""
convert_airlines.py
--------------------
Downloads the OpenFlights airlines database and converts it to the
airlines.json format expected by the SkyTracker Lambda function.

Output format:
  { "AAL": "American Airlines", "IGO": "IndiGo", ... }

Usage:
  python convert_airlines.py

Output file: airlines.json  (ready to upload to your S3 bucket)
"""

import urllib.request
import csv
import json
import io

SOURCE_URL = "https://raw.githubusercontent.com/jpatokal/openflights/master/data/airlines.dat"

# Column indices in airlines.dat
# 0: ID, 1: Name, 2: Alias, 3: IATA, 4: ICAO, 5: Callsign, 6: Country, 7: Active
COL_NAME   = 1
COL_ICAO   = 4
COL_ACTIVE = 7

def download_dat(url):
    print(f"Downloading from {url} ...")
    with urllib.request.urlopen(url) as response:
        return response.read().decode("utf-8")

def convert(raw_csv):
    airline_map = {}
    skipped = 0

    reader = csv.reader(io.StringIO(raw_csv))
    for row in reader:
        if len(row) < 8:
            skipped += 1
            continue

        name   = row[COL_NAME].strip()
        icao   = row[COL_ICAO].strip().upper()
        active = row[COL_ACTIVE].strip()

        # Skip inactive airlines, unknown/blank ICAO codes
        if active != "Y":
            skipped += 1
            continue
        if not icao or icao == "N/A" or icao == "-" or len(icao) < 2:
            skipped += 1
            continue
        # Skip placeholder names
        if name in ("", "Unknown", "-"):
            skipped += 1
            continue

        # First entry wins — avoids overwriting with duplicates
        if icao not in airline_map:
            airline_map[icao] = name

    return airline_map, skipped

def main():
    raw = download_dat(SOURCE_URL)
    airline_map, skipped = convert(raw)

    output_path = "airlines.json"
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(airline_map, f, ensure_ascii=False, indent=2, sort_keys=True)

    print(f"Done! {len(airline_map)} airlines written to {output_path}")
    print(f"Skipped {skipped} rows (inactive / missing ICAO / unknown name)")
    print(f"\nUpload to S3:")
    print(f"  aws s3 cp {output_path} s3://your-bucket-name/airlines.json")

    # Quick sanity check — print a few Indian / Gulf airlines
    samples = ["AIC", "IGO", "UAE", "ETD", "QTR", "SEJ", "AIX"]
    print("\nSample entries:")
    for code in samples:
        print(f"  {code}: {airline_map.get(code, '(not found)')}")

if __name__ == "__main__":
    main()
