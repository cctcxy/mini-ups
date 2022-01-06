#include "db.h"

void DBWrapper::createTables(){
    std::string sql = "DROP TABLE IF EXISTS truck CASCADE;"
    "CREATE TABLE truck("
    "id     INT PRIMARY KEY NOT NULL,"
    "status INT NOT NULL,"
    "x      INT NOT NULL,"
    "y      INT NOT NULL"
    ");";
    // package status
    // 0: send truck to warehouse
    // 1: wait for Amazon to say it’s loaded
    // 2: send it for delivery
    // 3: delivered
    sql += "DROP TABLE IF EXISTS package CASCADE;"
    "CREATE TABLE package("
    "shipid INT PRIMARY KEY NOT NULL,"
    "whnum  INT NOT NULL,"
    "status INT NOT NULL,"
    "dest_x INT NOT NULL,"
    "dest_y INT NOT NULL,"
    "descr  VARCHAR(100) NOT NULL,"
    "account    VARCHAR(100) NOT NULL,"         // should be foreign key
    "truck  INT NOT NULL,"
    "FOREIGN KEY (truck) REFERENCES truck (id)"
    ");";
    sql += "DROP TABLE IF EXISTS delivered_history CASCADE;"
    "CREATE TABLE delivered_history("
    "shipid INT NOT NULL,"
    "FOREIGN KEY (shipid) REFERENCES package (shipid)"
    ");";
    simple_exec(sql);
}

void DBWrapper::insertDeliveredHistory(int shipid){
    std::string sql = "INSERT INTO delivered_history (shipid) VALUES ( "
    + pqxx::to_string(shipid)
    + ");";
    simple_exec(sql);
}

std::vector<int> DBWrapper::getPackage_truckid_status(int tid, int status){
    std::string sql = "SELECT shipid FROM package "
    "WHERE status = " + pqxx::to_string(status) + " AND truck = " + pqxx::to_string(tid) + ";";
    pqxx::result r = exec(sql);
    std::vector<int> ret;
    for(const auto &i:r){
        ret.push_back(i[0].as<int>());
    }
    return ret;
}

std::pair<int, int> DBWrapper::getPackageDest(int shipid){
    std::string sql = "SELECT dest_x, dest_y FROM package "
    "WHERE shipid = " + pqxx::to_string(shipid) + ";";
    pqxx::result r = exec(sql);
    if(r.size()<1) throw std::runtime_error("package not found");
    return {r[0][0].as<int>(), r[0][1].as<int>()};
}

void DBWrapper::updatePackageStatus(int shipid, int status){
    std::string sql = "UPDATE package"
    " SET status = " + pqxx::to_string(status) +
    " WHERE shipid = " + pqxx::to_string(shipid) + 
    ";";
    simple_exec(sql);
}

void DBWrapper::updatePackageDest(int shipid, int x, int y){
    std::string sql = "UPDATE package"
    " SET dest_x = " + pqxx::to_string(x) +
    ", dest_y = " + pqxx::to_string(y) +
    " WHERE shipid = " + pqxx::to_string(shipid) + 
    ";";
    simple_exec(sql);
}

void DBWrapper::insertPackage(int shipid, int whnum, int dest_x, int dest_y, std::string descr, int truckid, std::string account, int status){
    std::string sql = "INSERT INTO package (shipid, whnum, status, dest_x, dest_y, descr, account, truck) VALUES ( "
    + pqxx::to_string(shipid)
    + ", " + pqxx::to_string(whnum)
    + ", " + pqxx::to_string(status)
    + ", " + pqxx::to_string(dest_x)
    + ", " + pqxx::to_string(dest_y)
    + ", '" + pqxx::to_string(descr) + "'"
    + ", '" + pqxx::to_string(account) + "'"
    + ", " + pqxx::to_string(truckid)
    + ");";
    simple_exec(sql);
}

int DBWrapper::getPackageTruck(int shipid){
    std::string sql = "SELECT truck FROM package "
    "WHERE shipid = " + pqxx::to_string(shipid) + ";";
    pqxx::result r = exec(sql);
    if(r.size()<1) throw std::runtime_error("truck not found");
    return r[0][0].as<int>();
}

void DBWrapper::insertTruck(int id, int status, int x, int y){
    std::string sql = "INSERT INTO truck (id, status, x, y) VALUES ( "
    + pqxx::to_string(id)
    + ", " + pqxx::to_string(status)
    + ", " + pqxx::to_string(x)
    + ", " + pqxx::to_string(y)
    + ");";
    simple_exec(sql);
}

void DBWrapper::updateTruck(int id, int status, int x, int y){
    std::string sql = "UPDATE truck"
    " SET status = " + pqxx::to_string(status) +
    ", x = " + pqxx::to_string(x) + 
    ", y = " + pqxx::to_string(y) + 
    " WHERE id = " + pqxx::to_string(id) + 
    ";";
    simple_exec(sql);
}

void DBWrapper::updateTruckStatus(int id, int status){
    std::string sql = "UPDATE truck"
    " SET status = " + pqxx::to_string(status) +
    " WHERE id = " + pqxx::to_string(id) + 
    ";";
    simple_exec(sql);
}

int DBWrapper::getVacantTruck(){
    std::string sql = "SELECT id FROM truck "
    "WHERE status = 0 OR status = 4 " // arrive warehouse should not be vacant, doesn't make sense
    // "WHERE status = 0 OR status = 2 OR status = 4 "
    "ORDER BY status ASC;";
    pqxx::result r = exec(sql);
    for(const auto &i:r){
        return i[0].as<int>();
    }
    return -1;
}