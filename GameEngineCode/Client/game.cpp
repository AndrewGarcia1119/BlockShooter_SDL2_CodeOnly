#include "game.hpp"
#include "draw.hpp"
#include "defs.hpp"
#include "input.hpp"
#include "properties.hpp"
#include <iostream>

#define MAX_PLAYERS 1
#define IMMUNITY_TIME 0.5f
#define CONNECTING "Connecting"
#define EVENT_SOCKET_ADDRESS "tcp://localhost:5561"
#define GAME_X_UNIT_MAX 11
#define GAME_Y_UNIT_MAX 8
#define GAME_UNIT_SIZE 80
#define ENEMY_SIZE 50
#define GAME_UNIT_POS_SNAP_X 1
#define GAME_UNIT_POS_SNAP_Y 1
#define GAME_BOARD_PADDING 200
#define GAME_BOARD_TILE_MOVE_MAX 35
#define STARTING_HEALTH 3


float oscillationTimer;
int64_t lastTime;
bool serverSlotAvailable[MAX_PLAYERS] = { true };
int64_t otherPlayers[MAX_PLAYERS];
int64_t controllable;
int64_t bottomBoundary;
int64_t topBoundary;
int64_t leftBoundary;
int64_t rightBoundary;
float immunityTime;

//int64_t spawnZone;

ShapeInfo* csInfo;
std::thread client;
std::mutex m;

zmq::context_t context{ 1 };
// REP (reply) socket for sending client info upon connection
zmq::socket_t connector{ context, zmq::socket_type::req };


zmq::socket_t playerSocket{ context, zmq::socket_type::req };

zmq::socket_t eventSocket{ context, zmq::socket_type::req };

int clientNumber = -1;
int playerArrayIndex = -1;

bool messageSent = false;
bool newShapeAdded = false;
bool inServer = false;


std::string connectedAddress;
bool connect = false;


//this variable is used to help determine if a button was just pressed
bool justPressed = true;

float moveTimer = 0;
float enemyShootTimer = 0;
float enemyShootSpeed = 100.f;

int direction = -1;

int64_t playerProjectile;
std::vector<int64_t>enemyProjectiles = std::vector<int64_t>();
Coord projectileSize = Coord(5, 20);
float playerProjectileMoveSpeed = 10;
float enemyProjectileMoveSpeed = 4;
float levelSpeedMultiplier = 1.f;
typedef struct GameBoardUnit {
    Coord truePos;
    Coord gamePos;
    Coord originalPos;
    int64_t occupyingUUID;

    GameBoardUnit(Coord t_pos, Coord g_pos) : truePos(t_pos), gamePos(g_pos), originalPos(t_pos), occupyingUUID(-1) {}
    GameBoardUnit() : truePos(Coord(0, 0)), gamePos(Coord(0, 0)), originalPos(Coord(0, 0)), occupyingUUID(-1) {}
} GameUnit;

//the 2x multiplier is to match the grid snapping since there are 16 units each position is ever 0.5 units. The - 1 is to prevent the coordinates from going offscreen
GameUnit gameBoard[GAME_Y_UNIT_MAX][GAME_X_UNIT_MAX];
std::vector<int64_t> shipsInGameBoard = std::vector<int64_t>();
std::vector<GameUnit*> *lowestShips =  new std::vector<GameUnit*>(GAME_X_UNIT_MAX);
int64_t healthPreviews[STARTING_HEALTH];



void initGameBoard() {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX; j++) {

            gameBoard[i][j] = GameUnit(Coord(GAME_UNIT_SIZE * j + GAME_BOARD_PADDING, GAME_UNIT_SIZE * i + 20), Coord(GAME_UNIT_POS_SNAP_X * j, GAME_UNIT_POS_SNAP_Y * i));
            //std::cout << "|x: " << gameBoard[i][j].truePos.x << ", y: " << gameBoard[i][j].truePos.y;

        }
        //std::cout << "|" << std::endl;
    }

}

void initProjectiles() {
    playerProjectile = -1;
    /*for (int i = 0; i < GAME_X_UNIT_MAX; i++) {
        enemyProjectiles[i] = -1;
    }*/

}

int getLowestRowWithEnemies() {
    for (int i = GAME_Y_UNIT_MAX - 1; i >= 0; i--) {
        for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
            if (gameBoard[i][j].occupyingUUID != -1) {
                Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(gameBoard[i][j].occupyingUUID));
                if (pos) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int getLeftMostRowWithEnemies() {
    for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
        for (int i = GAME_Y_UNIT_MAX - 1; i >= 0; i--) {
            if (gameBoard[i][j].occupyingUUID != -1) {
                Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(gameBoard[i][j].occupyingUUID));
                if (pos) {
                    return j;
                }
            }
        }
    }
    return -1;
}

int getRightMostRowWithEnemies() {
    for (int j = GAME_X_UNIT_MAX - 1; j >= 0; j--) {
        for (int i = GAME_Y_UNIT_MAX - 1; i >= 0; i--) {
            if (gameBoard[i][j].occupyingUUID != -1) {
                Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(gameBoard[i][j].occupyingUUID));
                if (pos) {
                    return j;
                }
            }
        }
    }
    return -1;
}

void getLowestOccupiedUnitOfEveryColumn(std::vector<GameUnit*> * list) {
    list->clear();
    for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
        for (int i = GAME_Y_UNIT_MAX - 1; i >= 0; i--) {
            if (gameBoard[i][j].occupyingUUID != -1) {
                list->push_back(&gameBoard[i][j]);
                break;
            }
        }
    }
}

static void setPos(int64_t UUID, Coord pos) {
    pos.x += globalOffset->x;
    propertyMap.at(POSITION).at(UUID) = pos;
}



static void createShape(int64_t UUID, Coord pos, float width, float height, int r, int g, int b) {
    addComponent(UUID, POSITION);
    //propertyMap.at(POSITION).at(UUID) = pos;
    setPos(UUID, pos);
    addComponent(UUID, RENDER);
    ShapeAttribute gSA = ShapeAttribute(width, height, r, g, b);
    propertyMap.at(RENDER).at(UUID) = gSA;
    ColliderAttribute gCA = ColliderAttribute();
    gCA.rectCollider.x = pos.x;
    gCA.rectCollider.y = pos.y;
    gCA.rectCollider.w = gSA.shape.w;
    gCA.rectCollider.h = gSA.shape.h;
    addComponent(UUID, COLLISION);
    propertyMap.at(COLLISION).at(UUID) = gCA;
}

static void createHealthShape(int64_t UUID, Coord pos, float width, float height, int r, int g, int b) {
    addComponent(UUID, POSITION);
    //propertyMap.at(POSITION).at(UUID) = pos;
    setPos(UUID, pos);
    addComponent(UUID, RENDER);
    ShapeAttribute gSA = ShapeAttribute(width, height, r, g, b);
    propertyMap.at(RENDER).at(UUID) = gSA;
}

void initHealthUI() {
    for (int i = 0; i < STARTING_HEALTH; i++) {
        healthPreviews[i] = getNewUUID();
        createHealthShape(healthPreviews[i], Coord(SCREEN_WIDTH - 100, 520 + 50 * i), 80, 40, 20, 200, 20);
    }
}

void playerTryShootProjectile() {
    if (playerProjectile < 0) {
        playerProjectile = getNewUUID();
        Coord pos = std::get<Coord>(propertyMap.at(POSITION).at(controllable));
        ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(controllable));
        createShape(playerProjectile, Coord(pos.x + sa.shape.w / 2, pos.y - projectileSize.y - 5), projectileSize.x, projectileSize.y, 0, 100, 0);
    }
}

void enemyShootProjectile() {
    getLowestOccupiedUnitOfEveryColumn(lowestShips);
    if (lowestShips->empty())
        return;
    int randomNum = rand() % lowestShips->size();
    int64_t newEnemyProjectile = getNewUUID();
    Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(lowestShips->at(randomNum)->occupyingUUID));
    ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(lowestShips->at(randomNum)->occupyingUUID));
    createShape(newEnemyProjectile, Coord(pos->x + sa.shape.w / 2, pos->y + projectileSize.y + 5), projectileSize.x, projectileSize.y, 0, 100, 0);
    //std::cout << "pos: " << lowestShips->at(randomNum)->truePos.x << ", " << lowestShips->at(randomNum)->truePos.y << std::endl;
    enemyProjectiles.push_back(newEnemyProjectile);
    std::cout << "size: " << enemyProjectiles.size() << std::endl;
}

void moveAllProjectiles() {
    if (playerProjectile >= 0) {
        Coord* pProjPos = std::get_if<Coord>(&propertyMap.at(POSITION).at(playerProjectile));
        if (pProjPos) {
            pProjPos->y -= playerProjectileMoveSpeed * gameTimeManager->deltaTime;
        }
    }
    for (int i = 0; i < enemyProjectiles.size(); i++) {
        if (enemyProjectiles.at(i) >= 0) {
            Coord* eProjPos = std::get_if<Coord>(&propertyMap.at(POSITION).at(enemyProjectiles.at(i)));
            if (eProjPos) {
                eProjPos->y += enemyProjectileMoveSpeed * gameTimeManager->deltaTime;
            }
        }
    }
}
/** Checks for key presses and updates object position, called every deltaT
*/
void controlPlayer(int64_t UUID, int64_t deltaT) {
    auto& controlAttribute = propertyMap.at(CONTROL).at(UUID); //check for controllable property
    if (!std::holds_alternative<ControlAttribute>(controlAttribute)) { //check if attribute is controllable attribute
        //error, does not have controllable attribute
        return;
    }
    auto& collisionAttribute = propertyMap.at(COLLISION).at(UUID); // get collision attribue to access collider
    SDL_FRect collider = std::get<ColliderAttribute>(collisionAttribute).rectCollider; //get object collider

    Coord* totalMovement = new Coord(); //holds amount that the object moved
    if (isKeyPressed(SPACE_BAR) && justPressed) { //W - up
        justPressed = false;
        playerTryShootProjectile();
        //if (collider.y < 10) {
        //    totalMovement->y -= collider.y;
        //}
        //else {
        //    totalMovement->y -= (10.); // moves 10 pixels per second
        //}
    }
    else if (!isKeyPressed(SPACE_BAR)) {
        justPressed = true;
    }
    if (isKeyPressed(A)) { //A - left
        if (collider.x < 10) {
            totalMovement->x -= collider.x;
        }
        else {
            totalMovement->x -= (10.);
        }
    }
    if (isKeyPressed(D)) { //D - right
        if ((collider.x + collider.w + 10) > SCREEN_WIDTH) {
            totalMovement->x += (SCREEN_WIDTH - (collider.x + collider.w));
        }
        else {
            totalMovement->x += (10.);
        }
    }

    Coord posProp = std::get<Coord>(propertyMap.at(POSITION).at(UUID));
    posProp.x += totalMovement->x * deltaT;
    posProp.y += totalMovement->y * deltaT;

    propertyMap.at(POSITION).at(UUID) = posProp;

}






void setAllShipPos() {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
            if (gameBoard[i][j].occupyingUUID != -1) {
                Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(gameBoard[i][j].occupyingUUID));
                if (pos) {
                    *pos = Coord(gameBoard[i][j].truePos.x + ((GAME_UNIT_SIZE - ENEMY_SIZE) / 2), gameBoard[i][j].truePos.y + ((GAME_UNIT_SIZE - ENEMY_SIZE) / 2));
                }
            }
        }
    }
}

GameUnit* getUnoccupiedUnit() {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
            if (gameBoard[i][j].occupyingUUID < 0) {
                return &gameBoard[i][j];
            }
        }
    }
    return NULL;
}

GameUnit* getOccupiedUnitByID(int64_t UUID) {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
            if (gameBoard[i][j].occupyingUUID == UUID) {
                return &gameBoard[i][j];
            }
        }
    }
    return NULL;
}

void getNewShips(int numBubbles) {
    for (int i = 0; i < numBubbles; i++) {
        int64_t newShip = getNewUUID();
        GameUnit* gu = getUnoccupiedUnit();
        int x = rand() % 3;
        createShape(newShip, Coord(gu->truePos.x + ((GAME_UNIT_SIZE - ENEMY_SIZE) / 2), gu->truePos.y + ((GAME_UNIT_SIZE - ENEMY_SIZE) / 2)), ENEMY_SIZE, ENEMY_SIZE, 200, 0, 0);
        gu->occupyingUUID = newShip;
        shipsInGameBoard.push_back(newShip);
    }
    setAllShipPos();
}


void moveAllEnemies(int direction) {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
            gameBoard[i][j].truePos.x += 5 * direction;
        }
    }
    setAllShipPos();
}

void moveShipsDown() {
        for (int i = GAME_Y_UNIT_MAX - 1; i > 0; i--) {
            for (int j = 0; j < GAME_X_UNIT_MAX; j++) {
                gameBoard[i][j].occupyingUUID = gameBoard[i - 1][j].occupyingUUID;
                gameBoard[i - 1][j].occupyingUUID = -1;

            }
        }
        setAllShipPos();
}

void resetGame(bool win) {
    destroy(&playerProjectile);
    playerProjectile = -1;
    for (int i = 0; i < enemyProjectiles.size(); i++) {
        destroy(&enemyProjectiles.at(i));
        enemyProjectiles.at(i) = -1;
    }
    enemyProjectiles.clear();
    Event e = Event(SpawnEvent);
    eventSocket.send(zmq::buffer(&e, sizeof(Event)), zmq::send_flags::none);
    std::vector<zmq::message_t> recv_msgs;
    zmq::recv_result_t result = zmq::recv_multipart(eventSocket, std::back_inserter(recv_msgs), zmq::recv_flags::none);
    if (result && *result == 2) {
        initGameBoard();
        moveTimer = 0;
        enemyShootTimer = 0;
        if (win) {
            levelSpeedMultiplier *= 1.25;
            //setPos(controllable, Coord(SCREEN_WIDTH / 2 - 40, 650));
        }
        else {
            levelSpeedMultiplier = 1;
            HealthAttribute h = HealthAttribute();
            std::cout << "Starting Health: " << h.hitPoints << std::endl;
            propertyMap.at(HEALTH).at(controllable) = h;
            initHealthUI();
            Coord newPos = *((Coord*)recv_msgs[0].data());
            setPos(controllable, newPos);
        }
        for (int i = 0; i < shipsInGameBoard.size(); i++) {
            destroy(&shipsInGameBoard.at(i));
            shipsInGameBoard.at(i) = -1;
        }
        shipsInGameBoard.clear();
        direction = -1;
        getNewShips(*((int*)recv_msgs[1].data()));
    }
}

void handleCollisionCustom(EventType eventType, int64_t collidedUUID, EventParams otherParams) {
    if (eventType == CollisionEvent) {
        Coord* tempUpdateCoord = std::get<Coord*>(otherParams.at(0));
        int64_t otherUUID = std::get<int64_t>(otherParams.at(1));
        SDL_FRect* overlap = std::get<SDL_FRect*>(otherParams.at(2));
        SDL_FRect collider1 = std::get<SDL_FRect>(otherParams.at(3));
        SDL_FRect collider2 = std::get<SDL_FRect>(otherParams.at(4));
        //SDL_FRect* overlap = new SDL_FRect();
        Coord movement;
        SDL_IntersectFRect(&collider1, &collider2, overlap);
       if (collidedUUID == playerProjectile) {
            destroy(&playerProjectile);
            playerProjectile = -1;

            for (int i = 0; i < enemyProjectiles.size(); i++) {
                if (enemyProjectiles.at(i) == otherUUID) {
                    destroy(&enemyProjectiles.at(i));
                    enemyProjectiles.erase(enemyProjectiles.begin() + i);
                }
            }

            for (int i = 0; i < shipsInGameBoard.size(); i++) {
                if (shipsInGameBoard.at(i) == otherUUID) {
                    destroy(&shipsInGameBoard.at(i));
                    shipsInGameBoard.erase(shipsInGameBoard.begin() + i);
                    GameUnit* gu = getOccupiedUnitByID(otherUUID);
                    if (gu) {
                        gu->occupyingUUID = -1;
                    }
                }
            }
            if (shipsInGameBoard.empty())
                resetGame(true);
            return;
        }
       for (int i = 0; i < enemyProjectiles.size(); i++) {
           if (collidedUUID == enemyProjectiles.at(i)) {
               destroy(&enemyProjectiles.at(i));
               enemyProjectiles.erase(enemyProjectiles.begin() + i);
               //write health reducing code if it hits player
               if (otherUUID == controllable) {
                   HealthAttribute* playerHealth = std::get_if<HealthAttribute>(&propertyMap.at(HEALTH).at(controllable));
                   if (playerHealth) {
                       playerHealth->hitPoints--;
                       destroy(&healthPreviews[playerHealth->hitPoints]);
                       if (playerHealth->hitPoints <= 0) {
                           resetGame(false);
                       }
                   }
               }
               return;
           }
       }
        if (overlap->w < overlap->h) {
            if (collider1.x <= collider2.x) {
                movement.x = -(overlap->w);
            }
            else {
                movement.x = overlap->w;
            }
        }
        else {
            if (collider1.y <= collider2.y) {
                movement.y = -(overlap->h);
            }
            else {
                movement.y = overlap->h;
            }
        }
        //coord variable temporarily holds object changes while program is still object oriented
        if (tempUpdateCoord) {
            tempUpdateCoord->x += movement.x;
            tempUpdateCoord->y += movement.y;
        }
    }
}

void redirectPhysicsCollisionEventHandler() {
    std::list<EventType> list = std::list<EventType>();
    list.push_back(CollisionEvent);
    eventManager->deregisterEvent(list, &physicsManager->handler);
    physicsManager->handler.eventHandleMethod = handleCollisionCustom;
    eventManager->registerEvent(list, &physicsManager->handler);
    propertyMap.at(EVENTHANDLER).at(physicsManager->managerUUID) = physicsManager->handler;
}

void handleDeathEvent(EventType eventType, int64_t victimUUID, EventParams otherParams)
{
    if (eventType == DeathEvent) {
        Event spawn = Event(SpawnEvent, victimUUID);
        //send event to server
        eventSocket.send(zmq::buffer(&spawn, sizeof(Event)), zmq::send_flags::none);

        zmq::message_t reply;
        eventSocket.recv(reply, zmq::recv_flags::none);
        Coord pos = *((Coord*)reply.data());
        setPos(victimUUID, pos);
        HealthAttribute* h = std::get_if<HealthAttribute>(&propertyMap.at(HEALTH).at(victimUUID));
        if (h) {
            h->hitPoints--;
            std::cout << "Health: " << h->hitPoints << std::endl;
        }
        //propertyMap.at(POSITION).at(victimUUID) = pos;

    }
}

void testClientThread() {
    connector.connect("tcp://localhost:5556");
    connect = true;
    connector.send(zmq::buffer(&connect, sizeof(bool)), zmq::send_flags::none);
    /*std::vector<zmq::message_t> first_recv_msgs;
    zmq::recv_result_t firstResult = zmq::recv_multipart(connector, std::back_inserter(first_recv_msgs));*/
    zmq::message_t msg;
    if (connector.recv(msg, zmq::recv_flags::none)) {
        int numShips = *((int*)msg.data());
        getNewShips(numShips);
    }

    playerSocket.connect("tcp://localhost:5557");
    connectedAddress = "tcp://localhost:5557";
    inServer = true;

    
}


void doAllProjectileCollision() {
    if (playerProjectile > 0) {
        for (int i = 0; i < shipsInGameBoard.size(); i++) {
            physicsManager->resolveCollision(COLLISION, playerProjectile, shipsInGameBoard.at(i), NULL, NULL, NULL);
        }
        physicsManager->resolveCollision(COLLISION, playerProjectile, leftBoundary, NULL, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, playerProjectile, rightBoundary, NULL, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, playerProjectile, bottomBoundary, NULL, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, playerProjectile, topBoundary, NULL, NULL, NULL);
    }
    for (int i = 0; i < enemyProjectiles.size(); i++) {
        if (enemyProjectiles.at(i) > 0) {
            if (playerProjectile > 0) {
                physicsManager->resolveCollision(COLLISION, playerProjectile, enemyProjectiles.at(i), NULL, NULL, NULL);
            }
            physicsManager->resolveCollision(COLLISION, enemyProjectiles.at(i), controllable, NULL, NULL, NULL);
            physicsManager->resolveCollision(COLLISION, enemyProjectiles.at(i), leftBoundary, NULL, NULL, NULL);
            physicsManager->resolveCollision(COLLISION, enemyProjectiles.at(i), rightBoundary, NULL, NULL, NULL);
            physicsManager->resolveCollision(COLLISION, enemyProjectiles.at(i), bottomBoundary, NULL, NULL, NULL);
            physicsManager->resolveCollision(COLLISION, enemyProjectiles.at(i), topBoundary, NULL, NULL, NULL);
        }
    }
}

/** Initializes objects in game window
*/
void onGameStart()
{
    initGameBoard();
    redirectPhysicsCollisionEventHandler();
    initProjectiles();
    
    /*spawnZone = getNewUUID();
    addComponent(spawnZone, POSITION);
    setPos(spawnZone, Coord(50, 385));*/

    leftBoundary = getNewUUID();
    createShape(leftBoundary, Coord(-200, -200), 380, 400 + SCREEN_HEIGHT, 50, 50, 50);
    rightBoundary = getNewUUID();
    createShape(rightBoundary, Coord(SCREEN_WIDTH - 180, -200), 380, 400 + SCREEN_HEIGHT, 50, 50, 50);
    topBoundary = getNewUUID();
    createShape(topBoundary, Coord(-200, -200), SCREEN_WIDTH + 400, 220, 50, 50, 50);
    bottomBoundary = getNewUUID();
    createShape(bottomBoundary, Coord(-200, SCREEN_HEIGHT - 20), SCREEN_WIDTH + 400, 220, 50, 50, 50);

    initHealthUI();

    controllable = getNewUUID();
    createShape(controllable, Coord(SCREEN_WIDTH / 2 - 40, 650), 80, 40, 20, 200, 20);
    //addComponent(controllable, PHYSICS);
    Coord pos = std::get<Coord>(propertyMap.at(POSITION).at(controllable));
    /*PhysicsAttribute pa = PhysicsAttribute();
    pa.acceleration = Coord(0.0, 0.0);
    pa.positionPrev = pos;
    propertyMap.at(PHYSICS).at(controllable) = pa;*/
    addComponent(controllable, EVENTHANDLER);
    propertyMap.at(EVENTHANDLER).at(controllable) = EventHandler();
    EventHandler* handler = std::get_if<EventHandler>(&propertyMap.at(EVENTHANDLER).at(controllable));
    handler->eventHandleMethod = handleDeathEvent;
    std::list<EventType> list = std::list<EventType>();
    list.push_back(DeathEvent);
    eventManager->registerEvent(list, handler);
    addComponent(controllable, CONTROL);
    ControlAttribute conta = ControlAttribute();
    conta.controllable = true;
    propertyMap.at(CONTROL).at(controllable) = conta;
    addComponent(controllable, HEALTH);
    HealthAttribute h = HealthAttribute();
    std::cout << "Starting Health: " << h.hitPoints << std::endl;
    propertyMap.at(HEALTH).at(controllable) = h;

    ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(controllable));

    
    
    //getNewShips(44);

    eventSocket.connect(EVENT_SOCKET_ADDRESS);
    client = std::thread(testClientThread);
}



Coord* playerPos;

void onGameUpdate()
{

    if (gameTimeManager->isPaused()) {
        gameTimeManager->deltaTime = 0;
    }
    if (!inServer) return;
    moveTimer += gameTimeManager->deltaTime;
    enemyShootTimer += gameTimeManager->deltaTime;
    
    if (moveTimer > (6 + shipsInGameBoard.size()) / levelSpeedMultiplier) {
        bool doNotMoveSideways = false;
        moveTimer = 0;
        //std::cout << "bm: " << getLowestRowWithEnemies() << ", lm: " << getLeftMostRowWithEnemies()  << ", rm: " << getRightMostRowWithEnemies()  << std::endl;
        if (direction < 0) {
            if (gameBoard[0][0].truePos.x <= gameBoard[0][0].originalPos.x - GAME_BOARD_TILE_MOVE_MAX - getLeftMostRowWithEnemies() * GAME_UNIT_SIZE + 1) {
                direction = 1;
            }
        }
        else {
            if (gameBoard[0][0].truePos.x >= gameBoard[0][0].originalPos.x + GAME_BOARD_TILE_MOVE_MAX + ( GAME_X_UNIT_MAX - getRightMostRowWithEnemies() - 1) * GAME_UNIT_SIZE - 1) {
                if (getLowestRowWithEnemies() >= GAME_Y_UNIT_MAX - 1) {
                    resetGame(false);
                    return;
                }
                else {
                    moveShipsDown();
                    direction = -1;
                    doNotMoveSideways = true;
                }
            }
        }
        if (!doNotMoveSideways) {
            moveAllEnemies(direction);
        }
    }
    if (enemyShootTimer >= enemyShootSpeed) {
        enemyShootTimer = 0;
        enemyShootProjectile();
    }
    moveAllProjectiles();
    
    //Coord pos = std::get<Coord>(propertyMap.at(POSITION).at(controllable));
    if (!playerPos) {
        playerPos = &std::get<Coord>(propertyMap.at(POSITION).at(controllable));
    }
    if (immunityTime > IMMUNITY_TIME) {
        physicsManager->resolveCollision(COLLISION, controllable, leftBoundary, playerPos, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, controllable, rightBoundary, playerPos, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, controllable, bottomBoundary, playerPos, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, controllable, topBoundary, playerPos, NULL, NULL);
    }
    doAllProjectileCollision();
   /* physicsManager->resolveCollision(COLLISION, controllable, ground, playerPos, NULL, NULL);
    if (oscilShape)
        physicsManager->resolveCollision(COLLISION, controllable, oscilShape, playerPos, NULL, NULL);
    if (oscilShape2)
        physicsManager->resolveCollision(COLLISION, controllable, oscilShape2, playerPos, NULL, NULL);
    if (statShape)
        physicsManager->resolveCollision(COLLISION, controllable, statShape, playerPos, NULL, NULL);*/
    //if (deathZone && immunityTime > IMMUNITY_TIME) {
        //setPos(spawnZone, std::get<Coord>(propertyMap.at(POSITION).at(spawnZone)));
        //Coord spawnCoord = std::get<Coord>(propertyMap.at(POSITION).at(spawnZone));
        //if (physicsManager->resolveCollision(DEATH, controllable, deathZone, playerPos, NULL/*&spawnCoord*/, NULL)) {
        //    //printf("spawn point: %d %d", spawnCoord.x, spawnCoord.y);
        //    float prevOffset = globalOffset->x;

        //    globalOffset->x = 0;
        //    for (auto const& [UUID, Att] : propertyMap.at(POSITION)) {
        //        if (controllable == UUID || leftSideBoundary == UUID || rightSideBoundary == UUID /* || spawnZone == UUID*/) continue;
        //        Coord p = std::get<Coord>(Att);
        //        setPos(UUID, Coord(p.x - prevOffset, p.y));
        //        //setPos(UUID, p);
        //    }
        //}
    //}

    /*if (leftSideBoundary && immunityTime > IMMUNITY_TIME) {
        physicsManager->resolveCollision(SIDE, controllable, leftSideBoundary, playerPos, NULL, -1);
    }

    if (rightSideBoundary) {
        physicsManager->resolveCollision(SIDE, controllable, rightSideBoundary, playerPos, NULL, 1);
    }*/

    propertyMap.at(POSITION).at(controllable) = *playerPos;
    controlPlayer(controllable, gameTimeManager->deltaTime); //make object controllable
    immunityTime += gameTimeManager->deltaTime;
}

void onGameEnd()
{
    if (!inServer) {
        connector.disconnect("tcp://localhost:5556");
        eventSocket.disconnect(EVENT_SOCKET_ADDRESS);
        exit(0);
    }
    /*if (client.joinable()) {
        client.join();
    }
    else {
        client.detach();
    }*/
    connect = false;
    connector.send(zmq::buffer(&connect, sizeof(bool)), zmq::send_flags::none);

    connector.disconnect("tcp://localhost:5556");
    eventSocket.disconnect(EVENT_SOCKET_ADDRESS);
    playerSocket.disconnect(connectedAddress);
}
