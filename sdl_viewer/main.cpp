#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <SDL2_net/SDL_net.h>
#include <fstream>

#include <iostream>

// Screen dimension constants
#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       480
#define VIDEO_WIDTH         200
#define VIDEO_HEIGHT        120
#define VIDEO_OFFSET        40
#define BUTTON_SIZE         50
#define FRONT_CAM_X         VIDEO_OFFSET
#define FRONT_CAM_Y         VIDEO_OFFSET
#define REAR_CAM_X          VIDEO_OFFSET
#define REAR_CAM_Y          SCREEN_HEIGHT - VIDEO_OFFSET - VIDEO_HEIGHT
#define ANDROID_AUTO_CAM_X  SCREEN_WIDTH  - VIDEO_OFFSET - VIDEO_WIDTH
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
#define DEBOUNCE_MAX         60
#define TCP_TIMEOUT          10000000
#define ERROR_MESSAGE_TITLE  "System Error"
#define MEDIA_BACKGROUND     "media/bg2.jpg"
#define MEDIA_ARROW          "media/arrow.png"
#define MEDIA_GUIDES         "media/guides.png"
#define MEDIA_ANDROID_AUTO   "media/androidAuto.jpeg"
#define MEDIA_CLOCK_0        "media/0.png"
#define MEDIA_CLOCK_1        "media/1.png"
#define MEDIA_CLOCK_2        "media/2.png"
#define MEDIA_CLOCK_3        "media/3.png"
#define MEDIA_CLOCK_4        "media/4.png"
#define MEDIA_CLOCK_5        "media/5.png"
#define MEDIA_CLOCK_6        "media/6.png"
#define MEDIA_CLOCK_7        "media/7.png"
#define MEDIA_CLOCK_8        "media/8.png"
#define MEDIA_CLOCK_9        "media/9.png"
#define MEDIA_CLOCK_C        "media/colon.png"

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

// Global variables
SDL_Window*   window   = NULL;  // The window we'll be rendering to
SDL_Renderer* renderer = NULL;  // The window renderer
SDL_Texture*  texture  = NULL;  // Current displayed texture
SDL_Texture* yuvTexture = NULL;
SDL_Rect yuvRect;
SDL_Event e;
TCPsocket serverSock;
TCPsocket clientSock;
void* pixels;
int pitch;

std::ofstream logFile;
static volatile int keepRunning = 1;
int mousePressCounter = 0;
bool backupGuidesActive = false;

// Function declarations
void errorMessage(const char* msg);
void screenControl_tick();
bool initAll();
bool initTCP();
void renderClock();
void renderMainScreen();
void handleMouseClick(Sint32 x, Sint32 y);
void addTexture(int x, int y, int w, int h, const char* media);
void destroyAll();
appIndex getMouseClickIndex(Sint32 x, Sint32 y);

void errorMessage(const char* msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ERROR_MESSAGE_TITLE, msg, NULL);
}

void addTexture(int x, int y, int w, int h, const char* media)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    texture = IMG_LoadTexture(renderer, media);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
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
            logFile << "tick state default error\n";
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
            logFile << "tick state default error\n";
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
            addTexture(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, media);
            
            // Overlay back button
            addTexture(BACK_BUTTON_X, BACK_BUTTON_Y, BUTTON_SIZE,
                       BUTTON_SIZE, MEDIA_ARROW);
            
            if (currentView == REAR_CAMERA_VIEW)
            {
                // Display backup guides
                addTexture(BACKUP_GUIDES_X, BACKUP_GUIDES_Y, BUTTON_SIZE,
                           BUTTON_SIZE, MEDIA_GUIDES);
            }
            
            // Draw to the window tied to the renderer
            SDL_RenderPresent(renderer);
        }
    }
    else if (currentView == REAR_CAMERA_VIEW)
    {
        if (appClicked == BACKUP_GUIDES_INDEX)
        {
            // Display the rear camera screen
            addTexture(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, "media/cam3.jpg");

            // Overlay back button
            addTexture(BACK_BUTTON_X, BACK_BUTTON_Y, BUTTON_SIZE,
                       BUTTON_SIZE, MEDIA_ARROW);

            // Display backup guides
            addTexture(BACKUP_GUIDES_X, BACKUP_GUIDES_Y, BUTTON_SIZE,
                       BUTTON_SIZE, MEDIA_GUIDES);

            if (backupGuidesActive)
            {
                backupGuidesActive = false;
            }
            else
            {
                backupGuidesActive = true;
                
                // Back button display
                addTexture(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MEDIA_GUIDES);
            }

            // Draw to the window tied to the renderer
            SDL_RenderPresent(renderer);
        }
    }
}

void renderClock()
{
    addTexture(CLOCK_DIGIT_1_X, CLOCK_DIGIT_Y, CLOCK_DIGIT_SIZE,
               CLOCK_DIGIT_SIZE, MEDIA_CLOCK_1);

    addTexture(CLOCK_DIGIT_2_X, CLOCK_DIGIT_Y, CLOCK_DIGIT_SIZE,
               CLOCK_DIGIT_SIZE, MEDIA_CLOCK_2);

    addTexture(CLOCK_DIGIT_C_X, CLOCK_DIGIT_Y, CLOCK_DIGIT_SIZE,
               CLOCK_DIGIT_SIZE, MEDIA_CLOCK_C);

    addTexture(CLOCK_DIGIT_3_X, CLOCK_DIGIT_Y, CLOCK_DIGIT_SIZE,
               CLOCK_DIGIT_SIZE, MEDIA_CLOCK_5);

    addTexture(CLOCK_DIGIT_4_X, CLOCK_DIGIT_Y, CLOCK_DIGIT_SIZE,
               CLOCK_DIGIT_SIZE, MEDIA_CLOCK_9);
}

void renderMainScreen()
{
    // Set the color the screen clears to
    SDL_SetRenderDrawColor(renderer, 0x36, 0x53, 0x82, 0xFF);
    
    // Clear the entire screen to our selected color
    SDL_RenderClear(renderer);
    
    addTexture(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MEDIA_BACKGROUND);
    
    // Front camera display
    yuvTexture = SDL_CreateTexture(renderer,
                                   SDL_PIXELFORMAT_IYUV,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   VIDEO_WIDTH, VIDEO_HEIGHT);
    yuvRect.x = FRONT_CAM_X;
    yuvRect.y = FRONT_CAM_Y;
    yuvRect.w = VIDEO_WIDTH;
    yuvRect.h = VIDEO_HEIGHT;
    SDL_RenderCopy(renderer, yuvTexture, NULL, &yuvRect);

//    addTexture(FRONT_CAM_X, FRONT_CAM_Y, VIDEO_WIDTH,
//               VIDEO_HEIGHT, "media/cam3.jpg");
    
    // Rear camera display
    addTexture(REAR_CAM_X, REAR_CAM_Y, VIDEO_WIDTH,
               VIDEO_HEIGHT, "media/cam3.jpg");
    
    // Android auto display
    addTexture(ANDROID_AUTO_CAM_X, ANDROID_AUTO_CAM_Y, VIDEO_WIDTH,
               VIDEO_HEIGHT, MEDIA_ANDROID_AUTO);

    renderClock();
    
    // Draw to the window tied to the renderer
    SDL_RenderPresent(renderer);
    
    // View starts on main screen
    currentView = MAIN_WINDOW_VIEW;
}

bool initTCP()
{
    Uint32 tcpTimer = 0;
    IPaddress ip;
    
    // Create a listening TCP socket on port 9999 (server)
    if (SDLNet_ResolveHost(&ip, NULL, 9999) == -1)
    {
        logFile << "initTCP - SDLNet_ResolveHost: "
                << SDLNet_GetError() << '\n';
        return false;
    }
    
    serverSock = SDLNet_TCP_Open(&ip);
    if (!serverSock)
    {
        logFile << "initTCP - SDLNet_TCP_Open: "
                << SDLNet_GetError() << '\n';
        return false;
    }
    
    // Accept a connection coming in on the server tcp socket
    while (tcpTimer <= TCP_TIMEOUT && !clientSock)
    {
        clientSock = SDLNet_TCP_Accept(serverSock);
        tcpTimer++;
    }

    if (!clientSock)
    {
        logFile << "initTCP - Client Timeout\n";
        return false;
    }
    return true;
}

bool initAll()
{
    // Open log file
    logFile.open("carMonitor_log.txt");
    
    // Setup tcp connection
    if (!initTCP())
    {
        errorMessage("ERROR: Could Not Initialize TCP Connection");
        return false;
    }

    if (SDL_Init(SDL_INIT_VIDEO) >= 0)
    {
        // Set texture filtering to linear
        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
            logFile << "Warning: Linear texture filtering not enabled\n";
        
        // Create window
        window = SDL_CreateWindow("Car Monitor", SDL_WINDOWPOS_UNDEFINED,                      SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
        
        if (window != NULL)
        {
            // Create renderer for window
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            
            if (renderer != NULL)
                renderMainScreen();
            else
            {
                logFile << "Renderer Error:" << SDL_GetError() << '\n';
                return false;
            }
        }
        else
        {
            logFile << "Window Error:" << SDL_GetError() << '\n';
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

void destroyAll()
{
    std::cout << "shutting down\n";
    SDL_DestroyTexture(yuvTexture);
    // Close the connection on sock
    SDLNet_TCP_Close(serverSock);
    SDLNet_TCP_Close(clientSock);
    
    // Destroy SDL objects
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    // Quit SDL subsystems
    SDL_Quit();
    
    // Close the log file
    logFile.close();
}

int main(int argc, char* args[])
{
    if (initAll())
    {
        char msg[SCREEN_WIDTH * SCREEN_HEIGHT * 16];
        
        // Communicate over the new tcp socket
        int bytes = SDLNet_TCP_Recv(clientSock, msg, 1);
        
        if (bytes > 0)
        {
            std::cout << bytes << '\n';
            std::ofstream myFile;
            myFile.open("blob.yuv");
            myFile << msg << '\n';
            myFile.close();
        }
        else
            std::cout << "Something bad happened\n";

//        void* pixels = NULL;
//        int pitch = 0;
//        if (SDL_LockTexture(yuvTexture, NULL, &pixels, &pitch) < 0)
//        {
//            std::cout << "Error locking texture\n";
//        }
//        else
//        {
//            if (pixels)
//                memcpy(pixels, msg, pitch * SCREEN_HEIGHT);
//            else
//                std::cout << "error: pixels is null\n";
//
//            SDL_UnlockTexture(yuvTexture);
//            SDL_RenderCopy(renderer, yuvTexture, NULL, &yuvRect);
//            SDL_RenderPresent(renderer);
//        }
        
//        while (keepRunning)
//        {
//            screenControl_tick();
//        }
        
        destroyAll();
    }
    
    // Bluetooth: A2DP for music, AVRCP for music remote control
    
    return 0;
}
