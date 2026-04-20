//#include "game.hpp"
//#include "draw.hpp"
//#include "defs.hpp"
//#include "input.hpp"
//#include "properties.hpp"
//#include <iostream>
//
//#define MAX_PLAYERS 4
//#define IMMUNITY_TIME 0.5f
//#define CONNECTING "Connecting"
//#define EVENT_SOCKET_ADDRESS "tcp://localhost:5561"
//
//
//float oscillationTimer;
//int64_t lastTime;
//bool serverSlotAvailable[MAX_PLAYERS] = {true, true, true, true};
//int64_t otherPlayers[MAX_PLAYERS];
//int64_t controllable;
//int64_t ground;
//int64_t oscilShape;
//int64_t pattShape;
//int64_t oscilShape2;
//int64_t statShape;
//int64_t deathZone;
//int64_t rightSideBoundary;
//int64_t leftSideBoundary;
//int64_t referenceShape;
//float immunityTime;
//
////int64_t spawnZone;
//
//ShapeInfo *csInfo;
//std::thread client;
//std::mutex m;
//
//zmq::context_t context{ 1 };
//// REP (reply) socket for sending client info upon connection
//zmq::socket_t connector{ context, zmq::socket_type::req };
//
//// PUB (publisher) socket for receiving global information from server
//zmq::socket_t subscriber{ context, zmq::socket_type::sub };
//
//zmq::socket_t playerSocket{ context, zmq::socket_type::req };
//
//zmq::socket_t eventSocket{ context, zmq::socket_type::req };
//
//int clientNumber = -1;
//int playerArrayIndex = -1;
//
//bool messageSent = false;
//bool newShapeAdded = false;
//bool inServer = false;
//
//
//std::string connectedAddress;
//bool connect = false;
//
//
////this variable is used to help determine if a button was just pressed
//bool justPressed = true;
//
///** Checks for key presses and updates object position, called every deltaT
//*/
//void controlPlayer(int64_t UUID, int64_t deltaT) {
//    auto& controlAttribute = propertyMap.at(CONTROL).at(UUID); //check for controllable property
//    if (!std::holds_alternative<ControlAttribute>(controlAttribute)) { //check if attribute is controllable attribute
//        //error, does not have controllable attribute
//        return;
//    }
//    auto& collisionAttribute = propertyMap.at(COLLISION).at(UUID); // get collision attribue to access collider
//    SDL_FRect collider = std::get<ColliderAttribute>(collisionAttribute).rectCollider; //get object collider
//
//    Coord* totalMovement = new Coord(); //holds amount that the object moved
//    if (isKeyPressed(SPACE_BAR) && justPressed) { //W - up
//        justPressed = false;
//        if (collider.y < 10) {
//            totalMovement->y -= collider.y;
//        }
//        else {
//            totalMovement->y -= (10.); // moves 10 pixels per second
//        }
//    }
//    else if (!isKeyPressed(SPACE_BAR)) {
//        justPressed = true;
//    }
//    if (isKeyPressed(A)) { //A - left
//        if (collider.x < 10) {
//            totalMovement->x -= collider.x;
//        }
//        else {
//            totalMovement->x -= (10.);
//        }
//    }
//    if (isKeyPressed(S)) { //S - down
//        if ((collider.y + collider.h + 10) > SCREEN_HEIGHT) {
//            totalMovement->y += (SCREEN_HEIGHT - (collider.y + collider.h));
//        }
//        else {
//            totalMovement->y += (10.);
//        }
//
//    }
//    if (isKeyPressed(D)) { //D - right
//        if ((collider.x + collider.w + 10) > SCREEN_WIDTH) {
//            totalMovement->x += (SCREEN_WIDTH - (collider.x + collider.w));
//        }
//        else {
//            totalMovement->x += (10.);
//        }
//    }
//
//    if (isKeyPressed(LEFT_CLICK)) {
//        Coord mousePos = getMousePos();
//        std::cout << "Current Mouse position - x: " << mousePos.x << ", y: " << mousePos.y << std::endl;
//    }
//
//    Coord posProp = std::get<Coord>(propertyMap.at(POSITION).at(UUID));
//    posProp.x += totalMovement->x * deltaT;
//    posProp.y += totalMovement->y * deltaT;
//
//    propertyMap.at(POSITION).at(UUID) = posProp;
//
//}
//
//
//
//
//static void setPos(int64_t UUID, Coord pos) {
//    pos.x += globalOffset->x;
//    propertyMap.at(POSITION).at(UUID) = pos;
//}
//
//
//
//static void createShape(int64_t UUID, Coord pos, float width, float height, int r, int g, int b) {
//    addComponent(UUID, POSITION);
//    //propertyMap.at(POSITION).at(UUID) = pos;
//    setPos(UUID, pos);
//    addComponent(UUID, RENDER);
//    ShapeAttribute gSA = ShapeAttribute(width, height, r, g, b);
//    propertyMap.at(RENDER).at(UUID) = gSA;
//    ColliderAttribute gCA = ColliderAttribute();
//    gCA.rectCollider.x = gSA.shape.x;
//    gCA.rectCollider.y = gSA.shape.y;
//    gCA.rectCollider.w = gSA.shape.w;
//    gCA.rectCollider.h = gSA.shape.h;
//    addComponent(UUID, COLLISION);
//    propertyMap.at(COLLISION).at(UUID) = gCA;
//}
//
//static void createDeathZone(int64_t UUID, Coord pos, float width, float height) {
//    addComponent(UUID, POSITION);
//    //propertyMap.at(POSITION).at(UUID) = pos;
//    setPos(UUID, pos);
//    addComponent(UUID, COLLISION);
//    ColliderAttribute gCA = ColliderAttribute();
//    gCA.rectCollider.x = pos.x;
//    gCA.rectCollider.y = pos.y;
//    gCA.rectCollider.w = width;
//    gCA.rectCollider.h = height;
//    addComponent(UUID, DEATH);
//    propertyMap.at(DEATH).at(UUID) = DeathAttribute(true);
//    propertyMap.at(COLLISION).at(UUID) = gCA;
//}
//
//static void createSideBoundary(int64_t UUID, Coord pos, float width, float height) {
//    addComponent(UUID, POSITION);
//    propertyMap.at(POSITION).at(UUID) = pos;
//    addComponent(UUID, COLLISION);
//    ColliderAttribute gCA = ColliderAttribute();
//    gCA.rectCollider.x = pos.x;
//    gCA.rectCollider.y = pos.y;
//    gCA.rectCollider.w = width;
//    gCA.rectCollider.h = height;
//    addComponent(UUID, SIDE);
//    propertyMap.at(SIDE).at(UUID) = SideAttribute(true);
//    propertyMap.at(COLLISION).at(UUID) = gCA;
//}
//
//
//void handleDeathEvent(EventType eventType, int64_t victimUUID, EventParams otherParams)
//{
//    if (eventType == DeathEvent) {
//        Event spawn = Event(SpawnEvent, victimUUID);
//        //send event to server
//        eventSocket.send(zmq::buffer(&spawn, sizeof(Event)), zmq::send_flags::none);
//
//        zmq::message_t reply;
//        eventSocket.recv(reply, zmq::recv_flags::none);
//        Coord pos = *((Coord*)reply.data());
//        setPos(victimUUID, pos);
//        HealthAttribute* h = std::get_if<HealthAttribute>(&propertyMap.at(HEALTH).at(victimUUID));
//        if (h) {
//            h->hitPoints--;
//            std::cout << "Health: " << h->hitPoints << std::endl;
//        }
//        //propertyMap.at(POSITION).at(victimUUID) = pos;
//
//    }
//}
//
//void testClientThread() {
//    connector.connect("tcp://localhost:5556");
//    subscriber.connect("tcp://localhost:5555");
//    subscriber.set(zmq::sockopt::subscribe, "Movement");
//    connect = true;
//    connector.send(zmq::buffer(&connect, sizeof(bool)), zmq::send_flags::sndmore);
//    connector.send(zmq::buffer(csInfo, sizeof(ShapeInfo)), zmq::send_flags::none);
//    std::vector<zmq::message_t> first_recv_msgs;
//    zmq::recv_result_t firstResult = zmq::recv_multipart(connector, std::back_inserter(first_recv_msgs));
//    if (!firstResult || (*firstResult != 7)) exit(1);
//
//    ShapeInfo si1 = ((ShapeInfo*)first_recv_msgs[1].data())[0];
//    playerArrayIndex = -1;
//
//    //the client number shows how many clients are since this client connected, including this one.
//    m.lock();
//    clientNumber = *((int*)first_recv_msgs[2].data());
//    playerArrayIndex = *((int*)first_recv_msgs[5].data());
//    for (int i = 0; i < MAX_PLAYERS; i++) {
//        serverSlotAvailable[i] = ((bool*)first_recv_msgs[6].data())[i];
//    }
//    m.unlock();
//    for (int i = 0; i < MAX_PLAYERS; i++) { //changed from player array index to max players
//
//        if (i == playerArrayIndex || serverSlotAvailable[i]) continue;
//
//        ShapeInfo s = ((ShapeInfo*)first_recv_msgs[3].data())[i];
//        otherPlayers[i] = getNewUUID();
//        createShape(otherPlayers[i], s.pos, s.w, s.h, s.r, s.g, s.b);
//    }
//    oscilShape = getNewUUID();
//    createShape(oscilShape, si1.pos, si1.w, si1.h, si1.r, si1.g, si1.b);
//
//    ShapeInfo si2 = ((ShapeInfo*)first_recv_msgs[1].data())[1];
//    oscilShape2 = getNewUUID();
//    createShape(oscilShape2, si2.pos, si2.w, si2.h, si2.r, si2.g, si2.b);
//
//    ShapeInfo si3 = ((ShapeInfo*)first_recv_msgs[1].data())[2];
//    statShape = getNewUUID();
//    createShape(statShape, si3.pos, si3.w, si3.h, si3.r, si3.g, si3.b);
//
//    ShapeInfo si4 = ((ShapeInfo*)first_recv_msgs[1].data())[3];
//    if (si4.deathZone) {
//        deathZone = getNewUUID();
//        createDeathZone(deathZone, si4.pos, si4.w, si4.h);
//    }
//    
//    /*m.unlock();*/
//    playerSocket.connect((char*)first_recv_msgs[4].data());
//    connectedAddress = (char*)first_recv_msgs[4].data();
//    inServer = true;
//    while (gameRunning) {
//        zmq::message_t message;
//        //if (subscriber.recv(message, zmq::recv_flags::dontwait)) {
//        std::vector<zmq::message_t> recv_msgs;
//        zmq::recv_result_t result = zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
//        
//        if (result && (*result == 6)) {
//            ShapeInfo si = ((ShapeInfo*)recv_msgs[2].data())[0];
//           /* m.lock();*/
//                      
//            setPos(oscilShape, si.pos);
//
//
//            ShapeInfo si_2 = ((ShapeInfo*)recv_msgs[2].data())[1];
//            setPos(oscilShape2, si_2.pos);            
//
//        
//            if (clientNumber != *((int*)recv_msgs[3].data())) {
//                for (int i = 0; i < MAX_PLAYERS; i++) {
//                    serverSlotAvailable[i] = ((bool*)recv_msgs[5].data())[i];
//                }
//                //std::cout << "Client 0 available: " << serverSlotAvailable[0] << " | Client 1 available: " << serverSlotAvailable[1] << " | Client 2 available: " << serverSlotAvailable[2] << " | Client 3 available: " << serverSlotAvailable[3] << std::endl;
//                for (int i = 0; i < MAX_PLAYERS; i++) {
//                    /////// may be game breaking code here
//                    if (i == playerArrayIndex) continue;
//                    if (serverSlotAvailable[i]) {
//                        if (otherPlayers[i] != 0) {
//                            destroy(&otherPlayers[i]);
//                        }
//                    }
//                    
//                    if (otherPlayers[i] == 0 && !serverSlotAvailable[i]) {
//                        ShapeInfo ns = ((ShapeInfo*)recv_msgs[3].data())[i];
//                        otherPlayers[i] = getNewUUID();
//                        createShape(otherPlayers[i], ns.pos, ns.w, ns.h, ns.r, ns.g, ns.b);
//                    }
//                }
//             
//
//                int newClientNum = *((int*)recv_msgs[3].data());
//                
//
//                if (clientNumber < newClientNum) {
//                    clientNumber = newClientNum;
//                    newShapeAdded = true;
//                }
//                else {
//                    clientNumber = newClientNum;
//                }
//            }
//            for (int i = 0; i < MAX_PLAYERS; i++) {
//                if (otherPlayers[i] != 0 && i != playerArrayIndex) {
//                    setPos(otherPlayers[i], ((ShapeInfo*)recv_msgs[4].data())[i].pos);
//                    if (newShapeAdded) {
//                        for (int i = 0; i < MAX_PLAYERS; i++) {
//                            serverSlotAvailable[i] = ((bool*)recv_msgs[5].data())[i];
//                        }
//                        ShapeInfo s = ((ShapeInfo*)recv_msgs[4].data())[i];
//                        ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(otherPlayers[i]));
//                
//                        if (sa.shape.w != s.w) {
//                            sa.shape.w = s.w;
//                            propertyMap.at(RENDER).at(otherPlayers[i]) = sa;
//                            newShapeAdded = false;
//                        }
//                        if (sa.shape.h != s.h) {
//                            sa.shape.h = s.h;
//                            propertyMap.at(RENDER).at(otherPlayers[i]) = sa;
//                            newShapeAdded = false;
//                        }
//                        if (sa.r != s.r || sa.g != s.g || sa.b != s.b) {
//                            sa.r = s.r;
//                            sa.g = s.g;
//                            sa.b = s.b;
//                            propertyMap.at(RENDER).at(otherPlayers[i]) = sa;
//                            newShapeAdded = false;
//                        }
//                        
//                    }
//                }
//            }
//           /* m.unlock();*/
//        }
//
//        
//        Coord pos = std::get<Coord>(propertyMap.at(POSITION).at(controllable));
//        if (!csInfo) {
//            m.lock();
//            if (!csInfo) {
//                ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(controllable));
//                csInfo = new ShapeInfo(pos, sa.shape.w, sa.shape.h, sa.r, sa.g, sa.b);
//            }
//                m.unlock();
//
//        }
//        if (!messageSent) {
//            m.lock();
//            csInfo->pos = Coord(pos.x - globalOffset->x, pos.y);
//            playerSocket.send(zmq::buffer(csInfo, sizeof(ShapeInfo)), zmq::send_flags::none);
//            m.unlock();
//            messageSent = true;
//        }
//        if (messageSent) {
//            zmq::message_t mes;
//            if (playerSocket.recv(mes, zmq::recv_flags::dontwait)) {
//                messageSent = false;
//            }
//        }
//        
//    }
//}
//
//
///** Initializes objects in game window
//*/
//void onGameStart()
//{
//    /*spawnZone = getNewUUID();
//    addComponent(spawnZone, POSITION);
//    setPos(spawnZone, Coord(50, 385));*/
//
//    ground = getNewUUID();
//    createShape(ground, Coord(-500, 700), 15000, 500, 50, 180, 50);
//
//
//    rightSideBoundary = getNewUUID();
//    createSideBoundary(rightSideBoundary, Coord(1130, 0), 5, SCREEN_HEIGHT);
//
//    leftSideBoundary = getNewUUID();
//    createSideBoundary(leftSideBoundary, Coord(0, 0), 5, SCREEN_HEIGHT);
//
//
//    controllable = getNewUUID();
//    createShape(controllable, Coord(40, 500), 150, 150, rand() % 256, rand() % 256, rand() % 256);
//    addComponent(controllable, PHYSICS);
//    Coord pos = std::get<Coord>(propertyMap.at(POSITION).at(controllable));
//    PhysicsAttribute pa = PhysicsAttribute();
//    pa.acceleration = Coord(0.0, 0.0);
//    pa.positionPrev = pos;
//    propertyMap.at(PHYSICS).at(controllable) = pa;
//    addComponent(controllable, EVENTHANDLER);
//    propertyMap.at(EVENTHANDLER).at(controllable) = EventHandler();
//    EventHandler* handler = std::get_if<EventHandler>(&propertyMap.at(EVENTHANDLER).at(controllable));
//    handler->eventHandleMethod = handleDeathEvent;
//    std::list<EventType> list = std::list<EventType>();
//    list.push_back(DeathEvent);
//    eventManager->registerEvent(list, handler);
//    addComponent(controllable, CONTROL);
//    ControlAttribute conta = ControlAttribute();
//    conta.controllable = true;
//    propertyMap.at(CONTROL).at(controllable) = conta;
//    addComponent(controllable, HEALTH);
//    HealthAttribute h = HealthAttribute();
//    std::cout << "Health: " << h.hitPoints << std::endl;
//    propertyMap.at(HEALTH).at(controllable) = h;
//
//    ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(controllable));
//
//    m.lock();
//    csInfo = new ShapeInfo(pos, sa.shape.w, sa.shape.h, sa.r, sa.g, sa.b);
//    m.unlock();
//    eventSocket.connect(EVENT_SOCKET_ADDRESS);
//    client = std::thread(testClientThread);
//}
//
//Coord* playerPos;
//
//void onGameUpdate()
//{
//
//    if (gameTimeManager->isPaused()) {
//        gameTimeManager->deltaTime = 0;
//    }
//
//    //pause game by pressing p
//    if (isKeyPressed(P)) {
//        if (!getPauseHeld()) {
//            setPauseHeld(true);
//            if (gameTimeManager->isPaused()) {
//                gameTimeManager->unpause();
//            }
//            else {
//                gameTimeManager->pause();
//            }
//        }
//    }
//    else {
//        setPauseHeld(false);
//    }
//
//    //change time scale
//    //.5 scale
//    if (isKeyPressed(J)) {
//        gameTimeManager->setTimeScale(.5);
//    }
//
//    //1 scale
//    if (isKeyPressed(K)) {
//        gameTimeManager->setTimeScale(1.);
//    }
//
//    //2 scale
//    if (isKeyPressed(L)) {
//        gameTimeManager->setTimeScale(2.);
//    }
//    if (isKeyPressed(BACKSPACE)) {
//        quitGame();
//    }
//
//    //Coord pos = std::get<Coord>(propertyMap.at(POSITION).at(controllable));
//    if (!playerPos) {
//        playerPos = &std::get<Coord>(propertyMap.at(POSITION).at(controllable));
//    }
//    physicsManager->resolveCollision(COLLISION, controllable, ground, playerPos, NULL, NULL);
//    if (oscilShape)
//        physicsManager->resolveCollision(COLLISION, controllable, oscilShape, playerPos, NULL, NULL);
//    if (oscilShape2)
//        physicsManager->resolveCollision(COLLISION, controllable, oscilShape2, playerPos, NULL, NULL);
//    if (statShape)
//        physicsManager->resolveCollision(COLLISION, controllable, statShape, playerPos, NULL, NULL);
//    if (deathZone && immunityTime > IMMUNITY_TIME) {
//        //setPos(spawnZone, std::get<Coord>(propertyMap.at(POSITION).at(spawnZone)));
//        //Coord spawnCoord = std::get<Coord>(propertyMap.at(POSITION).at(spawnZone));
//        if (physicsManager->resolveCollision(DEATH, controllable, deathZone, playerPos, NULL/*&spawnCoord*/, NULL)) {
//            //printf("spawn point: %d %d", spawnCoord.x, spawnCoord.y);
//            float prevOffset = globalOffset->x;
//            
//            globalOffset->x = 0;
//            for (auto const& [UUID, Att] : propertyMap.at(POSITION)) {
//                if (controllable == UUID || leftSideBoundary == UUID || rightSideBoundary == UUID /* || spawnZone == UUID*/) continue;
//                    Coord p = std::get<Coord>(Att);
//                    setPos(UUID, Coord(p.x - prevOffset, p.y));
//                    //setPos(UUID, p);
//            }
//        }
//    }
//
//    if (leftSideBoundary && immunityTime > IMMUNITY_TIME) {
//        physicsManager->resolveCollision(SIDE, controllable, leftSideBoundary, playerPos, NULL, -1);
//    }
//
//    if (rightSideBoundary) {
//        physicsManager->resolveCollision(SIDE, controllable, rightSideBoundary, playerPos, NULL, 1);
//    }
//
//    propertyMap.at(POSITION).at(controllable) = *playerPos;
//    controlPlayer(controllable, gameTimeManager->deltaTime); //make object controllable
//    immunityTime += gameTimeManager->deltaTime;
//}
//
//void onGameEnd()
//{
//    if (!inServer) {
//        connector.disconnect("tcp://localhost:5556");
//        subscriber.disconnect("tcp://localhost:5555");
//        eventSocket.disconnect(EVENT_SOCKET_ADDRESS);
//        exit(0);
//    }
//    /*if (client.joinable()) {
//        client.join();
//    }
//    else {
//        client.detach();
//    }*/
//    connect = false;
//    connector.send(zmq::buffer(&connect, sizeof(bool)), zmq::send_flags::sndmore);
//    //connector.send(zmq::str_buffer("Disconnecting"), zmq::send_flags::none);
//    connector.send(zmq::buffer(&playerArrayIndex, sizeof(int)), zmq::send_flags::none);
//    connector.disconnect("tcp://localhost:5556");
//    subscriber.disconnect("tcp://localhost:5555");
//    eventSocket.disconnect(EVENT_SOCKET_ADDRESS);
//    playerSocket.disconnect(connectedAddress);
//}
