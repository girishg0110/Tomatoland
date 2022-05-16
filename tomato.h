// Dimensions for the drawn grid (should be GRIDSIZE * texture dimensions)
#define GRID_DRAW_WIDTH 640
#define GRID_DRAW_HEIGHT 640

#define WINDOW_WIDTH GRID_DRAW_WIDTH
#define WINDOW_HEIGHT (HEADER_HEIGHT + GRID_DRAW_HEIGHT)

// Header displays current score
#define HEADER_HEIGHT 50

// Number of cells vertically/horizontally in the grid
#define GRIDSIZE 10

// Client Macros
#define BOARD_STATE_SIZE sizeof(BoardState)
#define NUM_PLAYERS 4
#define POSITION_SIZE sizeof(Position)

typedef struct
{
    int x;
    int y;
} Position;

typedef enum
{
    TILE_GRASS,
    TILE_TOMATO
} TILETYPE;

typedef struct {
   TILETYPE grid[GRIDSIZE][GRIDSIZE]; // where grass and tomato tiles are on grid
   Position pos[NUM_PLAYERS]; // all player positions
   int scores[NUM_PLAYERS]; // all player scores
   int level; // current level
} BoardState;

void displayGrid(TILETYPE grid[][GRIDSIZE]) {
   for (int i = 0; i < GRIDSIZE; i++) {
      for (int j = 0; j < GRIDSIZE; j++) {
         printf((grid[i][j] == TILE_GRASS) ? "G " : "T ");
      }
      printf("\n");
   } 
}

void displayBoard(BoardState board) {
   printf("--Level %d--\n", board.level);
   displayGrid(board.grid);
   for (int i = 0; i < NUM_PLAYERS; i++) {
      printf("%d: (%d, %d) with score %d\n", i, board.pos[i].x, board.pos[i].y, board.scores[i]);
   }
   printf("--END OF DISPLAY--\n");
}

