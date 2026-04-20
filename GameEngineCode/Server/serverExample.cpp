//#include "server.hpp"
//
//#define MAX_PLAYERS 4
//#define MAX_PLATFORMS 4
//#define CONNECTING "Connecting"
//#define EVENT_SOCKET_ADDRESS "tcp://localhost:5561"
//
//const std::string playerSocketBases[MAX_PLAYERS] = { "tcp://localhost:5557", "tcp://localhost:5558", "tcp://localhost:5559", "tcp://localhost:5560" };
//std::thread playerThreads[MAX_PLAYERS];
//bool threadAvailable[MAX_PLAYERS] = { true, true, true, true };
//std::thread platformThreads[2];
//
//
////for now, all shapes need to be constructed
//ShapeInfo nonPlayerShapes[MAX_PLATFORMS] = { ShapeInfo(), ShapeInfo(),ShapeInfo() };
//ShapeInfo playerShapes[MAX_PLAYERS] = { ShapeInfo(), ShapeInfo(),ShapeInfo(), ShapeInfo() };
//
////std::mutex m;
//
//zmq::context_t context;
//// REP (reply) socket for receiving client position updates
//zmq::socket_t reply;
//// PUB (publisher) socket for sending information out to all clients
//zmq::socket_t publisher;
//zmq::socket_t clientSockets[4];
//zmq::socket_t eventSocket;
//int clientNum;
//size_t size;
//
//Coord* spawnLocation;
//
//
//int64_t eventHandlerUUID;
//EventHandler eventHandler;
//
//int getAvailableSocket() {
//    for (int i = 0; i < MAX_PLAYERS; i++) {
//        if (threadAvailable[i]) {
//            return i;
//        }
//    }
//    return -1;
//}
//
//void handleSpawnEvent(EventType eventType, int64_t sourceUUID, EventParams parameters) {
//    if (eventType == SpawnEvent) {
//        eventSocket.send(zmq::buffer(spawnLocation, sizeof(Coord)), zmq::send_flags::none);
//    }
//}
//
//void initEventHandler() {
//    eventHandlerUUID = getNewUUID();
//    addComponent(eventHandlerUUID, EVENTHANDLER);
//    propertyMap.at(EVENTHANDLER).at(eventHandlerUUID) = EventHandler();
//    eventHandler = std::get<EventHandler>(propertyMap.at(EVENTHANDLER).at(eventHandlerUUID));
//    eventHandler.eventHandleMethod = handleSpawnEvent;
//    std::list<EventType> list = std::list<EventType>();
//    list.push_back(SpawnEvent);
//    eventManager->registerEvent(list, &eventHandler);
//}
//
//
//////////////////////////////Threads
//void individualClientThread(int socketID, zmq::socket_t* clientSocket) {
//    clientSocket->bind(playerSocketBases[socketID]);
//    //std::cout << "binding successful" << std::endl;
//    zmq::message_t firstMessage;
//    clientSocket->recv(firstMessage, zmq::recv_flags::none);
//    //std::cout << "first message received" << std::endl;
//    playerShapes[socketID] = *((ShapeInfo*)firstMessage.data());
//    clientSocket->send(zmq::str_buffer("recieved first message"), zmq::send_flags::none);
//    //std::cout << "first reply sent" << std::endl;
//    while (!threadAvailable[socketID]) {
//        zmq::message_t message;
//        if (clientSocket->recv(message, zmq::recv_flags::dontwait)) {
//            //std::cout << "Got message from client" << std::endl;
//            playerShapes[socketID].pos = ((ShapeInfo*)message.data())->pos;
//            clientSocket->send(zmq::str_buffer("recieved message"), zmq::send_flags::none);
//        }
//    }
//}
//
//void firstPlatformThread() {
//    m.lock();
//    //TimeManager *pTManager = new TimeManager(serverTimeManager, 1);
//    TimeManager* pTManager = new TimeManager(timeManager, 10000000);
//    int64_t currentTime = 0;
//    int64_t elapsedTime = 0;
//    int64_t lastTime = pTManager->getTime();
//    float oscillationTimer = 0.f;
//    float originalposY1 = nonPlayerShapes[0].pos.y;
//    m.unlock();
//    while (true) {
//        //m.lock();
//        currentTime = pTManager->getTime();
//
//        elapsedTime = currentTime - lastTime; //time since last frame
//        pTManager->deltaTime = static_cast<double>(elapsedTime);
//        //m.unlock();
//
//
//        lastTime = currentTime;
//
//        oscillationTimer += pTManager->deltaTime / 100.;
//        //std::cout << "p1 thread oscillationTimer: " << oscillationTimer << std::endl;
//        nonPlayerShapes[0].pos.y = originalposY1 + sinf(oscillationTimer * 2.f) * 100.f;
//
//    }
//
//}
//
//void secondPlatformThread() {
//    m.lock();
//    //TimeManager *pTManager = new TimeManager(serverTimeManager, 1);
//    TimeManager* pTManager = new TimeManager(timeManager, 10000000);
//    int64_t currentTime = 0;
//    int64_t elapsedTime = 0;
//    int64_t lastTime = pTManager->getTime();
//    m.unlock();
//    float oscillationTimer = 0.f;
//    float originalposX2 = nonPlayerShapes[1].pos.x;
//
//    while (true) {
//        //m.lock();
//        currentTime = pTManager->getTime();
//        //m.unlock();
//        elapsedTime = currentTime - lastTime; //time since last frame
//        pTManager->deltaTime = static_cast<double>(elapsedTime);
//        
//
//        lastTime = currentTime;
//
//        oscillationTimer += pTManager->deltaTime / 100.;
//        
//        nonPlayerShapes[1].pos.x = originalposX2 + cosf(oscillationTimer * 2.f) * 150.f;
//
//    }
//}
/////////////////////////////////////////////////////////////////////////////////////////////
//void onServerStart()
//{
//    context = zmq::context_t(1);
//    reply  = zmq::socket_t(context, zmq::socket_type::rep);
//    publisher = zmq::socket_t(context, zmq::socket_type::pub);
//    eventSocket = zmq::socket_t(context, zmq::socket_type::rep);
//    for (int i = 0; i < MAX_PLAYERS; i++) {
//        clientSockets[i] = zmq::socket_t(context, zmq::socket_type::rep);
//    }
//    publisher.bind("tcp://*:5555");
//    reply.bind("tcp://*:5556");
//    eventSocket.bind("tcp://*:5561");
//    spawnLocation = new Coord(50, 385);
//    initEventHandler();
//    ShapeInfo oscillatingShapeInfo = ShapeInfo(Coord(200, 250), 150, 150, 255, 0, 0);
//    ShapeInfo oscillatingShape2Info = ShapeInfo(Coord(600, 200), 300, 50, 0, 255, 0);
//    ShapeInfo staticShapeInfo = ShapeInfo(Coord(500, 400), 300, 50, 0, 0, 255);
//    ShapeInfo deathZoneShapeInfo = ShapeInfo(Coord(1100, 600), 200, 100, true);
//    nonPlayerShapes[0] = oscillatingShapeInfo;
//    nonPlayerShapes[1] = oscillatingShape2Info;
//    nonPlayerShapes[2] = staticShapeInfo;
//    nonPlayerShapes[3] = deathZoneShapeInfo;
//    clientNum = 0;
//    size = 1;
//    platformThreads[0] = std::thread(firstPlatformThread);
//    platformThreads[1] = std::thread(secondPlatformThread);
//    std::cout << "Client 0 available: " << threadAvailable[0] << " | Client 1 available: " << threadAvailable[1] << " | Client 2 available: " << threadAvailable[2] << " | Client 3 available: " << threadAvailable[3] << std::endl;
//}
//
//void onServerUpdate()
//{
//
//    std::vector<zmq::message_t> recv_msgs;
//    zmq::recv_result_t result = zmq::recv_multipart(reply, std::back_inserter(recv_msgs), zmq::recv_flags::dontwait);
//    
//    if (result && *result == 2) {
//        //std::cout << "Successful" << std::endl;
//        if (*((bool *)recv_msgs[0].data())) {
//            //std::cout << "read bool success" << std::endl;
//            if (clientNum < MAX_PLAYERS) {
//                int availableSocket = getAvailableSocket();
//                if (availableSocket < 0) {
//                    std::cout << "No Sockets Available" << std::endl;
//                    exit(1);
//                }
//                threadAvailable[availableSocket] = false;
//                clientNum++;
//               
//                playerShapes[availableSocket] = *((ShapeInfo*)recv_msgs[1].data());
//                
//                m.lock();
//                reply.send(zmq::buffer(&size, sizeof(size_t)), zmq::send_flags::sndmore);
//                m.unlock();
//                reply.send(zmq::buffer(nonPlayerShapes, sizeof(ShapeInfo) * MAX_PLATFORMS), zmq::send_flags::sndmore);
//                reply.send(zmq::buffer(&clientNum, sizeof(int)), zmq::send_flags::sndmore);
//                reply.send(zmq::buffer(playerShapes, sizeof(ShapeInfo) * MAX_PLAYERS), zmq::send_flags::sndmore);
//                //reply.send(zmq::buffer(playerSocketBases[clientNum - 1]), zmq::send_flags::sndmore);
//                reply.send(zmq::buffer(playerSocketBases[availableSocket]), zmq::send_flags::sndmore);
//                reply.send(zmq::buffer(&availableSocket, sizeof(int)), zmq::send_flags::sndmore); //added another send message
//                reply.send(zmq::buffer(threadAvailable, sizeof(bool) * MAX_PLAYERS), zmq::send_flags::none); //message 2
//     
//               
//                playerThreads[availableSocket] = std::thread(individualClientThread, availableSocket, &clientSockets[availableSocket]);
//     
//            }
//            else {
//                reply.send(zmq::str_buffer("rejected"), zmq::send_flags::none);
//            }
//            //std::cout << "end of join loop" << std::endl;
//        }
//        else {
//            clientNum--;
//            if (clientNum < 0) exit(1);
//            int socketID = *((int*)recv_msgs[1].data());
//            reply.send(zmq::str_buffer("Done"), zmq::send_flags::none);
//            threadAvailable[socketID] = true;
//            playerShapes[socketID] = ShapeInfo();
//            playerThreads[socketID].join();
//            clientSockets[socketID].unbind(playerSocketBases[socketID]);
//     
//        }
//        std::cout << "Client 0 available: " << threadAvailable[0] << " | Client 1 available: " << threadAvailable[1] << " | Client 2 available: " << threadAvailable[2] << " | Client 3 available: " << threadAvailable[3] << std::endl;
//    }
//    zmq::message_t eventMessage;
//    if (eventSocket.recv(eventMessage, zmq::recv_flags::dontwait)) {
//        eventManager->raise(*((Event*)eventMessage.data()));
//    }
//    //send message to clients
//    //publisher.send(zmq::str_buffer("Movement"), zmq::send_flags::sndmore);
//    //publisher.send(zmq::buffer(&oscillatingShapeInfo, sizeof(ShapeInfo)), zmq::send_flags::none);
//
//    publisher.send(zmq::str_buffer("Movement"), zmq::send_flags::sndmore);
//    m.lock();
//    if (clientNum > 0) {
//        publisher.send(zmq::buffer(&size, sizeof(size_t)), zmq::send_flags::sndmore);
//        publisher.send(zmq::buffer(nonPlayerShapes, sizeof(ShapeInfo) * MAX_PLATFORMS), zmq::send_flags::sndmore);
//        publisher.send(zmq::buffer(&clientNum, sizeof(int)), zmq::send_flags::sndmore);
//        publisher.send(zmq::buffer(playerShapes, sizeof(ShapeInfo) * MAX_PLAYERS), zmq::send_flags::sndmore);
//        publisher.send(zmq::buffer(threadAvailable, sizeof(bool) * MAX_PLAYERS), zmq::send_flags::none);// added extra message, make sure that gameExample message count is increased
//    }
//    m.unlock();
//}
//
//void onServerEnd()
//{
//    for (int i = 0; i < clientNum; i++) {
//        playerThreads[i].join();
//    }
//    for (int i = 0; i < 2; i++) {
//        platformThreads[i].join();
//    }
//    exit(0);
//}
