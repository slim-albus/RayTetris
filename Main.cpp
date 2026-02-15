#include <raylib.h>
#include <algorithm>

enum Block {BarBlock, BoxBlock, TBlock, LBlock, JBlock, ZBlock, SBlock};
enum Orientation {Up, Right, Down, Left};

const int BOARD_WIDTH = 300;
const int BOARD_HEIGHT = 600;
const int ROWS = 20;
const int COLS = 10;
const int CELL_WIDTH = BOARD_WIDTH / COLS;
const int CELL_HEIGHT = BOARD_HEIGHT / ROWS;
const int INFO_AREA_WIDTH = 250;

const int EMPTY_CELL = 0;
const int KICK_TRIES = 6;
const int WALL_KICKS[KICK_TRIES][2] = {
    {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1}, {0, 1}
};

const Color WINDOW_BG_COLOR = RAYWHITE;
const Color GRID_LINE_COLOR = LIGHTGRAY;

const Color BLOCK_COLORS[7] = {
    SKYBLUE, GOLD, VIOLET, ORANGE, BLUE, RED, GREEN
};

// Shape data: [block][orientation][cell index][x/y].
const int BLOCK_SHAPES[7][4][4][2] = {
    // BarBlock (I)
    {
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}}
    },
    // BoxBlock (O)
    {
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}}
    },
    // TBlock
    {
        {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {1, 2}}
    },
    // LBlock
    {
        {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}}
    },
    // JBlock
    {
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 0}, {1, 1}, {0, 2}, {1, 2}}
    },
    // ZBlock
    {
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {0, 2}}
    },
    // SBlock
    {
        {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}}
    }
};

struct ActiveBlock {
    Block block;
    Orientation orientation;
    int x;
    int y;
    Color color;
};

int cellInfo[ROWS][COLS];
int score = 0;
int clearedLinesTotal = 0;
int level = 1;
bool gameOver = false;
float fallDelay = 0.0f;
Block nextBlock = BarBlock;

void init();
void initCells();
void playGame(ActiveBlock &activeBlock);

namespace GameLogic {
void resetGame(ActiveBlock &activeBlock);
int blockCellX(Block block, Orientation orientation, int index);
int blockCellY(Block block, Orientation orientation, int index);
Color getBlockColor(Block block);
const char *getBlockName(Block block);
Block chooseRandomBlock();
Orientation chooseRandomOrientation(Block block);
int findMiddle(Block block, Orientation orientation);
bool canPlace(Block block, Orientation orientation, int x, int y);
void spawnBlock(ActiveBlock &activeBlock);
void lockActiveBlock(const ActiveBlock &activeBlock);
int clearCompletedLines();
void applyLineClearScore(int linesCleared);
float getCurrentFallDelay();
void finishActivePiece(ActiveBlock &activeBlock);
}

namespace Rendering {
void drawGrid();
void drawLockedCells();
void drawActiveBlock(const ActiveBlock &activeBlock);
void drawInfoPanel();
void drawNextBlockPreview(Block block, int originX, int originY);
void drawGameOverOverlay();
}

namespace Movement {
bool canBlockGoDown(const ActiveBlock &activeBlock);
bool canBlockGoLeft(const ActiveBlock &activeBlock);
bool canBlockGoRight(const ActiveBlock &activeBlock);
bool canBlockRotate(const ActiveBlock &activeBlock);
void rotateBlock(ActiveBlock &activeBlock);
}

int main(){
    init();
    initCells();

    ActiveBlock activeBlock;
    GameLogic::resetGame(activeBlock);

    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WINDOW_BG_COLOR);
            playGame(activeBlock);
            Rendering::drawGrid();
            Rendering::drawInfoPanel();
            if(gameOver){
                Rendering::drawGameOverOverlay();
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void init(){
    InitWindow(BOARD_WIDTH + INFO_AREA_WIDTH, BOARD_HEIGHT, "TETRIS");
    SetTargetFPS(60);
}

void initCells(){
    for(int row = 0; row < ROWS; row++){
        for(int col = 0; col < COLS; col++){
            cellInfo[row][col] = EMPTY_CELL;
        }
    }
}

namespace GameLogic {

void resetGame(ActiveBlock &activeBlock){
    initCells();
    score = 0;
    clearedLinesTotal = 0;
    level = 1;
    gameOver = false;
    fallDelay = 0.0f;
    nextBlock = chooseRandomBlock();
    spawnBlock(activeBlock);
}

int blockCellX(Block block, Orientation orientation, int index){
    return BLOCK_SHAPES[(int)block][(int)orientation][index][0];
}

int blockCellY(Block block, Orientation orientation, int index){
    return BLOCK_SHAPES[(int)block][(int)orientation][index][1];
}

Color getBlockColor(Block block){
    return BLOCK_COLORS[(int)block];
}

const char *getBlockName(Block block){
    switch(block){
        case BarBlock: return "I";
        case BoxBlock: return "O";
        case TBlock: return "T";
        case LBlock: return "L";
        case JBlock: return "J";
        case ZBlock: return "Z";
        case SBlock: return "S";
    }
    return "?";
}

Block chooseRandomBlock(){
    return (Block)GetRandomValue((int)BarBlock, (int)SBlock);
}

Orientation chooseRandomOrientation(Block block){
    if(block == BoxBlock){
        return Up;
    }
    return (Orientation)GetRandomValue((int)Up, (int)Left);
}

int findMiddle(Block block, Orientation orientation){
    int minX = COLS;
    int maxX = 0;

    for(int i = 0; i < 4; i++){
        int cellX = blockCellX(block, orientation, i);
        minX = std::min(minX, cellX);
        maxX = std::max(maxX, cellX);
    }

    int width = maxX - minX + 1;
    return (COLS - width) / 2 - minX;
}

bool canPlace(Block block, Orientation orientation, int x, int y){
    for(int i = 0; i < 4; i++){
        int boardX = x + blockCellX(block, orientation, i);
        int boardY = y + blockCellY(block, orientation, i);

        if(boardX < 0 || boardX >= COLS || boardY < 0 || boardY >= ROWS){
            return false;
        }

        if(cellInfo[boardY][boardX] != EMPTY_CELL){
            return false;
        }
    }

    return true;
}

void spawnBlock(ActiveBlock &activeBlock){
    activeBlock.block = nextBlock;
    activeBlock.orientation = chooseRandomOrientation(activeBlock.block);
    activeBlock.x = findMiddle(activeBlock.block, activeBlock.orientation);
    activeBlock.y = 0;
    activeBlock.color = getBlockColor(activeBlock.block);
    nextBlock = chooseRandomBlock();

    if(!canPlace(activeBlock.block, activeBlock.orientation, activeBlock.x, activeBlock.y)){
        gameOver = true;
    }
}

void lockActiveBlock(const ActiveBlock &activeBlock){
    int blockValue = (int)activeBlock.block + 1;

    for(int i = 0; i < 4; i++){
        int boardX = activeBlock.x + blockCellX(activeBlock.block, activeBlock.orientation, i);
        int boardY = activeBlock.y + blockCellY(activeBlock.block, activeBlock.orientation, i);
        cellInfo[boardY][boardX] = blockValue;
    }
}

int clearCompletedLines(){
    int cleared = 0;

    for(int row = ROWS - 1; row >= 0; row--){
        bool isFull = true;

        for(int col = 0; col < COLS; col++){
            if(cellInfo[row][col] == EMPTY_CELL){
                isFull = false;
                break;
            }
        }

        if(!isFull){
            continue;
        }

        cleared++;

        for(int moveRow = row; moveRow > 0; moveRow--){
            for(int col = 0; col < COLS; col++){
                cellInfo[moveRow][col] = cellInfo[moveRow - 1][col];
            }
        }

        for(int col = 0; col < COLS; col++){
            cellInfo[0][col] = EMPTY_CELL;
        }

        // Re-check this row index because rows above were shifted down.
        row++;
    }

    return cleared;
}

void applyLineClearScore(int linesCleared){
    if(linesCleared <= 0){
        return;
    }

    const int scoreByClear[5] = {0, 100, 300, 500, 800};
    int appliedLines = std::min(linesCleared, 4);

    score += scoreByClear[appliedLines] * level;
    clearedLinesTotal += linesCleared;
    level = 1 + (clearedLinesTotal / 10);
}

float getCurrentFallDelay(){
    float delay = 0.60f - 0.05f * (level - 1);
    if(delay < 0.08f){
        delay = 0.08f;
    }
    return delay;
}

void finishActivePiece(ActiveBlock &activeBlock){
    lockActiveBlock(activeBlock);
    int linesCleared = clearCompletedLines();
    applyLineClearScore(linesCleared);
    spawnBlock(activeBlock);
}

} // namespace GameLogic

namespace Rendering {

void drawGrid(){
    for(int row = 0; row <= ROWS; row++){
        DrawLine(0, row * CELL_HEIGHT, BOARD_WIDTH, row * CELL_HEIGHT, GRID_LINE_COLOR);
    }

    for(int col = 0; col <= COLS; col++){
        DrawLine(col * CELL_WIDTH, 0, col * CELL_WIDTH, BOARD_HEIGHT, GRID_LINE_COLOR);
    }
}

void drawLockedCells(){
    for(int row = 0; row < ROWS; row++){
        for(int col = 0; col < COLS; col++){
            int cellValue = cellInfo[row][col];
            if(cellValue != EMPTY_CELL){
                Color color = BLOCK_COLORS[cellValue - 1];
                DrawRectangle(col * CELL_WIDTH, row * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, color);
            }
        }
    }
}

void drawActiveBlock(const ActiveBlock &activeBlock){
    for(int i = 0; i < 4; i++){
        int boardX = activeBlock.x + GameLogic::blockCellX(activeBlock.block, activeBlock.orientation, i);
        int boardY = activeBlock.y + GameLogic::blockCellY(activeBlock.block, activeBlock.orientation, i);
        DrawRectangle(boardX * CELL_WIDTH, boardY * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, activeBlock.color);
    }

    // TODO(NEXT): Draw a ghost piece (landing preview) to improve placement planning.
}

void drawNextBlockPreview(Block block, int originX, int originY){
    const int previewCell = 18;
    Orientation previewOrientation = (block == BarBlock) ? Right : Up;
    Color color = GameLogic::getBlockColor(block);

    for(int i = 0; i < 4; i++){
        int px = originX + GameLogic::blockCellX(block, previewOrientation, i) * previewCell;
        int py = originY + GameLogic::blockCellY(block, previewOrientation, i) * previewCell;
        DrawRectangle(px, py, previewCell, previewCell, color);
        DrawRectangleLines(px, py, previewCell, previewCell, Fade(BLACK, 0.5f));
    }
}

void drawInfoPanel(){
    int infoStartX = BOARD_WIDTH;
    int textX = infoStartX + 18;

    DrawRectangle(infoStartX, 0, INFO_AREA_WIDTH, BOARD_HEIGHT, Fade(LIGHTGRAY, 0.2f));
    DrawLine(infoStartX, 0, infoStartX, BOARD_HEIGHT, GRAY);

    DrawText("TETRIS", textX, 20, 36, BLACK);
    DrawText(TextFormat("Score: %d", score), textX, 90, 24, DARKBLUE);
    DrawText(TextFormat("Lines: %d", clearedLinesTotal), textX, 125, 24, DARKBLUE);
    DrawText(TextFormat("Level: %d", level), textX, 160, 24, DARKBLUE);

    DrawText("Next Piece", textX, 210, 24, BLACK);
    drawNextBlockPreview(nextBlock, textX, 245);
    DrawText(TextFormat("Type: %s", GameLogic::getBlockName(nextBlock)), textX, 330, 20, DARKGRAY);

    DrawText("Controls", textX, 385, 22, BLACK);
    DrawText("Left/Right: Move", textX, 415, 18, DARKGRAY);
    DrawText("Up: Rotate", textX, 440, 18, DARKGRAY);
    DrawText("Down: Soft drop", textX, 465, 18, DARKGRAY);
    DrawText("Space: Hard drop", textX, 490, 18, DARKGRAY);
    DrawText("R: Restart", textX, 515, 18, DARKGRAY);
}

void drawGameOverOverlay(){
    DrawRectangle(20, 240, BOARD_WIDTH - 40, 120, Fade(BLACK, 0.7f));
    DrawText("GAME OVER", 48, 263, 36, RAYWHITE);
    DrawText("Press R to restart", 68, 312, 20, RAYWHITE);
}

} // namespace Rendering

namespace Movement {

bool canBlockGoDown(const ActiveBlock &activeBlock){
    return GameLogic::canPlace(activeBlock.block, activeBlock.orientation, activeBlock.x, activeBlock.y + 1);
}

bool canBlockGoLeft(const ActiveBlock &activeBlock){
    return GameLogic::canPlace(activeBlock.block, activeBlock.orientation, activeBlock.x - 1, activeBlock.y);
}

bool canBlockGoRight(const ActiveBlock &activeBlock){
    return GameLogic::canPlace(activeBlock.block, activeBlock.orientation, activeBlock.x + 1, activeBlock.y);
}

bool canBlockRotate(const ActiveBlock &activeBlock){
    Orientation nextOrientation = (Orientation)(((int)activeBlock.orientation + 1) % 4);

    if(GameLogic::canPlace(activeBlock.block, nextOrientation, activeBlock.x, activeBlock.y)){
        return true;
    }

    for(int i = 0; i < KICK_TRIES; i++){
        int kickX = WALL_KICKS[i][0];
        int kickY = WALL_KICKS[i][1];
        if(GameLogic::canPlace(activeBlock.block, nextOrientation, activeBlock.x + kickX, activeBlock.y + kickY)){
            return true;
        }
    }

    return false;
}

void rotateBlock(ActiveBlock &activeBlock){
    Orientation nextOrientation = (Orientation)(((int)activeBlock.orientation + 1) % 4);

    if(GameLogic::canPlace(activeBlock.block, nextOrientation, activeBlock.x, activeBlock.y)){
        activeBlock.orientation = nextOrientation;
        return;
    }

    for(int i = 0; i < KICK_TRIES; i++){
        int kickX = WALL_KICKS[i][0];
        int kickY = WALL_KICKS[i][1];
        if(GameLogic::canPlace(activeBlock.block, nextOrientation, activeBlock.x + kickX, activeBlock.y + kickY)){
            activeBlock.x += kickX;
            activeBlock.y += kickY;
            activeBlock.orientation = nextOrientation;
            return;
        }
    }

    // TODO(NEXT): Replace this basic kick logic with full SRS kick tables for competitive behavior.
}

} // namespace Movement

void playGame(ActiveBlock &activeBlock){
    Rendering::drawLockedCells();

    if(!gameOver){
        Rendering::drawActiveBlock(activeBlock);
    }

    if(gameOver){
        if(IsKeyPressed(KEY_R)){
            GameLogic::resetGame(activeBlock);
        }
        return;
    }

    if(IsKeyPressed(KEY_LEFT) && Movement::canBlockGoLeft(activeBlock)){
        activeBlock.x--;
    }

    if(IsKeyPressed(KEY_RIGHT) && Movement::canBlockGoRight(activeBlock)){
        activeBlock.x++;
    }

    if(IsKeyPressed(KEY_UP) && Movement::canBlockRotate(activeBlock)){
        Movement::rotateBlock(activeBlock);
    }

    if(IsKeyPressed(KEY_SPACE)){
        while(Movement::canBlockGoDown(activeBlock)){
            activeBlock.y++;
        }
        GameLogic::finishActivePiece(activeBlock);
        fallDelay = 0.0f;
        return;
    }

    float targetFallDelay = IsKeyDown(KEY_DOWN) ? 0.05f : GameLogic::getCurrentFallDelay();
    fallDelay += GetFrameTime();

    if(fallDelay >= targetFallDelay){
        if(Movement::canBlockGoDown(activeBlock)){
            activeBlock.y++;
        }
        else{
            GameLogic::finishActivePiece(activeBlock);
        }
        fallDelay = 0.0f;
    }
}
