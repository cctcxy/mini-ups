#ifndef backendapp_h
#define backendapp_h

#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h> 
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <unordered_set>
#include <thread>
#include <mutex>

#include "world_ups.pb.h"
#include "u2a.pb.h"
#include "a2u.pb.h"
#include "db.h"
#include "util.h"
#include "server.h"

#if !CONFIG_HOST
static const char *world_address = "vcm-18232.vm.duke.edu", *world_port = "12345", *amazon_address = "vcm-18232.vm.duke.edu", *amazon_port = "22222";
// static const char *world_address = "host.docker.internal", *world_port = "12345", *amazon_address = "vcm-18232.vm.duke.edu", *amazon_port = "22222";
#endif
static const int TRUCK_NUM = 64;

class BackendApp{
private:
    int world_fd, amazon_fd, socket_fd;
    //socket_fd as the server of front_end
    int world_id;
    std::vector<std::string> truck_status;
    std::unordered_map<std::string, int> status_number;
    // these hash sets are not thread safe
    std::unordered_set<int> world_seq_recv, world_seq_send;
    int seq2world = 0;
    std::mutex seq2world_lock;
#if CONFIG_HOST
    const char *world_address, *world_port, *amazon_address, *amazon_port;
#endif
#if SINGLE_FRONT_END_SOCKET
    server front_server;
    int client_fd;
#endif

#if UNIT_TEST
public:
#endif
    inline int getNxtSeq(){
        std::lock_guard<std::mutex> lock(seq2world_lock);
        int ret = seq2world++;
        return ret;
    }

    inline void worldMsgHandler(){
        UResponses response;
        recvMesgFrom(response, world_fd);
        for(auto &i:response.completions()){
            UFinishedHandler(i);
        }
        for(auto &i:response.delivered()){
            UDeliveryMadeHandler(i);
        }
        for(auto &i:response.truckstatus()){
            UTruckHandler(i);
        }
        for(auto &i:response.acks()){
            std::cout << "recevied ack: " << i << std::endl;
            world_seq_send.insert(i);
        }
        for(auto &i:response.error()){
            int seq = i.seqnum();
            acknowledge(seq);
            if(world_seq_recv.count(seq)) return;
            world_seq_recv.insert(seq);
            std::cerr << "error: " << i.err() << ", originated from seq " << i.originseqnum() << std::endl;
        }
        if(response.has_finished() && response.finished()){
            std::cout << "world send finished = true" << std::endl;
            world_fd = _connect(world_address, world_port);
            UConnect msg;
            msg.set_worldid(world_id);
            msg.set_isamazon(false);
            sendMesgTo(msg, world_fd);
            UConnected response;
            recvMesgFrom(response, world_fd);
            while(response.result() != "connected!"){
                std::cout << "uconnect failed: " << response.result() << std::endl;
                recvMesgFrom(response, world_fd);
            }
        }
    }

    inline void UFinishedHandler(const UFinished &msg){
        int seq = msg.seqnum();
        acknowledge(seq);
        if(world_seq_recv.count(seq)) return;
        world_seq_recv.insert(seq);
        std::thread(&BackendApp::_UFinishedHandler, this, msg).detach();
    }

    inline void UDeliveryMadeHandler(const UDeliveryMade &msg){
        int seq = msg.seqnum();
        acknowledge(seq);
        if(world_seq_recv.count(seq)) return;
        world_seq_recv.insert(seq);
        std::thread(&BackendApp::_UDeliveryMadeHandler, this, msg).detach();
    }

    inline void UTruckHandler(const UTruck &msg){
        int seq = msg.seqnum();
        acknowledge(seq);
        if(world_seq_recv.count(seq)) return;
        world_seq_recv.insert(seq);
        std::thread(&BackendApp::_UTruckHandler, this, msg).detach();
    }

    inline void amazonMsghandler(){
        a2u::AToU response;
        recvMesgFrom(response, amazon_fd);
        if(response.has_request()){
            requestTruckHandler(response.request());
        }
        if(response.has_readyfordelivery()){
            readyForDeliveryHandler(response.readyfordelivery());
        }
        if(response.has_changedest()){
            changeDestinationHandler(response.changedest());
        }
    }

    inline void requestTruckHandler(const a2u::request_truck &msg){
        std::thread(&BackendApp::_requestTruckHandler, this, msg, getNxtSeq()).detach();
    }

    inline void readyForDeliveryHandler(const a2u::ready_for_delivery &msg){
        std::thread(&BackendApp::_readyForDeliveryHandler, this, msg, getNxtSeq()).detach();
    }

    inline void changeDestinationHandler(const a2u::change_destination &msg){   
        std::thread(&BackendApp::_changeDestinationHandler, this, msg, getNxtSeq()).detach(); 
    }

    template<typename T>
    inline void sendToAmazon(const T &message){
#if !UNIT_TEST
        sendMesgTo(message, amazon_fd);
#endif
    }

    template<typename T>
    inline void sendToWorld(const T &message, int seq){
        safeSend(message, world_fd, world_seq_send, seq);
    }

    inline UCommands MakeGoDelivery(int seq, int tid, int shipid, int x, int y){
        UCommands ucmd;
        auto ugd = ucmd.add_deliveries();
        ugd->set_seqnum(seq);
        ugd->set_truckid(tid);
        auto loc = ugd->add_packages();
        loc->set_packageid(shipid);
        loc->set_x(x);
        loc->set_y(y);
        return ucmd;
    }

    inline void acknowledge(int num, int fd = -1){
        if(fd == -1) fd = world_fd;
        UCommands msg;
        msg.add_acks(num);
        sendMesgTo(msg, fd);
    }

    inline void listenFront(){
        while(1){
            server s;
            s.serverBuild();
            FrontMsgHandler(s.as_server());
        }
    }

    inline void setSimSpeed(int speed){
        std::thread(&BackendApp::_setSimSpeed, this, speed).detach();
    }

    inline void _setSimSpeed(int speed){
        int seq = getNxtSeq();
        UCommands ucmd;
        auto q = ucmd.add_queries();
        q->set_truckid(0);
        q->set_seqnum(seq);
        ucmd.set_simspeed(speed);
        sendToWorld(ucmd, seq);
    }

    int _connect(const char *, const char *);
    void _UFinishedHandler(const UFinished &);
    void _UDeliveryMadeHandler(const UDeliveryMade &);
    void _UTruckHandler(const UTruck &);
    void _requestTruckHandler(const a2u::request_truck &, int);
    void _readyForDeliveryHandler(const a2u::ready_for_delivery &msg, int seq);
    void _changeDestinationHandler(const a2u::change_destination &msg, int seq);
    void FrontMsgHandler(int fd);

public:
#if !CONFIG_HOST
    BackendApp();
#else
    BackendApp(const char *wa, const char *wp, const char *aa, const char *ap);
#endif

    ~BackendApp(){
        close(world_fd);
#if !UNIT_TEST
        // close(amazon_fd);
#endif
    }

    inline void listen(){
        fd_set allset, rset;
        std::vector<int> all_fd = {world_fd, amazon_fd};
        FD_ZERO(&allset);
        FD_SET(world_fd, &allset);
#if SINGLE_FRONT_END_SOCKET
        all_fd = {world_fd, amazon_fd, client_fd};
        FD_SET(client_fd, &allset);
#else
        std::thread(&BackendApp::listenFront, this).detach();
#endif
        int maxfd = *std::max_element(all_fd.begin(), all_fd.end());
#if UNIT_TEST == 0
        FD_SET(amazon_fd, &allset);
#endif
        while(1){ 
            rset = allset;
            if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
                throw std::runtime_error(strerror(errno));
            }
            if(FD_ISSET(world_fd, &rset)){ 
                worldMsgHandler();
            } else if(FD_ISSET(amazon_fd, &rset)){
                amazonMsghandler();
#if SINGLE_FRONT_END_SOCKET
            } else if(FD_ISSET(client_fd, &rset)){
                FrontMsgHandler();
#endif
            }
        }
    }
};

#endif
