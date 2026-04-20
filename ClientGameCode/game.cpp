#include "game.hpp"
#include "draw.hpp"
#include "defs.hpp"
#include "input.hpp"
#include "properties.hpp"
#include <iostream>

#define MAX_PLAYERS 1
#define CONNECTING "Connecting"
#define PREVIEW_BUBBLE_SIZE 4
#define GAME_X_UNIT_MAX 15
#define GAME_Y_UNIT_MAX 8
#define GAME_UNIT_SIZE 80
#define GAME_UNIT_POS_SNAP_X 0.5
#define GAME_UNIT_POS_SNAP_Y 1
#define GAME_BOARD_PADDING 20
#define STARTING_MOVEMENT_WAIT 4
#define STARTING_POP_SCORE 10

int64_t lastTime;
bool serverSlotAvailable[MAX_PLAYERS] = { true };


std::thread client;
std::mutex m;

zmq::context_t context{ 1 };
// REP (reply) socket for sending client info upon connection
zmq::socket_t connector{ context, zmq::socket_type::req };

zmq::socket_t playerSocket{ context, zmq::socket_type::req };


int clientNumber = -1;

bool inServer = false;

std::string connectedAddress;
bool connect = false;


//this variable is used to help determine if a button was just pressed
bool justPressed = true;

Coord currentMovementDirection = Coord();
//this variable determines the speed of a bubble
float moveSpeed = 10.f;

int64_t controllable;
int64_t mainPreviewBubble;
int64_t otherPreviewBubbles[PREVIEW_BUBBLE_SIZE];
//int64_t otherPreviews[PREVIEW_BUBBLE_SIZE];
int untilMovement = STARTING_MOVEMENT_WAIT;
std::vector<int64_t> bubblesInGameBoard = std::vector<int64_t>();
int64_t leftBoundary;
int64_t rightBoundary;
int64_t topBoundary;
int64_t bottomBoundary;
int64_t blockBoundary;
int score = 0;

int nextR = -1;
int nextG = -1;
int nextB = -1;



typedef struct GameBoardUnit {
    Coord truePos;
    Coord gamePos;
    int64_t occupyingUUID;

    GameBoardUnit(Coord t_pos, Coord g_pos) : truePos(t_pos), gamePos(g_pos), occupyingUUID(-1) {}
    GameBoardUnit() : truePos(Coord(0, 0)), gamePos(Coord(0, 0)), occupyingUUID(-1) {}
} GameUnit;

//the 2x multiplier is to match the grid snapping since there are 16 units each position is ever 0.5 units. The - 1 is to prevent the coordinates from going offscreen
GameUnit gameBoard[GAME_Y_UNIT_MAX][GAME_X_UNIT_MAX * 2];
std::map<Coord, GameUnit*> unitByPos = std::map<Coord, GameUnit*>();


void initGameBoard() {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {

            gameBoard[i][j] = GameUnit(Coord(GAME_UNIT_SIZE * j / 2 + GAME_BOARD_PADDING, GAME_UNIT_SIZE * i + GAME_BOARD_PADDING), Coord(GAME_UNIT_POS_SNAP_X * j, GAME_UNIT_POS_SNAP_Y * i));
            //std::cout << "|x: " << gameBoard[i][j].truePos.x << ", y: " << gameBoard[i][j].truePos.y;
           
        }
        //std::cout << "|" << std::endl;
    }
    
}

void initPreviewBubbles() {
    for (int i = 0; i < PREVIEW_BUBBLE_SIZE; i++) {
        otherPreviewBubbles[i] = -1;
    }
}

static void setPos(int64_t UUID, Coord pos) {
    pos.x += globalOffset->x;
    propertyMap.at(POSITION).at(UUID) = pos;
}

/// <summary>
/// create preview bubble, has no collider
/// </summary>
/// <param name="UUID">uuid of new bubble</param>
/// <param name="pos">starting position of bubble</param>
/// <param name="width">width of bubble (bubbles are not circular in this case)</param>
/// <param name="height">height of bubble </param>
/// <param name="r">how red is it?</param>
/// <param name="g">how green is it?</param>
/// <param name="b">how blue is it></param>
static void createPreview(int64_t UUID, Coord pos, float width, float height, int r, int g, int b) {
    addComponent(UUID, POSITION);
    setPos(UUID, pos);
    addComponent(UUID, RENDER);
    ShapeAttribute gSA = ShapeAttribute(width, height, r, g, b);
    propertyMap.at(RENDER).at(UUID) = gSA;
}

static void createShape(int64_t UUID, Coord pos, float width, float height, int r, int g, int b) {
    addComponent(UUID, POSITION);
    setPos(UUID, pos);
    addComponent(UUID, RENDER);
    ShapeAttribute gSA = ShapeAttribute(width, height, r, g, b);
    propertyMap.at(RENDER).at(UUID) = gSA;
    ColliderAttribute gCA = ColliderAttribute();
    gCA.rectCollider.x = gSA.shape.x;
    gCA.rectCollider.y = gSA.shape.y;
    gCA.rectCollider.w = gSA.shape.w;
    gCA.rectCollider.h = gSA.shape.h;
    addComponent(UUID, COLLISION);
    propertyMap.at(COLLISION).at(UUID) = gCA;
}

void prepareBubbleForShot(int64_t bubble) {
    addComponent(bubble, COLLISION);
    ShapeAttribute* sa = std::get_if<ShapeAttribute>(&propertyMap.at(RENDER).at(bubble));
    if (!sa)
        return;
    ColliderAttribute ca = ColliderAttribute();
    ca.rectCollider.x = sa->shape.x;
    ca.rectCollider.y = sa->shape.y;
    ca.rectCollider.w = sa->shape.w;
    ca.rectCollider.h = sa->shape.h;
    propertyMap.at(COLLISION).at(bubble) = ca;
}
/// <summary>
/// This should be called after bubble UUIDs get changed on gameboard. This will ensure that all bubbles are in their intended location
/// </summary>
void setAllBubblePos() {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {
            if (gameBoard[i][j].occupyingUUID != -1) {
                Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(gameBoard[i][j].occupyingUUID));
                if (pos) {
                    *pos = gameBoard[i][j].truePos;
                }
            }
        }
    }
}

/// <summary>
/// called every frame, moves a bubble if it should move
/// </summary>
/// <param name="bubble"> The direction of the movement </param>
void moveBubble(int64_t bubble) {
    if (currentMovementDirection.x == 0 && currentMovementDirection.y == 0)
        return;
    Coord* pos = std::get_if<Coord>(&propertyMap.at(POSITION).at(bubble));
    if (!pos)
        return;
    pos->x += currentMovementDirection.x * moveSpeed * gameTimeManager->deltaTime;
    pos->y += currentMovementDirection.y * moveSpeed * gameTimeManager->deltaTime;
    //std::cout << "should have moved" << std::endl;
}

/// <summary>
/// Shoots the bubble shown in the preview in a given direction.
/// </summary>
/// <param name="direction"> The direction of the shot, this functional will automatically set the direction to a unit vector, so the magnitude does not matter </param>
void shootBubble(Coord direction) {
    float magnitude = sqrtf(direction.x * direction.x + direction.y * direction.y);
    Coord unitDir = Coord(direction.x / magnitude, direction.y / magnitude);
    currentMovementDirection = unitDir;
    //std::cout << "should have shot" << std::endl;
}

void resetBubbleDirection() {
    currentMovementDirection = Coord();
}

Coord vectorDifference(Coord vec1, Coord vec2) {
    return Coord(vec1.x - vec2.x, vec1.y - vec2.y);
}


void showPreviewBubbles() {
    for (int i = 0; i < PREVIEW_BUBBLE_SIZE; i++) {
        if (otherPreviewBubbles[i] >= 0) {
            destroy(&otherPreviewBubbles[i]);
            otherPreviewBubbles[i] = -1;
        }
    }
    for (int i = 0; i < untilMovement; i++) {
        otherPreviewBubbles[i] = getNewUUID();
        int random = rand() % 3;
        int r, g, b;
        if (i == 0) {
            r = random == 0 ? 255 : 0;
            g = random == 1 ? 255 : 0;
            b = random == 2 ? 255 : 0;
            nextR = r;
            nextG = g;
            nextB = b;
        }
        else {
            r = 200;
            g = 200;
            b = 200;
        }
        createPreview(otherPreviewBubbles[i], Coord(30 + 50 * i, 620), GAME_UNIT_SIZE / 2, GAME_UNIT_SIZE / 2, r, g, b);
    }
}

void resetPreview() {
    mainPreviewBubble = getNewUUID();
    if (nextR == -1) {
        int x = rand() % 3;
        createPreview(mainPreviewBubble, Coord(640, 600), GAME_UNIT_SIZE - 1, GAME_UNIT_SIZE - 1, x == 0 ? 255 : 0, x == 1 ? 255 : 0, x == 2 ? 255 : 0);
    }
    else {
        createPreview(mainPreviewBubble, Coord(640, 600), GAME_UNIT_SIZE - 1, GAME_UNIT_SIZE - 1, nextR, nextG, nextB);
    }
    showPreviewBubbles();
}

/// <summary>
/// Gets top left-most unoccupied unit in the gameboard
/// </summary>
/// <returns></returns>
GameUnit* getUnoccupiedUnit() {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {
            if (gameBoard[i][j].occupyingUUID < 0) {
                if ((i == 0 || gameBoard[i - 1][j].occupyingUUID < 0) && (j == 0 || gameBoard[i][j - 1].occupyingUUID < 0))
                    return &gameBoard[i][j];
            }
        }
    }
    return NULL;
}

GameUnit* getOccupiedUnitByID(int64_t UUID) {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {
            if (gameBoard[i][j].occupyingUUID == UUID) {
                return &gameBoard[i][j];
            }
        }
    }
    return NULL;
}

GameUnit* getUnitByCoord(Coord gamePos) {
    for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
        for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {
            if ((gameBoard[i][j].gamePos.x <= gamePos.x + 0.2 && gameBoard[i][j].gamePos.x >= gamePos.x - 0.2) && (gameBoard[i][j].gamePos.y <= gamePos.y + 0.2 && gameBoard[i][j].gamePos.y >= gamePos.y - 0.2)) {
                return &gameBoard[i][j];
            }
        }
    }
    return NULL;
}

/// <summary>
/// reads adjacent bubbles and gives output in the form of a list given from the parameter
/// </summary>
/// <param name="startingGU"> the GameUnit you want to get the adjacent values of </param>
/// <param name="list"> list to store adjacent unit info</param>
void getAdjacentBubbles(const GameUnit* startingGU, GameUnit* adjacentUnits[6]) {
    //GameUnit* adjacentUnits[6];
    adjacentUnits[0] = getUnitByCoord(Coord(startingGU->gamePos.x - GAME_UNIT_POS_SNAP_X * 2, startingGU->gamePos.y));
    adjacentUnits[1] = getUnitByCoord(Coord(startingGU->gamePos.x + GAME_UNIT_POS_SNAP_X * 2, startingGU->gamePos.y));
    adjacentUnits[2] = getUnitByCoord(Coord(startingGU->gamePos.x - GAME_UNIT_POS_SNAP_X, startingGU->gamePos.y + GAME_UNIT_POS_SNAP_Y));
    adjacentUnits[3] = getUnitByCoord(Coord(startingGU->gamePos.x + GAME_UNIT_POS_SNAP_X, startingGU->gamePos.y + GAME_UNIT_POS_SNAP_Y));
    adjacentUnits[4] = getUnitByCoord(Coord(startingGU->gamePos.x - GAME_UNIT_POS_SNAP_X, startingGU->gamePos.y - GAME_UNIT_POS_SNAP_Y));
    adjacentUnits[5] = getUnitByCoord(Coord(startingGU->gamePos.x + GAME_UNIT_POS_SNAP_X, startingGU->gamePos.y - GAME_UNIT_POS_SNAP_Y));
}
void popBubbles(int64_t *start, float scoreMultiplier) {
    GameUnit* startingGU = getOccupiedUnitByID(*start);
    GameUnit* adjacentUnits[6];
    ShapeAttribute sa = std::get<ShapeAttribute>(propertyMap.at(RENDER).at(*start));
    getAdjacentBubbles(startingGU, adjacentUnits);
    score += STARTING_POP_SCORE * scoreMultiplier;
    for (int i = 0; i < bubblesInGameBoard.size(); i++) {
        if (bubblesInGameBoard.at(i) == *start) {
            bubblesInGameBoard.erase(bubblesInGameBoard.begin() + i);
            break;
        }
    }
    destroy(start);
    startingGU->occupyingUUID = -1;
    for (int i = 0; i < 6; i++) {
        ShapeAttribute* sa2 = NULL;
        if (adjacentUnits[i] && adjacentUnits[i]->occupyingUUID != -1) {
            sa2 = std::get_if<ShapeAttribute>(&propertyMap.at(RENDER).at(adjacentUnits[i]->occupyingUUID));
        }
        if (sa2 && sa.r == sa2->r && sa.g == sa2->g && sa.b == sa2->b) {
            popBubbles(&adjacentUnits[i]->occupyingUUID, scoreMultiplier * 1.2f);
        }
    }
    
}

void getNewBubbles(int numBubbles) {
    for (int i = 0; i < numBubbles; i++) {
        int64_t newBubble = getNewUUID();
        GameUnit* gu = getUnoccupiedUnit();
        int x = rand() % 3;
        createShape(newBubble, gu->truePos, GAME_UNIT_SIZE - 1, GAME_UNIT_SIZE - 1, x == 0 ? 255 : 0, x == 1 ? 255 : 0, x == 2 ? 255 : 0);
        gu->occupyingUUID = newBubble;
        bubblesInGameBoard.push_back(newBubble);
    }
    setAllBubblePos();
}

void resetGame(int numBubbles) {
    for (int i = 0; i < bubblesInGameBoard.size(); i++) {
        destroy(&bubblesInGameBoard.at(i));
    }
    bubblesInGameBoard.clear();
    if (mainPreviewBubble > 0) {
        destroy(&mainPreviewBubble);
    }
    untilMovement = STARTING_MOVEMENT_WAIT;
    currentMovementDirection = Coord();
    score = 0;
    nextR = -1;
    nextG = -1;
    nextB = -1;
    initGameBoard();
    resetPreview();
    getNewBubbles(numBubbles);

}

void checkFinalRow() {
    for (int i = 0; i < GAME_X_UNIT_MAX * 2; i++) {
        if (gameBoard[GAME_Y_UNIT_MAX - 1][i].occupyingUUID != -1) {
            std::cout << "Game Over!" << std::endl;
            std::cout << "Final Score: " << score << std::endl;
            zmq::message_t message;
            playerSocket.send(zmq::str_buffer("request"), zmq::send_flags::none);
            if (playerSocket.recv(message, zmq::recv_flags::none)) {
                resetGame(*((int*)message.data()));
            }
        }
    }
}

void tryMovingBubbles() {
    untilMovement--;
    if (untilMovement <= 0) {
        untilMovement = rand() % STARTING_MOVEMENT_WAIT + 1;
        for (int i = GAME_Y_UNIT_MAX - 1; i > 0; i--) {
            for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {
                gameBoard[i][j].occupyingUUID = gameBoard[i - 1][j].occupyingUUID;
                gameBoard[i - 1][j].occupyingUUID = -1;
                
            }
        }
        setAllBubblePos();
    }
}

void handleCollisionCustom(EventType eventType, int64_t collidedUUID, EventParams otherParams) {
    //std::cout << "Using custom collision" << std::endl;
    if (eventType == CollisionEvent) {
        if (collidedUUID != mainPreviewBubble)
            return;
        
        int64_t otherUUID = std::get<int64_t>(otherParams.at(1));
        if (otherUUID == leftBoundary) {
            if (currentMovementDirection.x < 0)
                currentMovementDirection.x *= -1;
        }
        else if (otherUUID == rightBoundary) {
            if (currentMovementDirection.x > 0)
                currentMovementDirection.x *= -1;
        }
        else if (otherUUID == topBoundary) {
            if (currentMovementDirection.y < 0)
                currentMovementDirection.y *= -1;
        }
        else if (otherUUID == bottomBoundary) {
            if (currentMovementDirection.y > 0)
                currentMovementDirection.y *= -1;
        }
        else {
            Coord* tempUpdateCoord = std::get<Coord*>(otherParams.at(0));
            SDL_FRect* overlap = std::get<SDL_FRect*>(otherParams.at(2));
            SDL_FRect collider1 = std::get<SDL_FRect>(otherParams.at(3));
            SDL_FRect collider2 = std::get<SDL_FRect>(otherParams.at(4));
            //SDL_FRect* overlap = new SDL_FRect();
            //Coord movement;
            SDL_IntersectFRect(&collider1, &collider2, overlap);
            GameUnit* gu = getOccupiedUnitByID(otherUUID);
            if (!gu)
                return;
            GameUnit* adjGu;
            Coord adjGamePos;
            resetBubbleDirection();
            if (overlap->w < overlap->h) {
                if (collider1.x <= collider2.x) {
                    adjGamePos = Coord(gu->gamePos.x - GAME_UNIT_POS_SNAP_X * 2, gu->gamePos.y);
                    adjGu = getUnitByCoord(adjGamePos);
                    if (adjGu && adjGu->occupyingUUID == -1) {
                        setPos(collidedUUID, adjGu->truePos);
                    }
                }
                else {
                    adjGamePos = Coord(gu->gamePos.x + GAME_UNIT_POS_SNAP_X * 2, gu->gamePos.y);
                    adjGu = getUnitByCoord(adjGamePos);
                    if (adjGu && adjGu->occupyingUUID == -1) {
                        setPos(collidedUUID, adjGu->truePos);
                    }
                }
            }
            else {
                if (collider1.y <= collider2.y) {
                    if (collider1.x <= collider2.x) {
                        adjGamePos = Coord(gu->gamePos.x - GAME_UNIT_POS_SNAP_X, gu->gamePos.y - GAME_UNIT_POS_SNAP_Y);
                        adjGu = getUnitByCoord(adjGamePos);
                        if (adjGu && adjGu->occupyingUUID == -1) {
                            setPos(collidedUUID, adjGu->truePos);
                        }
                    }
                    else {
                        adjGamePos = Coord(gu->gamePos.x + GAME_UNIT_POS_SNAP_X, gu->gamePos.y - GAME_UNIT_POS_SNAP_Y);
                        adjGu = getUnitByCoord(adjGamePos);
                        if (adjGu && adjGu->occupyingUUID == -1) {
                            setPos(collidedUUID, adjGu->truePos);
                        }
                    }
                }
                else {
                    if (collider1.x <= collider2.x) {
                        adjGamePos = Coord(gu->gamePos.x - GAME_UNIT_POS_SNAP_X, gu->gamePos.y + GAME_UNIT_POS_SNAP_Y);
                        adjGu = getUnitByCoord(adjGamePos);
                        if (adjGu && adjGu->occupyingUUID == -1) {
                            setPos(collidedUUID, adjGu->truePos);
                        }
                    }
                    else {
                        adjGamePos = Coord(gu->gamePos.x + GAME_UNIT_POS_SNAP_X, gu->gamePos.y + GAME_UNIT_POS_SNAP_Y);
                        adjGu = getUnitByCoord(adjGamePos);
                        if (adjGu && adjGu->occupyingUUID == -1) {
                            setPos(collidedUUID, adjGu->truePos);
                        }
                    }
                }
            }
            if (adjGu && adjGu->occupyingUUID == -1) {
                adjGu->occupyingUUID = collidedUUID;
                bubblesInGameBoard.push_back(collidedUUID);
                setAllBubblePos();
            }
            GameUnit* startingGU = getOccupiedUnitByID(collidedUUID);
            if (!startingGU) {
                return;
            }
            GameUnit* adjacentUnits[6];
            ShapeAttribute* sa = std::get_if<ShapeAttribute>(&propertyMap.at(RENDER).at(collidedUUID));
            getAdjacentBubbles(startingGU, adjacentUnits);
            bool didPop = false;
            for (int i = 0; i < 6; i++) {
                bool pop = false;
                ShapeAttribute* sa2 = NULL;
                if (adjacentUnits[i] && adjacentUnits[i]->occupyingUUID != -1) {
                    sa2 = std::get_if<ShapeAttribute>(&propertyMap.at(RENDER).at(adjacentUnits[i]->occupyingUUID));
                }
                if (sa2 && sa->r == sa2->r && sa->g == sa2->g && sa->b == sa2->b) {
                    GameUnit* adjacentUnits2[6];
                    getAdjacentBubbles(adjacentUnits[i], adjacentUnits2);
                    for (int j = 0; j < 6; j++) {
                        ShapeAttribute* sa3 = NULL;
                        if (adjacentUnits2[j] && adjacentUnits2[j]->occupyingUUID != -1) {
                            sa3 = std::get_if<ShapeAttribute>(&propertyMap.at(RENDER).at(adjacentUnits2[j]->occupyingUUID));

                        }
                        if (sa3 && sa2 != sa3 && sa != sa3 && sa->r == sa3->r && sa->g == sa3->g && sa->b == sa3->b) {
                            pop = true;
                            break;
                        }
                    }
                    if (pop) {
                        popBubbles(&mainPreviewBubble, 1);
                        didPop = true;
                        std::cout << "Current Score: " << score << std::endl;
                        break;
                    }
                    
                }
            }
            if (!didPop) {
                tryMovingBubbles();
                std::cout << untilMovement << " more non-popping moves until all bubbles move" << std::endl;
            }
            else {
                bool isFinished = true;
                for (int i = 0; i < GAME_Y_UNIT_MAX; i++) {
                    for (int j = 0; j < GAME_X_UNIT_MAX * 2; j++) {
                        if (gameBoard[i][j].occupyingUUID > 0) {
                            isFinished = false;
                            break;
                        }
                    }
                }
                if (isFinished) {
                    std::cout << "completed\n" << std::endl;
                    std::cout << "Final Score: " << score << std::endl;
                    zmq::message_t message;

                    playerSocket.send(zmq::str_buffer("request"), zmq::send_flags::none);
                    if (playerSocket.recv(message, zmq::recv_flags::none)) {
                        resetGame(*((int*)message.data()));
                    }
                    
                    return;
                }
            }
            resetPreview();
            checkFinalRow();
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

Coord getMiddleOfPlayer() {
    Coord previewPos = std::get<Coord>(propertyMap.at(POSITION).at(mainPreviewBubble));
    return Coord(previewPos.x + GAME_UNIT_SIZE / 2, previewPos.y + GAME_UNIT_SIZE / 2);
}

/** Checks for key presses and updates object position, called every deltaT
*/
void controlPlayer(int64_t UUID, int64_t deltaT) {
    if (isKeyPressed(LEFT_CLICK) && justPressed) { //W - up
        justPressed = false;
        if (currentMovementDirection.x != 0 || currentMovementDirection.y != 0)
            return;
        Coord mousePos = getMousePos();
        //Coord previewPos = std::get<Coord>(propertyMap.at(POSITION).at(mainPreviewBubble));
        prepareBubbleForShot(mainPreviewBubble);
        shootBubble(vectorDifference(mousePos, getMiddleOfPlayer()));
        //std::cout << "Current Mouse position - x: " << mousePos.x << ", y: " << mousePos.y << std::endl;
       
    }
    else if (!isKeyPressed(LEFT_CLICK)) {
        justPressed = true;
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
        int numBlocks = *((int*) msg.data());
        getNewBubbles(numBlocks);
    }
    

    /*m.unlock();*/
    playerSocket.connect("tcp://localhost:5557");
    connectedAddress = "tcp://localhost:5557";
    inServer = true;
   
}




/** Initializes objects in game window
*/
void onGameStart()
{
    //physicsManager->gravity->y = 0.f;
    redirectPhysicsCollisionEventHandler();
    initGameBoard();
    initPreviewBubbles;

    blockBoundary = getNewUUID();
    createShape(bottomBoundary, Coord(0, 581), SCREEN_WIDTH, 2, 150, 0, 0);
    leftBoundary = getNewUUID();
    createShape(leftBoundary, Coord(-200, -200), 219, 400 + SCREEN_HEIGHT, 50, 180, 50);
    rightBoundary = getNewUUID();
    createShape(rightBoundary, Coord(SCREEN_WIDTH - 20, -200), 220, 400 + SCREEN_HEIGHT, 50, 180, 50);
    topBoundary = getNewUUID();
    createShape(topBoundary, Coord(-200, -200), SCREEN_WIDTH + 400, 219, 50, 180, 50);
    bottomBoundary = getNewUUID();
    createShape(bottomBoundary, Coord(-200, SCREEN_HEIGHT - 20), SCREEN_WIDTH + 400, 220, 50, 180, 50);
    

    //enable controls
    controllable = getNewUUID();
    addComponent(controllable, CONTROL);
    ControlAttribute conta = ControlAttribute();
    conta.controllable = true;
    propertyMap.at(CONTROL).at(controllable) = conta;

    //create preview bubble
    resetPreview();
    
    client = std::thread(testClientThread);
    //getNewBubbles(50);
    
}


void onGameUpdate()
{
    if (gameTimeManager->isPaused()) {
        gameTimeManager->deltaTime = 0;
    }
    if (!inServer) 
        return;
    moveBubble(mainPreviewBubble);
    
    if (currentMovementDirection.x != 0 || currentMovementDirection.y != 0) {
        physicsManager->resolveCollision(COLLISION, mainPreviewBubble, leftBoundary, NULL, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, mainPreviewBubble, rightBoundary, NULL, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, mainPreviewBubble, topBoundary, NULL, NULL, NULL);
        physicsManager->resolveCollision(COLLISION, mainPreviewBubble, bottomBoundary, NULL, NULL, NULL);
        for (int i = 0; i < bubblesInGameBoard.size(); i++) {
            if (mainPreviewBubble != bubblesInGameBoard[i]) {
                physicsManager->resolveCollision(COLLISION, mainPreviewBubble, bubblesInGameBoard.at(i), NULL, NULL, NULL);
            }
        }
    }
    controlPlayer(controllable, gameTimeManager->deltaTime); //make object controllable
    
}

void onGameEnd()
{
    if (!inServer) {
        connector.disconnect("tcp://localhost:5556");
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
    //connector.send(zmq::str_buffer("Disconnecting"), zmq::send_flags::none);
    connector.disconnect("tcp://localhost:5556");
    playerSocket.disconnect(connectedAddress);
}
