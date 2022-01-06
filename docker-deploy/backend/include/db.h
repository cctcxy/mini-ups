#ifndef db_h
#define db_h

#include <pqxx/pqxx>
#include <vector>
#include <iostream>

static const char *dblogin = "dbname = postgres user = postgres password = postgres host = db port = 5432";

class DBWrapper{
private:
    pqxx::connection C;

    inline pqxx::result exec(std::string &sql){
        pqxx::work W{C};
        try{
            pqxx::result r{W.exec(sql)};
            W.commit();
            return r;
        } catch(...){
            W.abort();
            throw;
        }
    }

    inline void simple_exec(std::string &sql){
        pqxx::work W{C};
        try{
            W.exec(sql);
            W.commit();
        } catch(...){
            W.abort();
            throw;
        }
    }

public:
    DBWrapper(): C(dblogin){}

    ~DBWrapper(){
        C.disconnect();
    }

    void createTables();
    void insertDeliveredHistory(int);
    std::vector<int> getPackage_truckid_status(int, int);
    std::pair<int, int> getPackageDest(int);
    void updatePackageStatus(int, int);
    void updatePackageDest(int, int, int);
    void insertPackage(int, int, int, int, std::string, int, std::string = "", int = 0);
    int getPackageTruck(int);
    void insertTruck(int, int, int, int);
    void updateTruck(int, int, int, int);
    void updateTruckStatus(int, int);
    int getVacantTruck();
};

#endif