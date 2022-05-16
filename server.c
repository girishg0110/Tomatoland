#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "csapp.h"
#include "tomato.h"

#define TOMATO_PERCENT 0.1

BoardState board;
int numTomatoes;
int pCount = 0;

sem_t posSem;

// get a random value in the range [0, 1]
double rand01()
{
    return (double) rand() / (double) RAND_MAX;
}

void initGrid()
{
    for (int i = 0; i < GRIDSIZE; i++) {
        for (int j = 0; j < GRIDSIZE; j++) {
            double r = rand01();
            if (r < TOMATO_PERCENT) {
                board.grid[i][j] = TILE_TOMATO;
                numTomatoes++;
            }
            else
                board.grid[i][j] = TILE_GRASS;
        }
    }

   for (int i = 0; i < NUM_PLAYERS; i++) {
      Position pos = board.pos[i];
      if (pos.x == -1) continue;
      if (board.grid[pos.x][pos.y] == TILE_TOMATO) {
         board.grid[pos.x][pos.y] = TILE_GRASS;
         numTomatoes--;
      }
   }

    // ensure grid isn't empty
    while (numTomatoes == 0)
        initGrid();
}

/* Safe Getter Methods */


void resetPlayer(int id) {
   board.pos[id].x = -1;
   board.pos[id].y = -1;
  
   board.scores[id] = -1;
}

void initGame() {
   initGrid(); 

   for (int i = 0; i < NUM_PLAYERS; i++) {
      resetPlayer(i);
   }   

   board.level = 1;
   pCount = 0;

   Sem_init(&posSem, 0, 1);

   //displayBoard(board);
}

bool playerCollision(int x, int y) {
   for (int i = 0; i < NUM_PLAYERS; i++) {
      if ((board.pos[i].x == x) && (board.pos[i].y == y)) return true;
   }
   return false;
}

bool tomatoCollision(int x, int y) {
   return board.grid[x][y] == TILE_TOMATO;
}

void updatePlayer(int id, int x, int y) {
   Position playerPosition = board.pos[id];
  
   if ((x < 0) || (y < 0) || (x > GRIDSIZE - 1) || (y > GRIDSIZE - 1)) return;
   for (int i = 0; i < NUM_PLAYERS; i++) {
      if ((board.pos[i].x == x) && (board.pos[i].y == y)) { return; }
   }
   if (!(abs(playerPosition.x - x) == 1 && abs(playerPosition.y - y) == 0) &&
        !(abs(playerPosition.x - x) == 0 && abs(playerPosition.y - y) == 1)) {
        return;
    }

    board.pos[id].x = x;
    board.pos[id].y = y;

    if (board.grid[x][y] == TILE_TOMATO) {
        board.grid[x][y] = TILE_GRASS;
        board.scores[id]++;
        numTomatoes--;
        if (numTomatoes == 0) {
            board.level++;
            initGrid();
        }
    }

   return;
} 

void placePlayer(int id) { 
   // Generate safe player position
   int x, y;
   do {
      x = rand() % GRIDSIZE;
      y = rand() % GRIDSIZE;
   } while (tomatoCollision(x, y) || playerCollision(x, y));
   board.pos[id].x = x;
   board.pos[id].y = y;
}

void addPlayer(int id) {
   placePlayer(id);

   // Initialize score and mark player as active
   board.scores[id] = 0;

   pCount++;
}

void* thread(void *vargp) {
   int connfd = *((int*) vargp);    
   Pthread_detach(pthread_self());
   Free(vargp);

   int playerID = pCount;
   if (playerID == NUM_PLAYERS) { Close(connfd); return NULL; }   
 
   P(&posSem); 
   addPlayer(playerID);
   V(&posSem);
 
   size_t n = -1;
   void* buf[POSITION_SIZE];
   rio_t rio;
   Position* newPos;
 
   Rio_readinitb(&rio, connfd); 
   Rio_writen(connfd, (void*) &board, BOARD_STATE_SIZE);
   while (1) {       
      // Wait for position update
      n = Rio_readnb(&rio, buf, POSITION_SIZE);
      if (n == 0) break;
      
      newPos = (Position*) buf;
      P(&posSem);
      bool changedPos = ((newPos->x != board.pos[playerID].x) || 
                           (newPos->y != board.pos[playerID].y));
      if (changedPos) {
         updatePlayer(playerID, newPos->x, newPos->y);
      }
      V(&posSem);

      // Send board back
      Rio_writen(connfd, (void*) &board, BOARD_STATE_SIZE);
   }
   
   P(&posSem);
   resetPlayer(playerID);
   V(&posSem);

   Close(connfd);
   return NULL; 
}

int main(int argc, char *argv[]) 
{
    srand(time(NULL));
     
    initGame();

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	   fprintf(stderr, "usage: %s <port>\n", argv[0]);
	   exit(0);
    }
    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen=sizeof(struct sockaddr_storage);
	     connfdp = Malloc(sizeof(int)); //line:conc:echoservert:beginmalloc
	     *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); //line:conc:echoservert:endmalloc
        Pthread_create(&tid, NULL, thread, connfdp);
    }
}

