#include "database.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>

Database::Database() {
    if (sqlite3_open("scratch/database.db", &db) != SQLITE_OK) {
        std::cerr << "Erro ao abrir o banco de dados: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Banco de dados aberto com sucesso" << std::endl;
    }
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
        std::cout << "Banco de dados fechado." << std::endl;
    }
}

void Database::AddNodeToDatabase(double power, double initial_consumption, double current_consumption, double cpu, double memory, double transmission, double storage, std::string node_name)
{
    std::ostringstream oss;
    oss << "INSERT INTO WORKERS (POWER, INITIAL_CONSUMPTION, CURRENT_CONSUMPTION, CPU, MEMORY, TRANSMISSION, STORAGE, NAME) VALUES ("
        << power << ", " 
        << initial_consumption << ", " 
        << current_consumption << ", " 
        << cpu << ", " 
        << memory << ", " 
        << transmission << ", " 
        << storage << ", "
        << "'" << node_name << "');";
    
    if (ExecuteQuery(oss.str().c_str()))
    {
        std::cout << "Inserting Node " << node_name << " to database with:\n"
                << "  > Power = " << power << ";\n"
                << "  > Initial Consumption = " << initial_consumption << ";\n"
                << "  > Current Consumption = " << current_consumption << ";\n"
                << "  > CPU = " << cpu << ";\n"
                << "  > Memory = " << memory << ";\n"
                << "  > Transmission = " << transmission << ";\n"
                << "  > Storage = " << storage << ";\n"
                << std::endl;
    }
}

void Database::UpdateNodeResources(int workerId, double updatedPower)
{
    std::ostringstream oss;
    oss << "UPDATE WORKERS SET "
        << "POWER = " << updatedPower
        << " WHERE ID = " << workerId << ";";

    ExecuteQuery(oss.str());
}


void Database::AddAppToDatabase(double currentTime, std::string policy, float start, float duration, double cpu, double memory, double storage)
{
    std::ostringstream oss;
    oss << "INSERT INTO APPLICATIONS (START, DURATION, FINISH, CPU, MEMORY, STORAGE, POLICY) VALUES ("
        << start << ", " << duration << ", 0, " << cpu << ", " << memory << ", " << storage << ", "
        << "\"" << policy << "\");";
    

    if (ExecuteQuery(oss.str().c_str()))
    {
        std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: ";
        std::cout << "Inserting Application " << policy << " to database with:\n"
                << "  > Start = " << start << ";\n"
                << "  > Duration = " << duration << ";\n"
                << "  > CPU = " << cpu << ";\n"
                << "  > Memory = " << memory << ";\n"
                << "  > Storage = " << storage << ";\n"
                << std::endl;
    }
}

APP Database::SelectApplicationById(int id)
{
    APP application;

    std::ostringstream oss;
    oss << "SELECT * FROM APPLICATIONS WHERE ID = " << id << ";";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, oss.str().c_str(), -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        application.ID = sqlite3_column_int(stmt, 0);
        application.START = static_cast<float>(sqlite3_column_double(stmt, 1));
        application.DURATION = static_cast<float>(sqlite3_column_double(stmt, 2));
        application.FINISH = sqlite3_column_int(stmt, 3);
        application.CPU = static_cast<float>(sqlite3_column_double(stmt, 4));
        application.MEMORY = static_cast<float>(sqlite3_column_double(stmt, 5));
        application.STORAGE = static_cast<float>(sqlite3_column_double(stmt, 6));

        const char *policyText = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
        if (policyText) {
            strncpy(application.POLICY, policyText, sizeof(application.POLICY) - 1);
            application.POLICY[sizeof(application.POLICY) - 1] = '\0';
        }
    }

    sqlite3_finalize(stmt);

    return application;
}

WRK Database::SelectWorker(float cpu, float memory, float storage, const char *policy, bool balanced)
{
    std::ostringstream oss;
    oss << "WITH FILTERED_WORKERS AS ("
        << "SELECT "
        << "WORKERS.ID AS worker, "
        << "WORKERS.POWER AS battery, "
        << "WORKERS.INITIAL_CONSUMPTION AS initial_consumption, "
        << "WORKERS.CURRENT_CONSUMPTION + (COUNT(APPLICATIONS.ID) * WORKERS.INITIAL_CONSUMPTION) AS current_consumption, "
        << "WORKERS.CPU - COALESCE(SUM(APPLICATIONS.CPU), 0) AS cpu_remaining, "
        << "WORKERS.MEMORY - COALESCE(SUM(APPLICATIONS.MEMORY), 0) AS memory_remaining, "
        << "WORKERS.TRANSMISSION AS transmission, "
        << "WORKERS.STORAGE - COALESCE(SUM(APPLICATIONS.STORAGE), 0) AS storage_remaining, "
        << "COUNT(APPLICATIONS.ID) AS application_quantity "
        << "FROM WORKERS "
        << "LEFT JOIN WORKERS_APPLICATIONS ON WORKERS_APPLICATIONS.ID_WORKER = WORKERS.ID "
        << "LEFT JOIN APPLICATIONS ON APPLICATIONS.ID = WORKERS_APPLICATIONS.ID_APPLICATION AND APPLICATIONS.FINISH = 2 "
        << "GROUP BY WORKERS.ID"
        << ") "
        << "SELECT * FROM FILTERED_WORKERS "
        << "WHERE "
        << "cpu_remaining >= " << cpu << " AND "
        << "memory_remaining >= " << memory << " AND "
        << "storage_remaining >= " << storage << " AND "
        << "battery > 50 "
        << "ORDER BY ";

    if (balanced)
    {
        oss << "application_quantity ASC, ";
    }

    if (strcmp(policy, "performance") == 0)
    {
        oss << "cpu_remaining DESC ";
    }
    else if (strcmp(policy, "storage") == 0)
    {
        oss << "storage_remaining DESC ";
    }
    else if (strcmp(policy, "transmission") == 0)
    {
        oss << "transmission DESC ";
    }

    oss << "LIMIT 1;";

    sqlite3_stmt *stmt;

    //std::cout << oss.str().c_str() << std::endl;

    WRK worker = {0};
    if (sqlite3_prepare_v2(db, oss.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    {
        worker.ID = sqlite3_column_int(stmt, 0);
        worker.POWER = static_cast<float>(sqlite3_column_double(stmt, 1));
        worker.INITIAL_CONSUMPTION = static_cast<float>(sqlite3_column_double(stmt, 2));
        worker.CURRENT_CONSUMPTION = static_cast<float>(sqlite3_column_double(stmt, 3));
        worker.CPU = static_cast<float>(sqlite3_column_double(stmt, 4));
        worker.MEMORY = static_cast<float>(sqlite3_column_double(stmt, 5));
        worker.TRANSMISSION = static_cast<float>(sqlite3_column_double(stmt, 6));
        worker.STORAGE = static_cast<float>(sqlite3_column_double(stmt, 7));

        std::cout << "Worker Found:\n"
                  << "  ID: " << worker.ID << "\n"
                  << "  Power: " << worker.POWER << "\n"
                  << "  Initial Consumption: " << worker.INITIAL_CONSUMPTION << "\n"
                  << "  Current Consumption: " << worker.CURRENT_CONSUMPTION << "\n"
                  << "  CPU: " << worker.CPU << "\n"
                  << "  Memory: " << worker.MEMORY << "\n"
                  << "  Transmission: " << worker.TRANSMISSION << "\n"
                  << "  Storage: " << worker.STORAGE << "\n";
    }

    sqlite3_finalize(stmt);
    return worker;
}

void Database::InsertWorkerApplication(int idWorker, int idApplication, double performedAt)
{
    std::ostringstream oss;
    oss << "INSERT INTO WORKERS_APPLICATIONS (ID_WORKER, ID_APPLICATION, PERFORMED_AT, FINISHED_AT) "
        << "VALUES (" << idWorker << ", " << idApplication << ", " << performedAt << ", 0);";

    if (ExecuteQuery(oss.str().c_str()))
    {
        std::cout << "At time " << std::to_string(performedAt).substr(0, std::to_string(performedAt).find(".") + 2) << "s: "
              << "Inserting Worker Application into database with:\n"
              << "  > ID Worker = " << idWorker << ";\n"
              << "  > ID Application = " << idApplication << ";\n"
              << "  > Performed At = " << performedAt << ";"
              << std::endl;
    }
}

void Database::RemoveWorkerApplication(int idWorker, int idApplication, double currentTime)
{
    std::ostringstream oss;
    oss << "UPDATE WORKERS_APPLICATIONS "
        << "SET FINISHED_AT = " << currentTime
        << " WHERE ID_WORKER = " << idWorker
        << " AND ID_APPLICATION = " << idApplication;

    if (ExecuteQuery(oss.str().c_str()))
    {
        std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: "
                  << "Removing Worker Application from database with:\n"
                  << "  > ID Worker = " << idWorker << ";\n"
                  << "  > ID Application = " << idApplication << ";\n"
                  << "  > Finished At = " << currentTime << ";"
                  << std::endl;
    }
}

std::vector<int> Database::GetApplicationsToReallocate()
{
    std::vector<int> result;

    std::string query = "SELECT ID FROM APPLICATIONS WHERE FINISH = 3;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int appId = sqlite3_column_int(stmt, 0);
            result.push_back(appId);
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

void Database::MarkApplicationStatus(int appId, std::string status, double currentTime)
{
    std::ostringstream oss;
    oss << "UPDATE APPLICATIONS SET FINISH = " << status << " WHERE ID = " << appId << ";";

    if (ExecuteQuery(oss.str()))
    {
        std::cout << "At time " << std::to_string(currentTime).substr(0, std::to_string(currentTime).find(".") + 2) << "s: ";
        std::cout << "Updated Application finish status in database with:\n"
                  << "  > ID = " << appId << "\n"
                  << "  > FINISH = " << status << ";\n";
    }
}

bool Database::ExecuteQuery(const std::string &query) {
    char *errMsg = nullptr;
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Erro ao executar query: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void Database::CreateAllTables()
{
    const char* sql_workers = R"(
        CREATE TABLE WORKERS (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            POWER FLOAT NOT NULL,
            INITIAL_CONSUMPTION FLOAT NOT NULL,
            CURRENT_CONSUMPTION FLOAT NOT NULL,
            CPU FLOAT NOT NULL,
            MEMORY FLOAT NOT NULL,
            TRANSMISSION FLOAT NOT NULL,
            STORAGE FLOAT NOT NULL,
            NAME TEXT NOT NULL
        );
    )";
    ExecuteQuery(sql_workers);

    const char* sql_applications = R"(
        CREATE TABLE APPLICATIONS(
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            START FLOAT NOT NULL,
            DURATION FLOAT NOT NULL,
            FINISH INTEGER NOT NULL,
            CPU FLOAT NOT NULL,
            MEMORY FLOAT NOT NULL,
            STORAGE FLOAT NOT NULL,
            POLICY TEXT NOT NULL
        );
    )";
    ExecuteQuery(sql_applications);

    const char* sql_workers_applications = R"(
        CREATE TABLE WORKERS_APPLICATIONS(
            ID_WORKER INTEGER NOT NULL,
            ID_APPLICATION INTEGER NOT NULL,
            PERFORMED_AT FLOAT NOT NULL,
            FINISHED_AT FLOAT NOT NULL,
            FOREIGN KEY(ID_WORKER) REFERENCES WORKERS(ID),
            FOREIGN KEY(ID_APPLICATION) REFERENCES APPLICATIONS(ID),
            UNIQUE(ID_WORKER, ID_APPLICATION, PERFORMED_AT)
        );
    )";
    ExecuteQuery(sql_workers_applications);
}

void Database::DropAllTables()
{
    const char sql[] =
        "DROP TABLE IF EXISTS WORKERS;"
        "DROP TABLE IF EXISTS APPLICATIONS;"
        "DROP TABLE IF EXISTS WORKERS_APPLICATIONS;";

    if (ExecuteQuery(sql))
    {
        std::cout << "All database tables dropped successfully." << std::endl;
    }
    else
    {
        std::cerr << "Failed to drop database tables." << std::endl;
    }
}

double Database::HibridPolicy()
{
    double avgPower = 0.0;

    const char *sql = "SELECT AVG(POWER) FROM WORKERS;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            avgPower = sqlite3_column_double(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);

    return avgPower;
}