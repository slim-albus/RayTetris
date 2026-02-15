#include <raylib.h>
#include <algorithm>

enum Block {BarBlock, BoxBlock, TBlock, LBlock, JBlock, ZBlock, SBlock};
enum Orientation {Up, Right, Down, Left};

constexpr int BOARD_WIDTH = 300;
constexpr int BOARD_HEIGHT = 600;
constexpr int ROWS = 20;
constexpr int COLS = 10;
constexpr int CELL_WIDTH = BOARD_WIDTH / COLS;
constexpr int CELL_HEIGHT = BOARD_HEIGHT / ROWS;
constexpr int INFO_AREA_WIDTH = 250;

constexpr int EMPTY_CELL = 0;
constexpr int KICK_TRIES = 6;
constexpr int WALL_KICKS[KICK_TRIES][2] = {
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

struct GameState {
    int cellInfo[ROWS][COLS];
    int score;
    int clearedLinesTotal;
    int level;
    bool gameOver;
    float fallDelay;
    Block nextBlock;
};

void init();
void initCells(GameState &state);
void finishActivePiece(GameState &state, ActiveBlock &activeBlock);
void playGame(GameState &state, ActiveBlock &activeBlock);

namespace Piece {
int blockCellX(Block block, Orientation orientation, int index);
int blockCellY(Block block, Orientation orientation, int index);
Color getBlockColor(Block block);
const char *getBlockName(Block block);
Block chooseRandomBlock();
Orientation chooseRandomOrientation(Block block);
int findMiddle(Block block, Orientation orientation);
} // namespace Piece

namespace Board {
void resetGame(GameState &state, ActiveBlock &activeBlock);
bool canPlace(const GameState &state, Block block, Orientation orientation, int x, int y);
void spawnBlock(GameState &state, ActiveBlock &activeBlock);
void lockActiveBlock(GameState &state, const ActiveBlock &activeBlock);
int clearCompletedLines(GameState &state);
} // namespace Board

namespace Rules {
void applyLineClearScore(GameState &state, int linesCleared);
float getCurrentFallDelay(const GameState &state);
} // namespace Rules

namespace Rendering {
void drawGrid();
void drawLockedCells(const GameState &state);
void drawActiveBlock(const ActiveBlock &activeBlock);
void drawNextBlockPreview(Block block, int originX, int originY);
void drawInfoPanel(const GameState &state);
void drawGameOverOverlay();
} // namespace Rendering

namespace Movement {
bool canBlockGoDown(const GameState &state, const ActiveBlock &activeBlock);
bool canBlockGoLeft(const GameState &state, const ActiveBlock &activeBlock);
bool canBlockGoRight(const GameState &state, const ActiveBlock &activeBlock);
bool canBlockRotate(const GameState &state, const ActiveBlock &activeBlock);
void rotateBlock(const GameState &state, ActiveBlock &activeBlock);
} // namespace Movement

int main(){
    init();

    GameState state;
    ActiveBlock activeBlock;
    Board::resetGame(state, activeBlock);

    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WINDOW_BG_COLOR);
            playGame(state, activeBlock);
            Rendering::drawGrid();
            Rendering::drawInfoPanel(state);
            if(state.gameOver){
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

void initCells(GameState &state){
    for(int row = 0; row < ROWS; row++){
        for(int col = 0; col < COLS; col++){
            state.cellInfo[row][col] = EMPTY_CELL;
        }
    }
}

namespace Piece {

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

} // namespace Piece

namespace Board {

void resetGame(GameState &state, ActiveBlock &activeBlock){
    initCells(state);
    state.score = 0;
    state.clearedLinesTotal = 0;
    state.level = 1;
    state.gameOver = false;
    state.fallDelay = 0.0f;
    state.nextBlock = Piece::chooseRandomBlock();
    spawnBlock(state, activeBlock);
}

bool canPlace(const GameState &state, Block block, Orientation orientation, int x, int y){
    for(int i = 0; i < 4; i++){
        int boardX = x + Piece::blockCellX(block, orientation, i);
        int boardY = y + Piece::blockCellY(block, orientation, i);

        if(boardX < 0 || boardX >= COLS || boardY < 0 || boardY >= ROWS){
            return false;
        }

        if(state.cellInfo[boardY][boardX] != EMPTY_CELL){
            return false;
        }
    }

    return true;
}

void spawnBlock(GameState &state, ActiveBlock &activeBlock){
    activeBlock.block = state.nextBlock;
    activeBlock.orientation = Piece::chooseRandomOrientation(activeBlock.block);
    activeBlock.x = Piece::findMiddle(activeBlock.block, activeBlock.orientation);
    activeBlock.y = 0;
    activeBlock.color = Piece::getBlockColor(activeBlock.block);
    state.nextBlock = Piece::chooseRandomBlock();

    if(!canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x, activeBlock.y)){
        state.gameOver = true;
    }
}

void lockActiveBlock(GameState &state, const ActiveBlock &activeBlock){
    int blockValue = (int)activeBlock.block + 1;

    for(int i = 0; i < 4; i++){
        int boardX = activeBlock.x + Piece::blockCellX(activeBlock.block, activeBlock.orientation, i);
        int boardY = activeBlock.y + Piece::blockCellY(activeBlock.block, activeBlock.orientation, i);
        state.cellInfo[boardY][boardX] = blockValue;
    }
}

int clearCompletedLines(GameState &state){
    int cleared = 0;

    for(int row = ROWS - 1; row >= 0; row--){
        bool isFull = true;

        for(int col = 0; col < COLS; col++){
            if(state.cellInfo[row][col] == EMPTY_CELL){
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
                state.cellInfo[moveRow][col] = state.cellInfo[moveRow - 1][col];
            }
        }

        for(int col = 0; col < COLS; col++){
            state.cellInfo[0][col] = EMPTY_CELL;
        }

        row++;
    }

    return cleared;
}

} // namespace Board

namespace Rules {

void applyLineClearScore(GameState &state, int linesCleared){
    if(linesCleared <= 0){
        return;
    }

    const int scoreByClear[5] = {0, 100, 300, 500, 800};
    int appliedLines = std::min(linesCleared, 4);

    state.score += scoreByClear[appliedLines] * state.level;
    state.clearedLinesTotal += linesCleared;
    state.level = 1 + (state.clearedLinesTotal / 10);
}

float getCurrentFallDelay(const GameState &state){
    float delay = 0.60f - 0.05f * (state.level - 1);
    if(delay < 0.08f){
        delay = 0.08f;
    }
    return delay;
}

} // namespace Rules

namespace Rendering {

void drawGrid(){
    for(int row = 0; row <= ROWS; row++){
        DrawLine(0, row * CELL_HEIGHT, BOARD_WIDTH, row * CELL_HEIGHT, GRID_LINE_COLOR);
    }

    for(int col = 0; col <= COLS; col++){
        DrawLine(col * CELL_WIDTH, 0, col * CELL_WIDTH, BOARD_HEIGHT, GRID_LINE_COLOR);
    }
}

void drawLockedCells(const GameState &state){
    for(int row = 0; row < ROWS; row++){
        for(int col = 0; col < COLS; col++){
            int cellValue = state.cellInfo[row][col];
            if(cellValue != EMPTY_CELL){
                Color color = BLOCK_COLORS[cellValue - 1];
                DrawRectangle(col * CELL_WIDTH, row * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, color);
            }
        }
    }
}

void drawActiveBlock(const ActiveBlock &activeBlock){
    for(int i = 0; i < 4; i++){
        int boardX = activeBlock.x + Piece::blockCellX(activeBlock.block, activeBlock.orientation, i);
        int boardY = activeBlock.y + Piece::blockCellY(activeBlock.block, activeBlock.orientation, i);
        DrawRectangle(boardX * CELL_WIDTH, boardY * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, activeBlock.color);
    }

    // TODO: Draw a ghost piece (landing preview) to improve placement planning.
}

void drawNextBlockPreview(Block block, int originX, int originY){
    const int previewCell = 18;
    Orientation previewOrientation = (block == BarBlock) ? Right : Up;
    Color color = Piece::getBlockColor(block);

    for(int i = 0; i < 4; i++){
        int px = originX + Piece::blockCellX(block, previewOrientation, i) * previewCell;
        int py = originY + Piece::blockCellY(block, previewOrientation, i) * previewCell;
        DrawRectangle(px, py, previewCell, previewCell, color);
        DrawRectangleLines(px, py, previewCell, previewCell, Fade(BLACK, 0.5f));
    }
}

void drawInfoPanel(const GameState &state){
    int infoStartX = BOARD_WIDTH;
    int textX = infoStartX + 18;

    DrawRectangle(infoStartX, 0, INFO_AREA_WIDTH, BOARD_HEIGHT, Fade(LIGHTGRAY, 0.2f));
    DrawLine(infoStartX, 0, infoStartX, BOARD_HEIGHT, GRAY);

    DrawText("TETRIS", textX, 20, 36, BLACK);
    DrawText(TextFormat("Score: %d", state.score), textX, 90, 24, DARKBLUE);
    DrawText(TextFormat("Lines: %d", state.clearedLinesTotal), textX, 125, 24, DARKBLUE);
    DrawText(TextFormat("Level: %d", state.level), textX, 160, 24, DARKBLUE);

    DrawText("Next Piece", textX, 210, 24, BLACK);
    drawNextBlockPreview(state.nextBlock, textX, 245);
    DrawText(TextFormat("Type: %s", Piece::getBlockName(state.nextBlock)), textX, 330, 20, DARKGRAY);

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

bool canBlockGoDown(const GameState &state, const ActiveBlock &activeBlock){
    return Board::canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x, activeBlock.y + 1);
}

bool canBlockGoLeft(const GameState &state, const ActiveBlock &activeBlock){
    return Board::canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x - 1, activeBlock.y);
}

bool canBlockGoRight(const GameState &state, const ActiveBlock &activeBlock){
    return Board::canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x + 1, activeBlock.y);
}

bool canBlockRotate(const GameState &state, const ActiveBlock &activeBlock){
    Orientation nextOrientation = (Orientation)(((int)activeBlock.orientation + 1) % 4);

    if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x, activeBlock.y)){
        return true;
    }

    for(int i = 0; i < KICK_TRIES; i++){
        int kickX = WALL_KICKS[i][0];
        int kickY = WALL_KICKS[i][1];
        if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x + kickX, activeBlock.y + kickY)){
            return true;
        }
    }

    return false;
}

void rotateBlock(const GameState &state, ActiveBlock &activeBlock){
    Orientation nextOrientation = (Orientation)(((int)activeBlock.orientation + 1) % 4);

    if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x, activeBlock.y)){
        activeBlock.orientation = nextOrientation;
        return;
    }

    for(int i = 0; i < KICK_TRIES; i++){
        int kickX = WALL_KICKS[i][0];
        int kickY = WALL_KICKS[i][1];
        if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x + kickX, activeBlock.y + kickY)){
            activeBlock.x += kickX;
            activeBlock.y += kickY;
            activeBlock.orientation = nextOrientation;
            return;
        }
    }

    // TODO: Replace this basic kick logic with full SRS kick tables for competitive behavior.
}

} // namespace Movement

void finishActivePiece(GameState &state, ActiveBlock &activeBlock){
    Board::lockActiveBlock(state, activeBlock);
    int linesCleared = Board::clearCompletedLines(state);
    Rules::applyLineClearScore(state, linesCleared);
    Board::spawnBlock(state, activeBlock);
}

void playGame(GameState &state, ActiveBlock &activeBlock){
    Rendering::drawLockedCells(state);

    if(!state.gameOver){
        Rendering::drawActiveBlock(activeBlock);
    }

    if(state.gameOver){
        if(IsKeyPressed(KEY_R)){
            Board::resetGame(state, activeBlock);
        }
        return;
    }

    if(IsKeyPressed(KEY_LEFT) && Movement::canBlockGoLeft(state, activeBlock)){
        activeBlock.x--;
    }

    if(IsKeyPressed(KEY_RIGHT) && Movement::canBlockGoRight(state, activeBlock)){
        activeBlock.x++;
    }

    if(IsKeyPressed(KEY_UP) && Movement::canBlockRotate(state, activeBlock)){
        Movement::rotateBlock(state, activeBlock);
    }

    if(IsKeyPressed(KEY_SPACE)){
        while(Movement::canBlockGoDown(state, activeBlock)){
            activeBlock.y++;
        }
        finishActivePiece(state, activeBlock);
        state.fallDelay = 0.0f;
        return;
    }

    float targetFallDelay = IsKeyDown(KEY_DOWN) ? 0.05f : Rules::getCurrentFallDelay(state);
    state.fallDelay += GetFrameTime();

    if(state.fallDelay >= targetFallDelay){
        if(Movement::canBlockGoDown(state, activeBlock)){
            activeBlock.y++;
        }
        else{
            finishActivePiece(state, activeBlock);
        }
        state.fallDelay = 0.0f;
    }
}
