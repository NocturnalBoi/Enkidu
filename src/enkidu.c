#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>

// DEFINES and VARIABLES
typedef struct vector2f { float x, y; } vec2f;
typedef struct vector2i { int32_t x, y; } vec2i;

#define ASSERT(_exception, ...) if (!(_exception)) { fprintf(stderr, __VA_ARGS__); exit(1); }

#define PI 3.1415926535

#define SCREEN_WIDTH 256 //384
#define SCREEN_HEIGHT 256 //216
#define SCREEN_PADDING 10

#define WINDOW_SCALE 3
#define WINDOW_WIDTH SCREEN_WIDTH * WINDOW_SCALE
#define WINDOW_HEIGHT SCREEN_HEIGHT * WINDOW_SCALE

#define MOVE_SPEED 2
#define ROTATION_SPEED 5
#define ROTATION_ANGLE_PER_TICK 0.1
#define ENABLE_TANK_CONTROLS true

#define MAP_SIZE 8
int map[MAP_SIZE * MAP_SIZE] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 1, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 1, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
};

#define CELL_SIZE_X SCREEN_WIDTH / MAP_SIZE
#define CELL_SIZE_Y SCREEN_HEIGHT / MAP_SIZE

double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

struct {
    // SDL data
    SDL_Window *window;
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    bool quit;

    // Player
    float angle;
    vec2f position, positionDelta;
} state; 

// GAMEPLAY FUNCTIONS

// Update player position in state struct. Clamps between edges of the screen. (maybe decouple from screen res?)
static void setPlayerPos(float new_x, float new_y){
    state.position.x = clamp(new_x, SCREEN_PADDING, SCREEN_WIDTH - SCREEN_PADDING);
    state.position.y = clamp(new_y, SCREEN_PADDING, SCREEN_HEIGHT - SCREEN_PADDING);
}

// Set player position to the middle of the screen and sets angle & position delta values
static void init_game() {
    setPlayerPos(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    state.angle = PI;
    state.positionDelta.x = cos(state.angle); // * MOVE_SPEED;
    state.positionDelta.y = sin(state.angle); // * MOVE_SPEED;
}

static int convertPixelCoordToLinear( int x, int y) {
    int newX, newY = 0;
    newX = clamp(x, 0, SCREEN_WIDTH);
    newY = clamp(y, 0, SCREEN_HEIGHT);
    
    return newY * SCREEN_WIDTH + newX;
}

// Draw a background cell as 2D image to the Pixel Buffer
static void drawCell2D(int x, int y, uint32_t color) {
    int px, py = 0;
    int startX, startY, newID = 0;
    int dw = 1; // Divider Width

    for (px = 0; px < CELL_SIZE_X; px++){
        for (py = 0; py < CELL_SIZE_Y; py++){
            bool isDividerX = (px < dw || px > CELL_SIZE_X - dw);
            bool isDividerY = (py < dw || py > CELL_SIZE_X - dw);
            uint32_t newColor = (isDividerX || isDividerY)  ? 0x000000 : color;
            int NewID = convertPixelCoordToLinear(x * CELL_SIZE_X + px, y * CELL_SIZE_Y + py);
            state.pixels[NewID] = newColor;
        }
    }
}

// Draw a background 2D map
static void drawMap2D() {
    int x, y, px, py;
    uint32_t CellColor;

    // Iterate over map cells
    for (x = 0; x < MAP_SIZE; x++) {
        for (y = 0; y < MAP_SIZE; y++){
            CellColor = (map[y*MAP_SIZE + x] == 1)  ? 0x808080 : 0x151515;
            drawCell2D(x, y, CellColor);
        }
    }
}

// Draw Player as a white point and his direction as a grey point
static void drawPlayer2D() {
    //Draw Player Point
    if ((state.position.x >= 0 && state.position.x <= SCREEN_WIDTH) && (state.position.y >= 0 && state.position.y <= SCREEN_HEIGHT)) {
        int pixelID = convertPixelCoordToLinear(state.position.x, state.position.y);
        state.pixels[pixelID] = 0xFFFFFFFF;
    }

    // Draw Debug Directional Point
    float arrowLength = 5;
    float arrowPosX = state.position.x + state.positionDelta.x * arrowLength;
    float arrowPosY = state.position.y + state.positionDelta.y * arrowLength;
    if ((arrowPosX>= 0 && arrowPosX <= SCREEN_WIDTH) && (arrowPosY >= 0 && arrowPosY <= SCREEN_HEIGHT)) {
        int pixelID = convertPixelCoordToLinear(arrowPosX, arrowPosY);
        state.pixels[pixelID] = 0x606060;
    }
}

// Add a rotational delta to player
static void rotatePlayer(float angleDelta) {
    float newAngle = state.angle + angleDelta;

    if (angleDelta > 0 && newAngle > 2 * PI) {
        newAngle -= 2 * PI;
    }

    if (angleDelta < 0 && newAngle < 0) {
        newAngle += 2 * PI;
    }

    state.angle = newAngle;
    state.positionDelta.x = cos(state.angle) * MOVE_SPEED;
    state.positionDelta.y = sin(state.angle) * MOVE_SPEED;
}

// Render Player and World on the screen
static void render() {
    drawMap2D();    // Draw Map to Pixel Buffer
    drawPlayer2D(); // Draw Player to Pixel Buffer
}

static void applyInput() {
    float newPosX = state.position.x;
    float newPosY = state.position.y;
    
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);

    if (ENABLE_TANK_CONTROLS) {
        if (keystate[SDL_SCANCODE_LEFT]) {
            rotatePlayer(-ROTATION_ANGLE_PER_TICK);
        }

        if (keystate[SDL_SCANCODE_RIGHT]) {
            rotatePlayer(ROTATION_ANGLE_PER_TICK);
        }

         if (keystate[SDL_SCANCODE_UP]) {
           newPosX += state.positionDelta.x;
           newPosY += state.positionDelta.y;
        }

        if (keystate[SDL_SCANCODE_DOWN]) {
           newPosX -= state.positionDelta.x;
           newPosY -= state.positionDelta.y;
        }
    } 
    else { // Grid-based movement without rotation
        if (keystate[SDL_SCANCODE_UP]) {
            newPosY -= MOVE_SPEED;
        }

        if (keystate[SDL_SCANCODE_DOWN]) {
            newPosY += MOVE_SPEED;
        }

        if (keystate[SDL_SCANCODE_LEFT]) {
            newPosX -= MOVE_SPEED;
        }

        if (keystate[SDL_SCANCODE_RIGHT]) {
            newPosX += MOVE_SPEED;
        }
    }
    
    setPlayerPos(newPosX, newPosY);
}

// MAIN ENTRY POINT
int main(int argc, char *argv[]) {
    ASSERT(
        !SDL_Init(SDL_INIT_VIDEO),
        "SDL failed to initialize: %s\n",
        SDL_GetError());

    state.window = SDL_CreateWindow(
        "DEMO",
        SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_ALLOW_HIGHDPI
    );

    ASSERT(state.window, "failed to create SDL window: %s\n", SDL_GetError());

    state.renderer = SDL_CreateRenderer(state.window, -1, SDL_RENDERER_PRESENTVSYNC);
    ASSERT(state.renderer, "failed to create SDL renderer: %s\n", SDL_GetError());

    state.texture = 
        SDL_CreateTexture(
            state.renderer,
            SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        );

    ASSERT(state.texture, "failed to create SDL texture: %s\n", SDL_GetError());

    init_game(); // Init player and gameplay systems

    while(!state.quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    state.quit = true;
                    break;
            }
        }
        SDL_RenderClear(state.renderer);
        memset(state.pixels, 0, sizeof(state.pixels));
        applyInput(); // Check if any buttons are being pressed and update player position & rotation
        render(); // Draw world to pixel buffer

        SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH*4);
        SDL_RenderCopyEx(
            state.renderer,
            state.texture,
            NULL,
            NULL,
            0.0,
            NULL,
            SDL_FLIP_NONE);

        SDL_RenderPresent(state.renderer);
    }

    SDL_DestroyTexture(state.texture);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);  
    return 0;
}

// Blinking pixel test
/*
float SineSpeed = 0.01;
Uint64 CurrentTick = SDL_GetTicks64() * SineSpeed;
double SineTick = sin(CurrentTick);
int PixelID = clamp(66 * SCREEN_WIDTH + 10, 0, sizeof(state.pixels)); // row * SCREEN_WIDTH + column
state.pixels[PixelID] = (SineTick > 0) ? 0xFFFF00FF : 0x00000000;
*/