import os
import sqlite3

input_dir = "data/"
output_dir = "results/"

files = os.listdir(input_dir)
for filename in files:
    db = os.path.join(input_dir, filename)

    conn = sqlite3.connect(db)
    cursor = conn.cursor()

    cursor.execute("SELECT ID, NAME FROM WORKERS")
    workers = cursor.fetchall()

    cursor.execute("SELECT ID, POLICY FROM APPLICATIONS")
    applications = cursor.fetchall()

    cursor.execute("SELECT ID_APPLICATION, ID_WORKER FROM WORKERS_APPLICATIONS")
    workers_applications = cursor.fetchall()

    worker_group = {}
    for row in workers:
        worker_id = row[0]
        worker_name = row[1]
        if worker_name.startswith("Grupo1-"):
            group = "Grupo1"
        elif worker_name.startswith("Grupo2-"):
            group = "Grupo2"
        elif worker_name.startswith("Grupo3-"):
            group = "Grupo3"
        worker_group[worker_id] = group

    application_policy = {}
    for row in applications:
        app_id = row[0]
        policy = row[1]
        application_policy[app_id] = policy

    counts = {
        "efficient1": 0,
        "efficient2": 0,
        "efficient3": 0,
        "inefficient": 0
    }

    for allocation in workers_applications:
        app_id = allocation[0]
        worker_id = allocation[1]
        
        policy = application_policy[app_id]
        group = worker_group[worker_id]

        if policy == "performance" and group == "Grupo1":
            counts["efficient1"] += 1
        elif policy == "storage" and group == "Grupo2":
            counts["efficient2"] += 1
        elif policy == "transmission" and group == "Grupo3":
            counts["efficient3"] += 1
        else:
            counts["inefficient"] += 1

    total = len(workers_applications)

    percent = {}

    for key in counts:
        percent[key] = round(counts[key] * 100.0 / total, 2)

    output_name = filename.replace(".db", ".csv")
    output_name = output_name.replace("sqlite-", "pefficient_")
    output_path = os.path.join(output_dir, output_name)
    f = open(output_path, "w")
    f.write("total,pefficient,gefficient\n")
    for key in counts:
        line = str(counts[key]) + "," + str(percent[key]) + "," + key + "\n"
        f.write(line)
    f.close()