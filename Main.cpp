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
const int BOARD_PADDING = 24;
const int BOARD_GAP = 24;
const int BOARD_ORIGIN_X = BOARD_PADDING;
const int BOARD_ORIGIN_Y = BOARD_PADDING;
const int INFO_ORIGIN_X = BOARD_ORIGIN_X + BOARD_WIDTH + BOARD_GAP;
const int INFO_ORIGIN_Y = BOARD_ORIGIN_Y;
const int SCREEN_WIDTH = INFO_ORIGIN_X + INFO_AREA_WIDTH + BOARD_PADDING;
const int SCREEN_HEIGHT = BOARD_HEIGHT + (BOARD_PADDING * 2);

const int EMPTY_CELL = 0;
const float DEFAULT_FALL_DELAY = 0.45f;
const int KICK_TRIES = 6;
const int WALL_KICKS[KICK_TRIES][2] = {
    {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1}, {0, 1}
};

const Color WINDOW_BG_COLOR = {18, 20, 26, 255};
const Color BOARD_BG_COLOR = {28, 31, 40, 255};
const Color GRID_LINE_COLOR = {57, 63, 82, 255};
const Color PANEL_BG_COLOR = {33, 37, 50, 255};
const Color PANEL_BORDER_COLOR = {73, 81, 107, 255};
const Color PANEL_TITLE_COLOR = {232, 238, 255, 255};
const Color PANEL_PRIMARY_TEXT_COLOR = {188, 199, 231, 255};
const Color PANEL_MUTED_TEXT_COLOR = {141, 153, 189, 255};

struct BlockData {
    Color color;
    int cells[4][4][2];
};

// Single source of truth for each block: color and all rotations.
const BlockData BLOCK_DATA[7] = {
    {
        {0, 194, 255, 255},
        {
            {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
            {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
            {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
            {{0, 2}, {1, 2}, {2, 2}, {3, 2}}
        }
    },
    {
        {255, 205, 64, 255},
        {
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}}
        }
    },
    {
        {188, 124, 255, 255},
        {
            {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
            {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
            {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
            {{1, 0}, {0, 1}, {1, 1}, {1, 2}}
        }
    },
    {
        {255, 156, 77, 255},
        {
            {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
            {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
            {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
            {{0, 0}, {1, 0}, {1, 1}, {1, 2}}
        }
    },
    {
        {86, 128, 255, 255},
        {
            {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
            {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
            {{1, 0}, {1, 1}, {0, 2}, {1, 2}}
        }
    },
    {
        {255, 95, 124, 255},
        {
            {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
            {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
            {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
            {{1, 0}, {0, 1}, {1, 1}, {0, 2}}
        }
    },
    {
        {88, 224, 147, 255},
        {
            {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
            {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
            {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
            {{0, 0}, {0, 1}, {1, 1}, {1, 2}}
        }
    }
};
const Color BLOCK_OUTLINE_COLOR = {8, 10, 14, 230};

// Represents the currently falling tetromino.
struct ActiveBlock {
    Block block;
    Orientation orientation;
    int x;
    int y;
};

// Holds persistent gameplay state shared across frames.
struct GameState {
    int cellInfo[ROWS][COLS];
    int score;
    int clearedLinesTotal;
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

// Program entry: initialize systems and run the frame loop until window close.
int main(){
    init();

    GameState state;
    ActiveBlock activeBlock;
    // Initialize gameplay state before entering the frame loop.
    Board::resetGame(state, activeBlock);

    // Main frame loop: update + draw once per iteration.
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WINDOW_BG_COLOR);
            Rendering::drawGrid();
            playGame(state, activeBlock);
            Rendering::drawInfoPanel(state);
            // Overlay is rendered only when the current round is over.
            if(state.gameOver){
                Rendering::drawGameOverOverlay();
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// Creates the raylib window and sets the target FPS.
void init(){
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "TETRIS");
    SetTargetFPS(60);
}

// Clears the board so every cell is EMPTY_CELL.
void initCells(GameState &state){
    for(int row = 0; row < ROWS; row++){
        for(int col = 0; col < COLS; col++){
            state.cellInfo[row][col] = EMPTY_CELL;
        }
    }
}

namespace Piece {

// Returns the X offset of one of the 4 cells of a piece for a given orientation.
int blockCellX(Block block, Orientation orientation, int index){
    return BLOCK_DATA[(int)block].cells[(int)orientation][index][0];
}

// Returns the Y offset of one of the 4 cells of a piece for a given orientation.
int blockCellY(Block block, Orientation orientation, int index){
    return BLOCK_DATA[(int)block].cells[(int)orientation][index][1];
}

// Maps a block type to its display color.
Color getBlockColor(Block block){
    return BLOCK_DATA[(int)block].color;
}

// Chooses a random tetromino type (I/O/T/L/J/Z/S).
Block chooseRandomBlock(){
    return (Block)GetRandomValue((int)BarBlock, (int)SBlock);
}

// Chooses a random starting orientation (O is fixed because all rotations are identical).
Orientation chooseRandomOrientation(Block block){
    if(block == BoxBlock){
        return Up;
    }
    return (Orientation)GetRandomValue((int)Up, (int)Left);
}

// Computes spawn X so the piece appears centered based on its current rotated width.
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

// Resets score/board and spawns the first active piece for a new round.
void resetGame(GameState &state, ActiveBlock &activeBlock){
    initCells(state);
    state.score = 0;
    state.clearedLinesTotal = 0;
    state.gameOver = false;
    state.fallDelay = 0.0f;
    // Roll a new queued piece for the following spawn.
    state.nextBlock = Piece::chooseRandomBlock();
    spawnBlock(state, activeBlock);
}

// Core collision function: validates bounds and overlap for all 4 cells.
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

// Promotes nextBlock into active play, then rolls a new nextBlock.
// Also checks spawn collision to detect game-over immediately.
void spawnBlock(GameState &state, ActiveBlock &activeBlock){
    activeBlock.block = state.nextBlock;
    activeBlock.orientation = Piece::chooseRandomOrientation(activeBlock.block);
    activeBlock.x = Piece::findMiddle(activeBlock.block, activeBlock.orientation);
    activeBlock.y = 0;
    // Roll a new queued piece for the following spawn.
    state.nextBlock = Piece::chooseRandomBlock();

    // If spawn location is blocked, no legal spawn exists -> game over.
    if(!canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x, activeBlock.y)){
        state.gameOver = true;
    }
}

// Writes active piece cells into the board grid as locked blocks.
void lockActiveBlock(GameState &state, const ActiveBlock &activeBlock){
    int blockValue = (int)activeBlock.block + 1;

    for(int i = 0; i < 4; i++){
        int boardX = activeBlock.x + Piece::blockCellX(activeBlock.block, activeBlock.orientation, i);
        int boardY = activeBlock.y + Piece::blockCellY(activeBlock.block, activeBlock.orientation, i);
        state.cellInfo[boardY][boardX] = blockValue;
    }
}

// Clears every full row, shifts rows above down, and returns number of cleared lines.
int clearCompletedLines(GameState &state){
    int cleared = 0;

    // Scan bottom-up so shifted rows can be rechecked after a line clear.
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

// Awards a flat +100 for each cleared line.
void applyLineClearScore(GameState &state, int linesCleared){
    if(linesCleared <= 0){
        return;
    }

    state.score += linesCleared * 100;
    state.clearedLinesTotal += linesCleared;
}

} // namespace Rules

namespace Rendering {

// Draws board guide lines (visual only).
void drawGrid(){
    DrawRectangle(BOARD_ORIGIN_X, BOARD_ORIGIN_Y, BOARD_WIDTH, BOARD_HEIGHT, BOARD_BG_COLOR);

    for(int row = 0; row <= ROWS; row++){
        int y = BOARD_ORIGIN_Y + row * CELL_HEIGHT;
        DrawLine(BOARD_ORIGIN_X, y, BOARD_ORIGIN_X + BOARD_WIDTH, y, GRID_LINE_COLOR);
    }

    for(int col = 0; col <= COLS; col++){
        int x = BOARD_ORIGIN_X + col * CELL_WIDTH;
        DrawLine(x, BOARD_ORIGIN_Y, x, BOARD_ORIGIN_Y + BOARD_HEIGHT, GRID_LINE_COLOR);
    }

    DrawRectangleLinesEx(
        Rectangle{(float)BOARD_ORIGIN_X, (float)BOARD_ORIGIN_Y, (float)BOARD_WIDTH, (float)BOARD_HEIGHT},
        2.0f,
        PANEL_BORDER_COLOR
    );
}

// Draws all locked board cells from state.cellInfo.
void drawLockedCells(const GameState &state){
    for(int row = 0; row < ROWS; row++){
        for(int col = 0; col < COLS; col++){
            int cellValue = state.cellInfo[row][col];
            if(cellValue != EMPTY_CELL){
                Color color = Piece::getBlockColor((Block)(cellValue - 1));
                int px = BOARD_ORIGIN_X + col * CELL_WIDTH;
                int py = BOARD_ORIGIN_Y + row * CELL_HEIGHT;
                DrawRectangle(px, py, CELL_WIDTH, CELL_HEIGHT, color);
                DrawRectangleLines(px, py, CELL_WIDTH, CELL_HEIGHT, BLOCK_OUTLINE_COLOR);
            }
        }
    }
}

// Draws the currently falling piece using shape offsets from BLOCK_DATA.
void drawActiveBlock(const ActiveBlock &activeBlock){
    Color color = Piece::getBlockColor(activeBlock.block);
    for(int i = 0; i < 4; i++){
        int boardX = activeBlock.x + Piece::blockCellX(activeBlock.block, activeBlock.orientation, i);
        int boardY = activeBlock.y + Piece::blockCellY(activeBlock.block, activeBlock.orientation, i);
        int px = BOARD_ORIGIN_X + boardX * CELL_WIDTH;
        int py = BOARD_ORIGIN_Y + boardY * CELL_HEIGHT;
        DrawRectangle(px, py, CELL_WIDTH, CELL_HEIGHT, color);
        DrawRectangleLines(px, py, CELL_WIDTH, CELL_HEIGHT, BLOCK_OUTLINE_COLOR);
    }
}


// Draws a compact preview of the queued next piece in the side panel.
void drawNextBlockPreview(Block block, int originX, int originY){
    const int previewCell = 18;
    Orientation previewOrientation = (block == BarBlock) ? Right : Up;
    Color color = Piece::getBlockColor(block);

    for(int i = 0; i < 4; i++){
        int px = originX + Piece::blockCellX(block, previewOrientation, i) * previewCell;
        int py = originY + Piece::blockCellY(block, previewOrientation, i) * previewCell;
        DrawRectangle(px, py, previewCell, previewCell, color);
        DrawRectangleLines(px, py, previewCell, previewCell, BLOCK_OUTLINE_COLOR);
    }
}

// Draws HUD data: score, lines, next piece, and control guide.
void drawInfoPanel(const GameState &state){
    int infoStartX = INFO_ORIGIN_X;
    int infoStartY = INFO_ORIGIN_Y;
    int textX = infoStartX + 18;

    DrawRectangle(infoStartX, infoStartY, INFO_AREA_WIDTH, BOARD_HEIGHT, PANEL_BG_COLOR);
    DrawRectangleLinesEx(
        Rectangle{(float)infoStartX, (float)infoStartY, (float)INFO_AREA_WIDTH, (float)BOARD_HEIGHT},
        2.0f,
        PANEL_BORDER_COLOR
    );

    DrawText("TETRIS", textX, infoStartY + 20, 45, PANEL_TITLE_COLOR);
    DrawText(TextFormat("Score: %d", state.score), textX, infoStartY + 90, 24, PANEL_PRIMARY_TEXT_COLOR);
    DrawText(TextFormat("Lines: %d", state.clearedLinesTotal), textX, infoStartY + 125, 24, PANEL_PRIMARY_TEXT_COLOR);

    DrawText("Next Piece", textX, infoStartY + 210, 24, PANEL_TITLE_COLOR);
    drawNextBlockPreview(state.nextBlock, textX, infoStartY + 245);

    DrawText("Controls", textX, infoStartY + 385, 22, PANEL_TITLE_COLOR);
    DrawText("Left/Right: Move", textX, infoStartY + 415, 18, PANEL_MUTED_TEXT_COLOR);
    DrawText("Up: Rotate", textX, infoStartY + 440, 18, PANEL_MUTED_TEXT_COLOR);
    DrawText("Down: Soft drop", textX, infoStartY + 465, 18, PANEL_MUTED_TEXT_COLOR);
    DrawText("Space: Hard drop", textX, infoStartY + 490, 18, PANEL_MUTED_TEXT_COLOR);
    DrawText("R: Restart", textX, infoStartY + 515, 18, PANEL_MUTED_TEXT_COLOR);
}

// Draws the game-over overlay above the board area.
void drawGameOverOverlay(){
    int overlayX = BOARD_ORIGIN_X + 20;
    int overlayY = BOARD_ORIGIN_Y + 240;
    DrawRectangle(overlayX, overlayY, BOARD_WIDTH - 40, 120, Fade(BLACK, 0.7f));
    DrawText("GAME OVER", overlayX + 28, overlayY + 23, 36, RAYWHITE);
    DrawText("Press R to restart", overlayX + 48, overlayY + 72, 20, RAYWHITE);
}

} // namespace Rendering

namespace Movement {

// Checks if the active piece can move one row down.
bool canBlockGoDown(const GameState &state, const ActiveBlock &activeBlock){
    return Board::canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x, activeBlock.y + 1);
}

// Checks if the active piece can move one column left.
bool canBlockGoLeft(const GameState &state, const ActiveBlock &activeBlock){
    return Board::canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x - 1, activeBlock.y);
}

// Checks if the active piece can move one column right.
bool canBlockGoRight(const GameState &state, const ActiveBlock &activeBlock){
    return Board::canPlace(state, activeBlock.block, activeBlock.orientation, activeBlock.x + 1, activeBlock.y);
}

// Checks clockwise rotation validity with simple wall-kick attempts.
bool canBlockRotate(const GameState &state, const ActiveBlock &activeBlock){
    Orientation nextOrientation = (Orientation)(((int)activeBlock.orientation + 1) % 4);

    if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x, activeBlock.y)){
        return true;
    }

    // Try a small list of kick offsets so rotation can work near walls/stack.
    for(int i = 0; i < KICK_TRIES; i++){
        int kickX = WALL_KICKS[i][0];
        int kickY = WALL_KICKS[i][1];
        if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x + kickX, activeBlock.y + kickY)){
            return true;
        }
    }

    return false;
}

// Applies clockwise rotation and first successful kick offset.
void rotateBlock(const GameState &state, ActiveBlock &activeBlock){
    Orientation nextOrientation = (Orientation)(((int)activeBlock.orientation + 1) % 4);

    if(Board::canPlace(state, activeBlock.block, nextOrientation, activeBlock.x, activeBlock.y)){
        activeBlock.orientation = nextOrientation;
        return;
    }

    // Try a small list of kick offsets so rotation can work near walls/stack.
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
}

} // namespace Movement

// Finalizes a landed piece: lock -> clear lines -> score update -> spawn next.
void finishActivePiece(GameState &state, ActiveBlock &activeBlock){
    Board::lockActiveBlock(state, activeBlock);
    int linesCleared = Board::clearCompletedLines(state);
    Rules::applyLineClearScore(state, linesCleared);
    Board::spawnBlock(state, activeBlock);
}

// Per-frame gameplay flow: draw, read input, apply gravity, and resolve piece state.
void playGame(GameState &state, ActiveBlock &activeBlock){
    Rendering::drawLockedCells(state);

    if(!state.gameOver){
        Rendering::drawActiveBlock(activeBlock);
    }

    // In game-over state, only restart input is handled.
    if(state.gameOver){
        if(IsKeyPressed(KEY_R)){
            // Restart immediately into a fresh round.
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

    // Hard drop: move down until blocked, then immediately finalize piece.
    if(IsKeyPressed(KEY_SPACE)){
        while(Movement::canBlockGoDown(state, activeBlock)){
            activeBlock.y++;
        }
        finishActivePiece(state, activeBlock);
        state.fallDelay = 0.0f;
        return;
    }

    // Soft drop temporarily overrides gravity speed while Down is held.
    float targetFallDelay = IsKeyDown(KEY_DOWN) ? 0.05f : DEFAULT_FALL_DELAY;
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

