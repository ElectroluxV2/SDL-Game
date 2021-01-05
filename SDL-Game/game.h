class Game {
  const int SCREEN_WIDTH = 640;
  const int SCREEN_HEIGHT = 480;
  const float RESISTANCE = 0.5; // 0.128
  const float PLAYER_FORCE = 1; // 0.5
  const float ACCELERATION_PER_TICK = 1.5; // 0.4

  // The window we'll be rendering to
  SDL_Window* window = NULL;

  // The surface contained by the window
  SDL_Surface* screenSurface = NULL;

  // The image we will load and show on the screen
  SDL_Surface* mapSurface = NULL;

  // Contains all possible characters
  SDL_Surface* charsetSurface = NULL;

  // Player
  SDL_Surface* juanSurface = NULL;

  // The window renderer
  SDL_Renderer* renderer = NULL;

  // Screen texture
  SDL_Texture* screenTexture;

  // Main loop flag
  bool quit = false;

  // Helpers colors
  int black, green, red, blue;

  // Main tmier
  float fps;

  // Start counting frames per second
  int countedFrames = 0;

  // Player x
  float playerX = 0;
  float accelerationX = 0;
  float lastAccelerationX = 0;
  bool xchanged = false;

  bool Load() {

   // printf("R: %f\n", RESISTANCE_PEAK);
    printf("R: %f\n", ACCELERATION_PER_TICK);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("SDL_Init error: %s\n", SDL_GetError());
      return false;
    }

    // Create window
    window = SDL_CreateWindow("Robot unicorn atack", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
      printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
      return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
      printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
      return false;
    }

    // Initialize renderer
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    // Get window surface
    screenSurface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    // Get screen texture
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Init colors
    black = SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00);
    green = SDL_MapRGB(screenSurface->format, 0x00, 0xFF, 0x00);
    red = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
    blue = SDL_MapRGB(screenSurface->format, 0x11, 0x11, 0xCC);

    // Disable cursor
    SDL_ShowCursor(SDL_DISABLE);

    printf("Successfully loaded\n");
    return true;
  }

  bool LoadMedia() {
    if (!LoadSurface("cs8x8.bmp", &charsetSurface)) return false;
    SDL_SetColorKey(charsetSurface, true, 0x000000);

    if (!LoadOptimizedSurface("map.bmp", &screenSurface, &mapSurface)) return false;
    if (!LoadOptimizedSurface("juan.bmp", &screenSurface, &juanSurface)) return false;

    printf("Successfully loaded media\n");
    return true;
  }

  void Close() {
    // Deallocate surface
    FreeSurface(&mapSurface);
    FreeSurface(&charsetSurface);
    FreeSurface(&juanSurface);

    // Destroy window
    SDL_DestroyWindow(window);
    window = NULL;

    // Quit SDL subsystems
    SDL_Quit();
    printf("Successfully closed\n");
  }

  void HandleEvents() {
    // Event handler
    SDL_Event e{};

    while (SDL_PollEvent(&e) != 0) {
      // User requests quit
      if (e.type == SDL_QUIT) {
        quit = true;
      } else if (e.type == SDL_KEYDOWN) {

        // Actions based on key
        switch (e.key.keysym.sym) {
          case SDLK_UP:
            break;

          case SDLK_DOWN:
            break;

          case SDLK_LEFT:
            accelerationX += ACCELERATION_PER_TICK;
            break;

          case SDLK_RIGHT:
            accelerationX -= ACCELERATION_PER_TICK;
            break;
        }
      }
    }
  }

  void DrawUI() {
    DrawRectangle(screenSurface, 4, 4, SCREEN_WIDTH - 8, 36, green, black);
    DrawRectangle(screenSurface, 4, 45, SCREEN_WIDTH - 8, 431, green, NULL);

    char text[128];
    sprintf(text, "Frames Per Second: %f", fps);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    sprintf(text, "accelerationX: %f, change: %d, posX: %f", accelerationX, xchanged, playerX);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26, text, charsetSurface);
  }

  void DrawBackground() {

    int x = (int) playerX % mapSurface->w;

    if (-x == mapSurface->w) {
      x = 0;
    }

   // printf("x: %d\n", x);

    BetterDrawSurface(screenSurface, mapSurface, x, 261);
    BetterDrawSurface(screenSurface, mapSurface, x + mapSurface->w, 261);
  }

  void DrawPlayer() { 
    BetterDrawSurface(screenSurface, juanSurface, 0 , 261);
  }

  bool hit = false;
  void Physics(unsigned long totalTicks) { 


    if (abs(accelerationX) < FLT_EPSILON) {
      hit = false;
    }

    if (abs(accelerationX) > ACCELERATION_PER_TICK || hit) {
      hit = true;
      if (accelerationX > 0)
        accelerationX -= RESISTANCE;
      else if (accelerationX < 0)
        accelerationX += RESISTANCE;
    } else {
      hit = false;
    }

    if (accelerationX == 0) return;
    playerX += PLAYER_FORCE * accelerationX;
  }

  void Render() {

    // Clear screen
    SDL_FillRect(screenSurface, NULL, black);

    DrawUI();
    
    DrawBackground();

    DrawPlayer();

    SDL_UpdateTexture(screenTexture, NULL, screenSurface->pixels, screenSurface->pitch);
    //		SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  void Loop() {
    // The frames per second timer
    LTimer fpsTimer;

    // Start counting frames per second
    unsigned long countedFrames = 0;
    fpsTimer.start();


    // While application is running
    while (!quit) {
      // Handle events on queue
      HandleEvents();

      // Handle physics
      Physics(countedFrames);

      // Calculate fps
      fps = countedFrames / (fpsTimer.getTicks() / (float) 1000);

      // Render
      Render();
      countedFrames++;
    }
  }

 public:
  void Start() { 
    if (!Load()) return;
    if (!LoadMedia()) return;

    Loop();

    Close();
  }
};