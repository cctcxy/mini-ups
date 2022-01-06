#ifndef util_h
#define util_h

#include <climits>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_set>
#include <shared_mutex>
#include <thread>

#include <unistd.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#define CONFIG_HOST 1
#define UNIT_TEST 0
#define SINGLE_FRONT_END_SOCKET 0
#define IS_DB_INIT 1
#define RESEND_TIMEOUT (1000000)

template<typename T>
inline void safeSend(const T &message, int fd, const std::unordered_set<int> &st, int seq){
    do{
        sendMesgTo(message, fd);
        usleep(RESEND_TIMEOUT);        
    } while(!st.count(seq));
}

//this is adpated from code that a Google engineer posted online 
template<typename T>
bool sendMesgTo(const T &message, int fd){
    google::protobuf::io::FileOutputStream out(fd);
    {   //extra scope: make output go away before out->Flush()
        // We create a new coded stream for each message.
        // Don’t worry, this is fast. 
        google::protobuf::io::CodedOutputStream output(&out); // Write the size.
        const int size = message.ByteSizeLong();
        output.WriteVarint32(size);
        uint8_t *buffer = output.GetDirectBufferForNBytesAndAdvance(size);
        if(buffer != NULL){
            // Optimization: The message fits in one buffer, so use the faster // direct-to-array serialization path. 
            message.SerializeWithCachedSizesToArray(buffer);
        } else{
            // Slightly-slower path when the message is multiple buffers. 
            message.SerializeWithCachedSizes(&output);
            if(output.HadError()){
                return false;
            }
        }
    }
    out.Flush();
    return true;
}

//this is adpated from code that a Google engineer posted online 
template<typename T>
bool recvMesgFrom(T &message, int fd){
    google::protobuf::io::FileInputStream in(fd);
    google::protobuf::io::CodedInputStream input(&in);
    uint32_t size;
    if(!input.ReadVarint32(&size)){
        return false;
    }
    // Tell the stream not to read beyond that size. 
    google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
    // Parse the message.
    if(!message.MergeFromCodedStream(&input)){
        return false;
    }
    if(!input.ConsumedEntireMessage()){
        return false;
    }
    // Release the limit. 
    input.PopLimit(limit); 
    return true;
}

#endif