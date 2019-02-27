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
#define BACKUP_GUIDES_X     SCREEN_WIDTH - VIDEO_OFFSET - BUTTON_SIZE
#define BACKUP_GUIDES_Y     VIDEO_OFFSET
#define CLOCK_DIGIT_1_X     220
#define CLOCK_DIGIT_2_X     280
#define CLOCK_DIGIT_C_X     355
#define CLOCK_DIGIT_3_X     400
#define CLOCK_DIGIT_4_X     460
#define CLOCK_DIGIT_Y       190
#define CLOCK_DIGIT_SIZE    100

// Other defines
#define DEBOUNCE_MAX 60
#define ERROR_MESSAGE_TITLE "System Error"

bool initDisplay();
void renderMainScreen();
void handleMouseClick(Sint32 x, Sint32 y);

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

static volatile int keepRunning = 1;
int mousePressCounter = 0;

void errorMessage(const char* msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ERROR_MESSAGE_TITLE, msg, NULL);
    
    printf("SDL_Error: %s\n", SDL_GetError());
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

appIndex getMouseClickIndex(Sint32 x, Sint32 y)
{
    // Main window
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
    else  // Button window
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
    displayView prevView = currentView;
    
    // Determine the selected app index and set the current view
    appIndex appClicked = getMouseClickIndex(x, y);
    
    if (prevView != currentView)
    {
        if (currentView == MAIN_WINDOW_VIEW)
        {
            renderMainScreen();
        }
        else
        {
            const char* media;
            switch (appClicked)
            {
                case FRONT_CAMERA_INDEX:
                    media = "media/cam3.jpg";
                    break;
                case REAR_CAMERA_INDEX:
                    media = "media/cam3.jpg";
                    break;
                case ANDROID_AUTO_INDEX:
                    media = "media/cam2.jpg";
                    break;
                default:
                    break;
            }
            
            // Display the selected screen
            SDL_Rect newScreen;
            newScreen.x = 0;
            newScreen.y = 0;
            newScreen.w = SCREEN_WIDTH;
            newScreen.h = SCREEN_HEIGHT;
            texture = IMG_LoadTexture(renderer, media);
            SDL_RenderCopy(renderer, texture, NULL, &newScreen);
            
            // Overlay back button
            SDL_Rect backButton;
            backButton.x = BACK_BUTTON_X;
            backButton.y = BACK_BUTTON_Y;
            backButton.w = BUTTON_SIZE;
            backButton.h = BUTTON_SIZE;
            texture = IMG_LoadTexture(renderer, "media/arrow.png");
            SDL_RenderCopy(renderer, texture, NULL, &backButton);
            
            if (currentView == REAR_CAMERA_VIEW)
            {
                // Display backup guides
                SDL_Rect backupGuides;
                backupGuides.x = BACKUP_GUIDES_X;
                backupGuides.y = BACKUP_GUIDES_Y;
                backupGuides.w = BUTTON_SIZE;
                backupGuides.h = BUTTON_SIZE;
                texture = IMG_LoadTexture(renderer, "media/guides.png");
                SDL_RenderCopy(renderer, texture, NULL, &backupGuides);
            }
            
            // Draw to the window tied to the renderer
            SDL_RenderPresent(renderer);
        }
    }
    else if (currentView == REAR_CAMERA_VIEW)
    {
        if (appClicked == BACKUP_GUIDES_INDEX)
        {
            // Back button display
            SDL_Rect backupGuides;
            backupGuides.x = 0;
            backupGuides.y = 0;
            backupGuides.w = SCREEN_WIDTH;
            backupGuides.h = SCREEN_HEIGHT;
            texture = IMG_LoadTexture(renderer, "media/guides.png");
            SDL_RenderCopy(renderer, texture, NULL, &backupGuides);

            // Draw to the window tied to the renderer
            SDL_RenderPresent(renderer);

        }
    }
}

bool initTCP()
{
    return true;
}

void renderClock()
{
    SDL_Rect digit1;
    digit1.x = CLOCK_DIGIT_1_X;
    digit1.y = CLOCK_DIGIT_Y;
    digit1.w = CLOCK_DIGIT_SIZE;
    digit1.h = CLOCK_DIGIT_SIZE;
    texture = IMG_LoadTexture(renderer, "media/1.png");
    SDL_RenderCopy(renderer, texture, NULL, &digit1);

    SDL_Rect digit2;
    digit2.x = CLOCK_DIGIT_2_X;
    digit2.y = CLOCK_DIGIT_Y;
    digit2.w = CLOCK_DIGIT_SIZE;
    digit2.h = CLOCK_DIGIT_SIZE;
    texture = IMG_LoadTexture(renderer, "media/2.png");
    SDL_RenderCopy(renderer, texture, NULL, &digit2);

    SDL_Rect colonDigit;
    colonDigit.x = CLOCK_DIGIT_C_X;
    colonDigit.y = CLOCK_DIGIT_Y;
    colonDigit.w = CLOCK_DIGIT_SIZE;
    colonDigit.h = CLOCK_DIGIT_SIZE;
    texture = IMG_LoadTexture(renderer, "media/colon.png");
    SDL_RenderCopy(renderer, texture, NULL, &colonDigit);

    SDL_Rect digit3;
    digit3.x = CLOCK_DIGIT_3_X;
    digit3.y = CLOCK_DIGIT_Y;
    digit3.w = CLOCK_DIGIT_SIZE;
    digit3.h = CLOCK_DIGIT_SIZE;
    texture = IMG_LoadTexture(renderer, "media/5.png");
    SDL_RenderCopy(renderer, texture, NULL, &digit3);

    SDL_Rect digit4;
    digit4.x = CLOCK_DIGIT_4_X;
    digit4.y = CLOCK_DIGIT_Y;
    digit4.w = CLOCK_DIGIT_SIZE;
    digit4.h = CLOCK_DIGIT_SIZE;
    texture = IMG_LoadTexture(renderer, "media/9.png");
    SDL_RenderCopy(renderer, texture, NULL, &digit4);
}

void renderMainScreen()
{
    // Set the color the screen clears to
    SDL_SetRenderDrawColor(renderer, 0x36, 0x53, 0x82, 0xFF);
    
    // Clear the entire screen to our selected color
    SDL_RenderClear(renderer);
    
    //                // Main screen background
    //                SDL_Rect mainBG;
    //                mainBG.x = 0;
    //                mainBG.y = 0;
    //                mainBG.w = SCREEN_WIDTH;
    //                mainBG.h = SCREEN_HEIGHT;
    //                texture = IMG_LoadTexture(renderer, "media/bg2_light.png");
    //                SDL_RenderCopy(renderer, texture, NULL, &mainBG);
    
    // Front camera display
    SDL_Rect frontCam;
    frontCam.x = FRONT_CAM_X;
    frontCam.y = FRONT_CAM_Y;
    frontCam.w = VIDEO_WIDTH;
    frontCam.h = VIDEO_HEIGHT;
    texture = IMG_LoadTexture(renderer, "media/cam3.jpg");
    SDL_RenderCopy(renderer, texture, NULL, &frontCam);
    
    // Rear camera display
    SDL_Rect rearCam;
    rearCam.x = REAR_CAM_X;
    rearCam.y = REAR_CAM_Y;
    rearCam.w = VIDEO_WIDTH;
    rearCam.h = VIDEO_HEIGHT;
    texture = IMG_LoadTexture(renderer, "media/cam3.jpg");
    SDL_RenderCopy(renderer, texture, NULL, &rearCam);
    
    // Android auto display
    SDL_Rect androidAuto;
    androidAuto.x = ANDROID_AUTO_CAM_X;
    androidAuto.y = ANDROID_AUTO_CAM_Y;
    androidAuto.w = VIDEO_WIDTH;
    androidAuto.h = VIDEO_HEIGHT;
    texture = IMG_LoadTexture(renderer, "media/androidAuto.jpeg");
    SDL_RenderCopy(renderer, texture, NULL, &androidAuto);
    
    renderClock();
    
    // Draw to the window tied to the renderer
    SDL_RenderPresent(renderer);
    
    // View starts on main screen
    currentView = MAIN_WINDOW_VIEW;
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
                renderMainScreen();
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
