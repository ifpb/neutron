import os
import sqlite3

input_dir = "data/"
output_dir = "results/"

results = {}

files = os.listdir(input_dir)

for filename in files:
    db = os.path.join(input_dir, filename)

    parts = filename.replace(".db", "").split("-")

    scenario = parts[1]
    policy = parts[2]
    exec_id = int(parts[3])

    print(scenario, policy, exec_id)

    conn = sqlite3.connect(db)
    cursor = conn.cursor()

    cursor.execute("SELECT ID FROM WORKERS")
    workers = cursor.fetchall()

    total_workers = 0
    for w in workers:
        total_workers += 1

    cursor.execute("SELECT ID_WORKER FROM WORKERS_APPLICATIONS")
    allocations = cursor.fetchall()

    used_workers = []
    for alloc in allocations:
        wid = alloc[0]
        if wid not in used_workers:
            used_workers.append(wid)

    used_count = 0
    for w in used_workers:
        used_count += 1

    pused = round((used_count * 100.0) / total_workers, 2)

    conn.close()

    if scenario not in results:
        results[scenario] = {}

    if exec_id not in results[scenario]:
        results[scenario][exec_id] = {
            "bal": None,
            "sat": None,
            "hib": None
        }

    results[scenario][exec_id][policy] = pused

for scenario in results:

    output_path = os.path.join(output_dir, f"pused_{scenario}.csv")

    f = open(output_path, "w")
    f.write("puBal,puSat,puHib\n")

    for exec_id in sorted(results[scenario].keys()):
        bal = results[scenario][exec_id]["bal"]
        sat = results[scenario][exec_id]["sat"]
        hib = results[scenario][exec_id]["hib"]

        line = f"{bal:.2f},{sat:.2f},{hib:.2f}\n"
        f.write(line)

    f.close()