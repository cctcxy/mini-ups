#include "app.h"

int BackendApp::_connect(const char *address, const char *port){
    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(address, port, &hints, &servinfo)){
        freeaddrinfo(servinfo);
        throw std::runtime_error(std::string("getaddrinfo error, ") + 
        "address: " + address + ", port: " + port + "\nReason: " + strerror(errno));
    }

    int fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(fd < 0){
        freeaddrinfo(servinfo);
        throw std::runtime_error(std::string("socket error, ") + 
        "address: " + address + ", port: " + port + "\n" + strerror(errno));
    }

    if(connect(fd, servinfo->ai_addr, servinfo->ai_addrlen)){
        freeaddrinfo(servinfo);
        throw std::runtime_error(std::string("connect error, ") + 
        "address: " + address + ", port: " + port + "\n" + strerror(errno));
    }

    freeaddrinfo(servinfo);
    return fd;
}

void BackendApp::_UFinishedHandler(const UFinished &msg){
    try{
        DBWrapper db;
        //update truck
        int tid = msg.truckid(), x = msg.x(), y = msg.y();
        std::string status = msg.status();
        if(status == "IDLE"){
            // truck complete delivery
            db.updateTruck(tid, 0, x, y);
        } else{
            std::cout << "not idle status: " << status << std::endl;
            // truck arrived
            db.updateTruck(tid, 2, x, y);
            u2a::UToA u2a;
            auto ta = u2a.mutable_truckarrived();
            // query for shipid
            auto shipids = db.getPackage_truckid_status(tid, 0);
            std::cout << "there are " << shipids.size() << " shipid's with tid = " << tid << " and status = 0" << std::endl;
            assert(shipids.size() == 1);
            ta->set_shipid(shipids[0]);
            ta->set_truckid(tid);
            sendToAmazon(u2a);
            db.updatePackageStatus(shipids[0], 1);
        }
        std::cout << "UFinished handled" << std::endl;
    } catch(std::exception &e){
        std::cerr << "fatal _UFinishedHandler error:\n" << e.what() << std::endl;
    }
}

void BackendApp::_UDeliveryMadeHandler(const UDeliveryMade &msg){
    try{
        DBWrapper db;
        int tid = msg.truckid(), shipid = msg.packageid();
        db.updatePackageStatus(shipid, 3);
        db.insertDeliveredHistory(shipid);
        u2a::UToA u2a;
        auto dlvd = u2a.mutable_delivered();
        dlvd->set_shipid(shipid);
        sendToAmazon(u2a);  
        std::cout << "UDeliveryMade handled" << std::endl;  
    } catch(std::exception &e){
        std::cerr << "fatal _UDeliveryMadeHandler error:\n" << e.what() << std::endl;
    } 
}

void BackendApp::_UTruckHandler(const UTruck &msg){
    try{
        DBWrapper db;
        if(!status_number.count(msg.status())){
            std::cout << msg.status() << std::endl;
            return;
        }
        db.updateTruck(msg.truckid(), status_number[msg.status()], msg.x(), msg.y());
        std::cout << "UTruck handled" << std::endl;
    } catch(std::exception &e){
        std::cerr << "fatal _UTruckHandler error:\n" << e.what() << std::endl;
    } 
}

void BackendApp::_requestTruckHandler(const a2u::request_truck &msg, int seq){
    try{
        DBWrapper db;
        int truck = db.getVacantTruck();
        if(truck == -1){
            // ?????
            truck = 0;
        }
        db.updateTruckStatus(truck, 1);
        // insert package info
        msg.has_ups_account_id() ? 
        db.insertPackage(msg.shipid(), msg.whnum(), msg.location_x(), msg.location_y(), msg.item_desc(), truck, msg.ups_account_id()) :
        db.insertPackage(msg.shipid(), msg.whnum(), msg.location_x(), msg.location_y(), msg.item_desc(), truck);
        std::cout << msg.shipid() << " " << msg.whnum() << " " << msg.location_x() << " " << msg.location_y() << " " << msg.item_desc() << " truck: " << truck << std::endl;
        // send pickup to world
        UCommands ucmd;
        auto pickup = ucmd.add_pickups();
        pickup->set_truckid(truck);
        pickup->set_whid(msg.whnum());
        pickup->set_seqnum(seq);
        // std::cout << "pickup: truckid = " << ucmd.pickups()[0].truckid() << " whid = " << ucmd.pickups()[0].whid() << " seq = " << ucmd.pickups()[0].seqnum() << std::endl;
        sendToWorld(ucmd, seq);
        std::cout << "request truck handled" << std::endl;
    } catch(std::exception &e){
        std::cerr << "fatal _requestTruckHandler error:\n" << e.what() << std::endl;
    } 
}

void BackendApp::_readyForDeliveryHandler(const a2u::ready_for_delivery &msg, int seq){
    try{
        DBWrapper db;
        int shipid = msg.shipid(), tid = db.getPackageTruck(shipid);
        auto dest = db.getPackageDest(shipid);
        std::cout << "package # " << shipid << " ready for delivery" << std::endl;
        UCommands ucmd = MakeGoDelivery(seq, tid, shipid, dest.first, dest.second);
        db.updateTruckStatus(tid, 4);
        db.updatePackageStatus(shipid, 2);
        u2a::UToA u2a;
        auto ds = u2a.mutable_deliverstarted();
        ds->set_shipid(shipid);
        ds->set_status(true);
        sendToAmazon(u2a);
        sendToWorld(ucmd, seq);
        std::cout << "ready for delivery handled" << std::endl;
    } catch(std::exception &e){
        std::cerr << "fatal _readyForDeliveryHandler error:\n" << e.what() << std::endl;
    } 
}

void BackendApp::_changeDestinationHandler(const a2u::change_destination &msg, int seq){
    try{
        DBWrapper db;
        int shipid = msg.shipid(), x = msg.location_x(), y = msg.location_y();
        int tid = db.getPackageTruck(shipid);
        UCommands ucmd = MakeGoDelivery(seq, tid, shipid, x, y);
        db.updatePackageDest(shipid, x, y);
        sendToWorld(ucmd, seq);
        u2a::UToA u2a;
        auto ds = u2a.mutable_deliverstarted();
        ds->set_shipid(shipid);
        ds->set_status(true);
        sendToAmazon(u2a);
        std::cout << "change destination handled" << std::endl;
    } catch(std::exception &e){
        std::cerr << "fatal _changeDestinationHandler error:\n" << e.what() << std::endl;
    } 
}

void BackendApp::FrontMsgHandler(int fd){
    try{
        char buffer[512];
        recv(fd, buffer, 512, 0);
        std::string message = std::string(buffer);
        std::cout<<"received from front-end"<<message<<std::endl;
        size_t new_line = message.find_first_of("\n", 0);
        size_t second_line = message.find_first_of("\n", new_line+1);
        std::string type = message.substr(0, new_line);
        std::stringstream ss;
        ss<<message.substr(new_line+1, second_line-new_line-1);
        if(type=="query"){
            std::string id;
            ss>>id;
            UCommands ucmd;
            std::cout << "query: " << id << std::endl;
            auto query = ucmd.add_queries();
            DBWrapper db;
            int shipid = stoi(id), tid;
            try{
                tid = db.getPackageTruck(shipid);
            } catch(std::runtime_error &e){
                std::string response = "error: package id <" + id +  "> does not exit.";
                send(fd, response.c_str(), response.size()+1, MSG_WAITALL);
            }
            query->set_truckid(tid);
            int seq = getNxtSeq();
            query->set_seqnum(seq);
            std::string response = "success";
            send(fd, response.c_str(), 8, MSG_WAITALL);
            sendToWorld(ucmd, seq);
            std::cout << "query: " << id << " closed" << std::endl;
        }else if(type=="redirect"){
            std::string packageId, newAddress_x, new_Address_y;
            ss>>packageId>>newAddress_x>>new_Address_y;
            DBWrapper db;
            int shipid = stoi(packageId), tid;
            try{
                tid = db.getPackageTruck(shipid);
            } catch(std::runtime_error &e){
                std::string response = "error: package id <" + packageId +  "> does not exit.";
                send(fd, response.c_str(), response.size()+1, MSG_WAITALL);
            }
            int seq = getNxtSeq(), x = stoi(newAddress_x), y = stoi(new_Address_y);
            UCommands ucmd = MakeGoDelivery(seq, tid, shipid, x, y);
            // db.updateTruckStatus(tid, 4);
            // db.updatePackageStatus(shipid, 2);
            db.updatePackageDest(shipid, x, y);
            std::string response = "success";
            send(fd, response.c_str(), 8, MSG_WAITALL);
            sendToWorld(ucmd, seq);
            std::cout << "redirect: " << packageId << ", " << newAddress_x << ", " << new_Address_y << " handled." << std::endl;
        }
    } catch(std::exception &e){
        std::cerr << "fatal FrontMsgHandler error:\n" << e.what() << std::endl;
    } 
}


#if !CONFIG_HOST
    BackendApp::BackendApp(){
#else
    BackendApp::BackendApp(const char *wa, const char *wp, const char *aa, const char *ap): world_address(wa), world_port(wp), amazon_address(aa), amazon_port(ap){
#endif
    truck_status = {
        "IDLE", 
        "TRAVELING", // when receives pickups requests and is on its way to warehouse
        "ARRIVE WAREHOUSE", 
        "LOADING", // loading package, after loading package finish, go back to “arrive warehouse” status
        "DELIVERING", //when finished all deliver job, go to status “idle”
    };
    for(int i=0; i<truck_status.size(); i++){
        status_number[truck_status[i]] = i;
    }
    world_fd = _connect(world_address, world_port);
    DBWrapper db;
    UConnect msg;
    // set trucks, insert to db
    msg.set_isamazon(false);
    for(int i=0; i<TRUCK_NUM; i++){
        auto tmp = msg.add_trucks();
        tmp->set_id(i);
        tmp->set_x(0);
        tmp->set_y(0);
#if IS_DB_INIT
        db.insertTruck(i, 0, 0, 0);
#endif
    }
#if SINGLE_FRONT_END_SOCKET
    front_server.serverBuild();
    client_fd = front_server.as_server();
    std::cout << "backend as a server" << std::endl;
#endif
    // init world
    sendMesgTo(msg, world_fd);
    UConnected response;
    recvMesgFrom(response, world_fd);
    while(response.result() != "connected!"){
        std::cout << "uconnect failed: " << response.result() << std::endl;
        recvMesgFrom(response, world_fd);
    }
    world_id = response.worldid();
    std::cout << "world id = " << world_id << std::endl;
#if UNIT_TEST == 0
    amazon_fd = _connect(amazon_address, amazon_port);
    // send world id to amazon
    u2a::UToA u2a;
    auto wi = u2a.mutable_worldinfo();
    wi->set_worldid(world_id);
    sendToAmazon(u2a);
    // wait for world id response
    a2u::AToU a2u;
    recvMesgFrom(a2u, amazon_fd);
    while(!(a2u.has_worldinfo() 
    && a2u.worldinfo().result() 
    && a2u.worldinfo().worldid() == world_id)){
        std::cout << a2u.worldinfo().result() << std::endl;
        std::cout << a2u.worldinfo().worldid() << std::endl;
        std::cout << "sync with amazon failed: " << std::endl;
        sendToAmazon(u2a);
    }
    std::cout << "sync success" << std::endl;
#endif
}