#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include "structs.h"

class Database {
public:
    Database();
    ~Database();
    bool ExecuteQuery(const std::string &query);
    void AddNodeToDatabase(double power, double initial_consumption, double current_consumption, double cpu, double memory, double transmission, double storage, std::string node_name);
    void UpdateNodeResources(int workerId, double updatedPower);
    void AddAppToDatabase(double currentTime, std::string policy, float start, float duration, double cpu, double memory, double storage);
    void InsertWorkerApplication(int idWorker, int idApplication, double performedAt);
    void RemoveWorkerApplication(int idWorker, int idApplication, double currentTime);
    void MarkApplicationStatus(int appId, std::string status, double currentTime);
    double HibridPolicy();
    void DropAllTables();
    void CreateAllTables();
    std::vector<int> GetApplicationsToReallocate();
    APP SelectApplicationById(int id);
    WRK SelectWorker(float cpu, float memory, float storage, const char *policy, bool balanced);

private:
    sqlite3 *db;
};

#endif // DATABASE_H
