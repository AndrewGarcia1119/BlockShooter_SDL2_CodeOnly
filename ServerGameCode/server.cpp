#include "server.hpp"

#define MAX_PLAYERS 1
#define CONNECTING "Connecting"

const std::string playerSocketBases[MAX_PLAYERS] = { "tcp://localhost:5557" };
std::thread playerThreads[MAX_PLAYERS];
bool threadAvailable[MAX_PLAYERS] = { true };
std::thread platformThreads[2];


//std::mutex m;

zmq::context_t context;
// REP (reply) socket for receiving client position updates
zmq::socket_t reply;
// PUB (publisher) socket for sending information out to all clients
//zmq::socket_t publisher;
zmq::socket_t clientSockets[MAX_PLAYERS];
//zmq::socket_t eventSocket;
int clientNum;


int numBlocks = 60;

int getAvailableSocket() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (threadAvailable[i]) {
            return i;
        }
    }
    return -1;
}


////////////////////////////Threads
void individualClientThread(int socketID, zmq::socket_t* clientSocket) {
    clientSocket->bind(playerSocketBases[socketID]);
    while (!threadAvailable[socketID]) {
        zmq::message_t message;
        if (clientSocket->recv(message, zmq::recv_flags::dontwait)) {
            //std::cout << "Got message from client" << std::endl;
            clientSocket->send(zmq::buffer(&numBlocks, sizeof(int)), zmq::send_flags::none);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////
void onServerStart()
{
    context = zmq::context_t(1);
    reply = zmq::socket_t(context, zmq::socket_type::rep);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        clientSockets[i] = zmq::socket_t(context, zmq::socket_type::rep);
    }
    reply.bind("tcp://*:5556");
    std::cout << "Client available: " << threadAvailable[0] << std::endl;
}

void onServerUpdate()
{

    /*std::vector<zmq::message_t> recv_msgs;
    zmq::recv_result_t result = zmq::recv_multipart(reply, std::back_inserter(recv_msgs), zmq::recv_flags::dontwait);*/
    zmq::message_t msg;
    if (reply.recv(msg, zmq::recv_flags::dontwait)) {
        //std::cout << "Successful" << std::endl;
        if (*((bool*)msg.data())) {
            //std::cout << "read bool success" << std::endl;
            if (clientNum < MAX_PLAYERS) {
                int availableSocket = getAvailableSocket();
                if (availableSocket < 0) {
                    std::cout << "No Sockets Available" << std::endl;
                    exit(1);
                }
                threadAvailable[availableSocket] = false;
                clientNum++;

                m.lock();
                reply.send(zmq::buffer(&numBlocks, sizeof(int)), zmq::send_flags::none);
                m.unlock();
                playerThreads[availableSocket] = std::thread(individualClientThread, availableSocket, &clientSockets[availableSocket]);

            }
            else {
                reply.send(zmq::str_buffer("rejected"), zmq::send_flags::none);
            }
            //std::cout << "end of join loop" << std::endl;
        }
        else {
            clientNum--;
            if (clientNum < 0) exit(1);
            int socketID = 0;
            reply.send(zmq::str_buffer("Done"), zmq::send_flags::none);
            threadAvailable[socketID] = true;
            playerThreads[socketID].join();
            clientSockets[socketID].unbind(playerSocketBases[socketID]);

        }
        std::cout << "Client available: " << threadAvailable[0] <<  std::endl;
    }
}

void onServerEnd()
{
    for (int i = 0; i < clientNum; i++) {
        playerThreads[i].join();
    }
    for (int i = 0; i < 2; i++) {
        platformThreads[i].join();
    }
    exit(0);
}
