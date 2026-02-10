import os
import sqlite3

input_dir = "data/"
output_dir = "results/"

results = {}

files = os.listdir(input_dir)

for filename in files:
    if not filename.endswith(".db"):
        continue

    db = os.path.join(input_dir, filename)

    parts = filename.replace(".db", "").split("-")

    scenario = parts[1]
    policy = parts[2]
    exec_id = int(parts[3])

    print(scenario, policy, exec_id)

    conn = sqlite3.connect(db)
    cursor = conn.cursor()

    cursor.execute("SELECT ID, DURATION FROM APPLICATIONS")
    applications = cursor.fetchall()

    cursor.execute("SELECT ID_APPLICATION, PERFORMED_AT, FINISHED_AT FROM WORKERS_APPLICATIONS")
    executions = cursor.fetchall()

    conn.close()

    total = 0
    mismatches = 0

    for app in applications:
        app_id = app[0]
        duration = app[1]

        executed_time = 0.0
        has_execution = False

        for e in executions:
            if e[0] == app_id:
                performed_at = e[1]
                finished_at = e[2]

                if performed_at is None or finished_at is None:
                    continue

                executed_time += (finished_at - performed_at)
                has_execution = True

        if not has_execution:
            continue

        total += 1

        difference = abs(executed_time - duration)

        if difference >= 1.0:
            mismatches += 1

    if total > 0:
        preempted = round(mismatches * 100.0 / total, 2)
    else:
        preempted = 0.0

    if scenario not in results:
        results[scenario] = {}

    if exec_id not in results[scenario]:
        results[scenario][exec_id] = {
            "bal": None,
            "sat": None,
            "hib": None
        }

    results[scenario][exec_id][policy] = preempted

for scenario in results:

    output_path = os.path.join(
        output_dir,
        f"ppreempted_{scenario}.csv"
    )

    f = open(output_path, "w")
    f.write("ppBal,ppSat,ppHib\n")

    for exec_id in sorted(results[scenario].keys()):
        bal = results[scenario][exec_id]["bal"]
        sat = results[scenario][exec_id]["sat"]
        hib = results[scenario][exec_id]["hib"]

        line = f"{bal:.2f},{sat:.2f},{hib:.2f}\n"
        f.write(line)

    f.close()
