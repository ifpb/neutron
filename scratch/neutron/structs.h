#ifndef STRUCTS_H
#define STRUCTS_H

#include <string>
#include <vector>

struct APP
{
    int ID;
    float START;
    float DURATION;
    int FINISH;
    float CPU;
    float MEMORY;
    float STORAGE;
    char POLICY[20];
}; 

struct WRK
{
    int ID;
    float POWER;
    float INITIAL_CONSUMPTION;
    float CURRENT_CONSUMPTION;
    float CPU;
    float MEMORY;
    float TRANSMISSION;
    float STORAGE;
    std::vector<int> APPS;
};

#endif