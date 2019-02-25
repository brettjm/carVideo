#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <stdio.h>
#include <string>

// Screen dimension constants
#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       480
#define VIDEO_WIDTH         200
#define VIDEO_HEIGHT        120
#define VIDEO_OFFSET        40
#define BUTTON_SIZE         VIDEO_OFFSET
#define FRONT_CAM_X         VIDEO_OFFSET
#define FRONT_CAM_Y         VIDEO_OFFSET
#define REAR_CAM_X          VIDEO_OFFSET
#define REAR_CAM_Y          SCREEN_HEIGHT - VIDEO_OFFSET - VIDEO_HEIGHT
#define ANDROID_AUTO_CAM_X  SCREEN_WIDTH - VIDEO_OFFSET - VIDEO_WIDTH
#define ANDROID_AUTO_CAM_Y  VIDEO_OFFSET
#define BACK_BUTTON_X       VIDEO_OFFSET
#define BACK_BUTTON_Y       VIDEO_OFFSET
#define BACKUP_GUIDES_X     SCREEN_WIDTH - VIDEO_OFFSET - VIDEO_OFFSET
#define BACKUP_GUIDES_Y     SCREEN_WIDTH - VIDEO_OFFSET - VIDEO_OFFSET

// Other defines
#define DEBOUNCE_MAX 60
#define ERROR_MESSAGE_TITLE "System Error"

enum appIndex
{
    EMPTY_INDEX,
    BACK_BUTTON_INDEX,
    BACKUP_GUIDES_INDEX,
    FRONT_CAMERA_INDEX,
    REAR_CAMERA_INDEX,
    ANDROID_AUTO_INDEX
};

enum displayView
{
    MAIN_WINDOW_VIEW,
    FRONT_CAMERA_VIEW,
    REAR_CAMERA_VIEW,
    ANDROID_AUTO_VIEW
} currentView;

SDL_Window* window = NULL;      // The window we'll be rendering to
SDL_Renderer* renderer = NULL;  // The window renderer
SDL_Texture* texture = NULL;    // Current displayed texture
SDL_Event e;
SDL_Rect frontCam;
SDL_Rect rearCam;
SDL_Rect androidAuto;

static volatile int keepRunning = 1;
int mousePressCounter = 0;

void errorMessage(const char* msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ERROR_MESSAGE_TITLE, msg, NULL);
    
    printf("SDL_Error: %s\n", SDL_GetError());
}

appIndex getMouseClickIndex(Sint32 x, Sint32 y)
{
    if (currentView == MAIN_WINDOW_VIEW)
    {
        if (x >= FRONT_CAM_X && x < FRONT_CAM_X + VIDEO_WIDTH)
        {
            if (y >= FRONT_CAM_Y && y < FRONT_CAM_Y + VIDEO_HEIGHT)
            {
                currentView = FRONT_CAMERA_VIEW;
                return FRONT_CAMERA_INDEX;
            }
            else if (y >= REAR_CAM_Y && y < REAR_CAM_Y + VIDEO_HEIGHT)
            {
                currentView = REAR_CAMERA_VIEW;
                return REAR_CAMERA_INDEX;
            }
        }
        else if (x >= ANDROID_AUTO_CAM_X && x < ANDROID_AUTO_CAM_X + VIDEO_WIDTH)
        {
            if (y >= ANDROID_AUTO_CAM_Y && y < ANDROID_AUTO_CAM_Y + VIDEO_HEIGHT)
            {
                currentView = ANDROID_AUTO_VIEW;
                return ANDROID_AUTO_INDEX;
            }
        }
    }
    else
    {
        if (x >= BACK_BUTTON_X && x < BACK_BUTTON_X + BUTTON_SIZE)
        {
            if (y >= BACK_BUTTON_Y && y < BACK_BUTTON_Y + BUTTON_SIZE)
            {
                currentView = MAIN_WINDOW_VIEW;
                return BACK_BUTTON_INDEX;
            }
        }
        else if (currentView == REAR_CAMERA_VIEW)
        {
            if (x >= BACKUP_GUIDES_X && x < BACKUP_GUIDES_X + BUTTON_SIZE)
            {
                if (y >= BACKUP_GUIDES_Y && y < BACKUP_GUIDES_Y + BUTTON_SIZE)
                    return BACKUP_GUIDES_INDEX;
            }
        }
    }
    
    return EMPTY_INDEX;
}

void handleMouseClick(Sint32 x, Sint32 y)
{
    // Determine the selected app index and set the view
    appIndex appClicked = getMouseClickIndex(x, y);
    
    if (currentView == MAIN_WINDOW_VIEW)
    {
        printf("%d\n", appClicked);
    }
    else
    {
        printf("%d\n", appClicked);
    }
}

enum screenControl_st_t
{
    init_st,
    mousePress_st,
    mouseRelease_st
} currentState = init_st;

void screenControl_tick()
{
    // Perform state action first
    switch (currentState)
    {
        case init_st:
            SDL_PollEvent(&e);
            break;
        case mousePress_st:
            mousePressCounter++;
            break;
        case mouseRelease_st:
            SDL_PollEvent(&e);
            break;
        default:
            printf("tick state default error");
            break;
    }
    
    // Perform state update next
    switch (currentState)
    {
        case init_st:
            if (e.type == SDL_QUIT)
                keepRunning = false;
            else if (e.type == SDL_MOUSEBUTTONDOWN)
                currentState = mousePress_st;
            break;
        case mousePress_st:
            if (mousePressCounter >= DEBOUNCE_MAX)
            {
                mousePressCounter = 0;
                currentState = mouseRelease_st;
            }
            break;
        case mouseRelease_st:
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                handleMouseClick(e.button.x, e.button.y);
                currentState = init_st;
            }
            break;
        default:
            printf("tick state default error");
            break;
    }
}

bool initTCP()
{
    return true;
}

bool initDisplay()
{
    if (SDL_Init(SDL_INIT_VIDEO) >= 0)
    {
        // Set texture filtering to linear
        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
            printf("Warning: Linear texture filtering not enabled!");
        
        // Create window
        window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,                      SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
        
        if (window != NULL)
        {
            // Create renderer for window
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            
            if (renderer != NULL)
            {
                // Set the color the screen clears to
                SDL_SetRenderDrawColor(renderer, 0x36, 0x53, 0x82, 0xFF);

                // Clear the entire screen to our selected color.
                SDL_RenderClear(renderer);
                
                // Front camera display
                frontCam.x = FRONT_CAM_X;
                frontCam.y = FRONT_CAM_Y;
                frontCam.w = VIDEO_WIDTH;
                frontCam.h = VIDEO_HEIGHT;
                texture = IMG_LoadTexture(renderer, "media/hello_world.bmp");
                SDL_RenderCopy(renderer, texture, NULL, &frontCam);
                
                // Rear camera display
                rearCam.x = REAR_CAM_X;
                rearCam.y = REAR_CAM_Y;
                rearCam.w = VIDEO_WIDTH;
                rearCam.h = VIDEO_HEIGHT;
                texture = IMG_LoadTexture(renderer, "media/hello_world.bmp");
                SDL_RenderCopy(renderer, texture, NULL, &rearCam);

                // Android auto display
                androidAuto.x = ANDROID_AUTO_CAM_X;
                androidAuto.y = ANDROID_AUTO_CAM_Y;
                androidAuto.w = VIDEO_WIDTH;
                androidAuto.h = VIDEO_HEIGHT;
                texture = IMG_LoadTexture(renderer, "media/hello_world.bmp");
                SDL_RenderCopy(renderer, texture, NULL, &androidAuto);
                
                // Draw to the window tied to the renderer
                SDL_RenderPresent(renderer);
                
                // View starts on main screen
                currentView = MAIN_WINDOW_VIEW;
            }
            else
            {
                errorMessage("ERROR: Renderer could not be created");
                return false;
            }
        }
        else
        {
            errorMessage("ERROR: Window could not be created");
            return false;
        }
    }
    else
    {
        errorMessage("ERROR: SDL_init failed");
        return false;
    }
    return true;
}

void destroySDL()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    // Quit SDL subsystems
    SDL_Quit();
}

int main(int argc, char* args[])
{
    // Setup tcp connection
    if (!initTCP())
    {
        errorMessage("ERROR: Could Not Initialize TCP Connection");
        return 0;
    }

    // If initializing the display works run
    // the program and destroy when closed
    if (initDisplay())
    {
        while (keepRunning)
            screenControl_tick();
        
        destroySDL();
    }
    
    return 0;
}
