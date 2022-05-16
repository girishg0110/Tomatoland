#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "csapp.h"
#include "tomato.h"

BoardState board;

bool shouldExit = false;

TTF_Font* font;
TTF_Font* levFont;

int clientfd;
rio_t rio;
int playerID = NUM_PLAYERS - 1;

/* Client */

void moveTo(int x, int y)
{
   Position p;
   p.x = x;
   p.y = y;
   
   Rio_writen(clientfd, (void*) (&p), POSITION_SIZE);
}

void getBoardState() {
   void* buf[BOARD_STATE_SIZE];
   ssize_t boardSize = Rio_readnb(&rio, buf, BOARD_STATE_SIZE);
   if (boardSize == 0) return;   

   BoardState* newBoard = (BoardState*) buf;

   for (int i = 0; i < GRIDSIZE; i++) {
      for (int j = 0; j < GRIDSIZE; j++) {
         board.grid[i][j] = newBoard->grid[i][j];
      }
   }
   for (int i = 0; i < NUM_PLAYERS; i++) {
      board.pos[i].x = newBoard->pos[i].x;
      board.pos[i].y = newBoard->pos[i].y;
      board.scores[i] = newBoard->scores[i];
   }
   board.level = newBoard->level; 
}

void connectToServer(char* host, char* port) {
   clientfd = Open_clientfd(host, port);
   Rio_readinitb(&rio, clientfd);

   getBoardState();
   for (int i = 0; i < NUM_PLAYERS; i++) {
      if (board.scores[i] == -1) {
         playerID = i-1;
         break;
      }
   }
}
 
double rand01()
{
    return (double) rand() / (double) RAND_MAX;
}

void initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int rv = IMG_Init(IMG_INIT_PNG);
    if ((rv & IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "Error initializing IMG: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "Error initializing TTF: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
}

void handleKeyDown(SDL_KeyboardEvent* event)
{
    Position playerPosition = board.pos[playerID];
    if (event->repeat)
        { moveTo(playerPosition.x, playerPosition.y); return; }

    if (event->keysym.scancode == SDL_SCANCODE_Q || event->keysym.scancode == SDL_SCANCODE_ESCAPE)
        { shouldExit = true; moveTo(playerPosition.x, playerPosition.y); }

    else if (event->keysym.scancode == SDL_SCANCODE_UP || event->keysym.scancode == SDL_SCANCODE_W)
        moveTo(playerPosition.x, playerPosition.y - 1);

    else if (event->keysym.scancode == SDL_SCANCODE_DOWN || event->keysym.scancode == SDL_SCANCODE_S)
        moveTo(playerPosition.x, playerPosition.y + 1);

    else if (event->keysym.scancode == SDL_SCANCODE_LEFT || event->keysym.scancode == SDL_SCANCODE_A)
        moveTo(playerPosition.x - 1, playerPosition.y);

    else if (event->keysym.scancode == SDL_SCANCODE_RIGHT || event->keysym.scancode == SDL_SCANCODE_D)
        moveTo(playerPosition.x + 1, playerPosition.y);
    else
        moveTo(playerPosition.x, playerPosition.y);

}

void processInputs()
{
   SDL_Event event;
   bool keyPressed = false;

	while (SDL_PollEvent(&event)) {
	   switch (event.type) {
         case SDL_KEYDOWN:
            keyPressed = true;
            handleKeyDown(&event.key);
				break;
   
   	   case SDL_QUIT:
   			shouldExit = true;
            break;
	
   		default:
				break;
		}
	}
   if (!keyPressed) {
      moveTo(board.pos[playerID].x, board.pos[playerID].y);
   }
}
  
void drawGrid(SDL_Renderer* renderer, SDL_Texture* grassTexture, SDL_Texture* tomatoTexture, SDL_Texture* playerTexture)
{
    SDL_Rect dest;
    for (int i = 0; i < GRIDSIZE; i++) {
        for (int j = 0; j < GRIDSIZE; j++) {
            dest.x = 64 * i;
            dest.y = 64 * j + HEADER_HEIGHT;
            SDL_Texture* texture = (board.grid[i][j] == TILE_GRASS) ? 
                                       grassTexture : tomatoTexture;
            SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, texture, NULL, &dest);
        }
    }

    for (int i = 0; i < NUM_PLAYERS; i++) {
      if (board.scores[i] == -1) continue;
      Position playerPosition = board.pos[i];
      dest.x = 64 * playerPosition.x;
      dest.y = 64 * playerPosition.y + HEADER_HEIGHT;
      SDL_QueryTexture(playerTexture, NULL, NULL, &dest.w, &dest.h);
      SDL_RenderCopy(renderer, playerTexture, NULL, &dest);
    }
}

void drawUI(SDL_Renderer* renderer)
{
   int activePlayers = 0;
   for (int id = 0; id < NUM_PLAYERS; id++) {
      if (board.scores[id] != -1) activePlayers++;
   }

    // largest score/level supported is 2147483647
    char** scoreStr = (char**) malloc(sizeof(char*) * activePlayers);
    for (int k = 0; k < activePlayers; k++) scoreStr[k] = (char*) malloc(sizeof(char) * 18);
    char levelStr[18];

   int k = 0; 
   int currIdx = -1;
   for (int id = 0; id < NUM_PLAYERS; id++) {
      if (board.scores[id] != -1) {
         if (id == playerID) currIdx = k;
         sprintf(scoreStr[k], "P%d Score: %d", id, board.scores[id]);
         k++;
      }
    }
    sprintf(levelStr, "Level: %d", board.level);

    SDL_Color white = {255, 255, 255};
    SDL_Color yellow = {255, 255, 0};
    SDL_Surface** scoreSurface = (SDL_Surface**) malloc(sizeof(SDL_Surface*) * activePlayers);
 
    for (int i = 0; i < activePlayers; i++) {
      scoreSurface[i] = TTF_RenderText_Solid(font, scoreStr[i], (currIdx == i) ? yellow : white);
    }
    SDL_Texture** scoreTexture = (SDL_Texture**) malloc(sizeof(SDL_Texture*) * activePlayers);
    for (int i = 0; i < activePlayers; i++) {
      scoreTexture[i] = SDL_CreateTextureFromSurface(renderer, scoreSurface[i]);
    }

    SDL_Rect* scoreDest = (SDL_Rect*) malloc(sizeof(SDL_Rect) * activePlayers);
    for (int i = 0; i < activePlayers; i++) {
       TTF_SizeText(font, scoreStr[i], &scoreDest[i].w, &scoreDest[i].h);
       scoreDest[i].x = 250 * ((i >> 1) & 1);
       scoreDest[i].y = (HEADER_HEIGHT / 2) * (i & 1);
    }

    SDL_Surface* levelSurface = TTF_RenderText_Solid(levFont, levelStr, white);
    SDL_Texture* levelTexture = SDL_CreateTextureFromSurface(renderer, levelSurface);

    SDL_Rect levelDest;
    TTF_SizeText(levFont, levelStr, &levelDest.w, &levelDest.h);
    levelDest.x = GRID_DRAW_WIDTH - levelDest.w;
    levelDest.y = 0;

    for (int i = 0; i < activePlayers; i++) {
       SDL_RenderCopy(renderer, scoreTexture[i], NULL, scoreDest + i);
    }
    SDL_RenderCopy(renderer, levelTexture, NULL, &levelDest);

    for (int i = 0; i < activePlayers; i++) {
       SDL_FreeSurface(scoreSurface[i]);
       SDL_DestroyTexture(scoreTexture[i]);
    }

    SDL_FreeSurface(levelSurface);
    SDL_DestroyTexture(levelTexture);

    for (int i = 0; i < activePlayers; i++) free(scoreStr[i]);
    free(scoreStr);
    free(scoreSurface);
    free(scoreTexture);
    free(scoreDest);
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    if (argc != 3) {
      fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
      exit(0); 
   } 
  
    initSDL();

    levFont = TTF_OpenFont("resources/Burbank-Big-Condensed-Bold-Font.otf", HEADER_HEIGHT);
    font = TTF_OpenFont("resources/Burbank-Big-Condensed-Bold-Font.otf", HEADER_HEIGHT/2);
    if (!font || !levFont) {
        fprintf(stderr, "Error loading fonts: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window* window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    if (window == NULL) {
        fprintf(stderr, "Error creating app window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == NULL)
	{
		fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
	}

    SDL_Texture *grassTexture = IMG_LoadTexture(renderer, "resources/grass.png");
    SDL_Texture *tomatoTexture = IMG_LoadTexture(renderer, "resources/tomato.png");
    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "resources/player.png");
 
    connectToServer(argv[1], argv[2]);

    // main game loop
    while (!shouldExit) {
        SDL_SetRenderDrawColor(renderer, 0, 105, 6, 255);
        SDL_RenderClear(renderer);
        
        processInputs();      
        getBoardState(); 

        drawGrid(renderer, grassTexture, tomatoTexture, playerTexture);
        drawUI(renderer);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // 16 ms delay to limit display to 60 fps
    }

    // clean up everything
    Close(clientfd);

    SDL_DestroyTexture(grassTexture);
    SDL_DestroyTexture(tomatoTexture);
    SDL_DestroyTexture(playerTexture);

    TTF_CloseFont(font);
    TTF_CloseFont(levFont);
    TTF_Quit();

    IMG_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
