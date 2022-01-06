#include "util.h"
#include "app.h"
#include "db.h"
#include "util.h"

#include "world_ups.pb.h"
#include "u2a.pb.h"
#include "a2u.pb.h"
#include <fstream>
#include <string>

using namespace std;

a2u::request_truck fake_request_truck(){
    a2u::request_truck rt;
    rt.set_whnum(1);
    rt.set_item_desc("this is item description");
    rt.set_ups_account_id("abc123");
    rt.set_location_x(11);
    rt.set_location_y(22);
    rt.set_shipid(1234);
    return rt;
}

a2u::ready_for_delivery fake_ready_for_delivery(){
    a2u::ready_for_delivery rfd;
    rfd.set_shipid(1234);
    return rfd;
}
a2u::change_destination fake_change_destination(){
    a2u::change_destination cd;
    cd.set_shipid(1234);
    cd.set_location_x(111);
    cd.set_location_y(222);
    return cd;
}

UFinished fake_UFinished1(){
    UFinished uf;
    uf.set_seqnum(20);
    uf.set_status("arrive warehouse");
    uf.set_truckid(0);
    uf.set_x(1);
    uf.set_y(1);
    return uf;
}

UFinished fake_UFinished2(){
    UFinished uf;
    uf.set_seqnum(23);
    uf.set_status("IDLE");
    uf.set_truckid(0);
    uf.set_x(1);
    uf.set_y(1);
    return uf;
}

UDeliveryMade fake_UDeliveryMade(){
    UDeliveryMade udm;
    udm.set_seqnum(21);
    udm.set_packageid(1234);
    udm.set_truckid(0);
    return udm;
}

UTruck fake_UTruck(){
    UTruck ut;
    ut.set_seqnum(22);
    ut.set_status("IDLE");
    ut.set_truckid(0);
    ut.set_x(0);
    ut.set_y(0);
    return ut;
}

int main(int argc, const char * argv[]){
#if IS_DB_INIT
    DBWrapper().createTables();
    cout << "create table" << endl;
#endif
#if !CONFIG_HOST
    BackendApp app;
#else
    ifstream in("config");
    string line, world_address, world_port, amazon_address, amazon_port;
    getline(in, world_address);
    getline(in, world_port);
    getline(in, amazon_address);
    getline(in, amazon_port);
    cout << "world: " << world_address << ":" << world_port << " amazon: " << amazon_address << ":" << amazon_port << endl;
    BackendApp app(world_address.c_str(), world_port.c_str(), amazon_address.c_str(), amazon_port.c_str());
#endif
    DBWrapper db;

    // app.setSimSpeed(10000);
#if UNIT_TEST
    auto rt0 = fake_request_truck();
    rt0.set_ups_account_id("xa");
    app.requestTruckHandler(rt0);
    sleep(2);
    app.UFinishedHandler(fake_UFinished1());
    sleep(2);
    app.readyForDeliveryHandler(fake_ready_for_delivery());
    sleep(2);
    app.changeDestinationHandler(fake_change_destination());
    sleep(2);
    app.UDeliveryMadeHandler(fake_UDeliveryMade());
    sleep(2);
    app.UFinishedHandler(fake_UFinished2());
    sleep(2);
    app.UTruckHandler(fake_UTruck());
    sleep(2);
    auto rt1 = fake_request_truck();
    rt1.set_shipid(1);
    rt1.set_ups_account_id("ax");
    app.requestTruckHandler(rt1);
    auto rt2 = fake_request_truck();
    rt2.set_shipid(2);
    rt2.set_ups_account_id("xa");
    app.requestTruckHandler(rt2);
    sleep(2);
    auto rfd = fake_ready_for_delivery();
    rfd.set_shipid(2);
    app.readyForDeliveryHandler(rfd);
#endif

    app.listen();
    return 0;
}