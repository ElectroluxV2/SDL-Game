const float RESISTANCE = 0.128; // 0.128
const float PLAYER_FORCE = 0.5; // 0.5
const float ACCELERATION_PER_TICK = 1; // 0.4
const bool DEBUG = false;

class Game {
  const int SCREEN_WIDTH = 640;
  const int SCREEN_HEIGHT = 480;

  const int SCREEN_FPS = 60;
  const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

  bool collisionDetected = false;

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

  // Platforms
  SDL_Surface* platformSurface = NULL;

  // Platfrom array
  struct Platform {
    SDL_Rect box = {0, 0, 0, 0};
    int onMapPlacementX = 0;
  } platforms[10];

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

  // Current time start time
  Uint32 startTime = 0;

  // false -> steering with arrows, true -> automatic movment
  bool mode = false;  

  // Player x
  struct Player {

    bool airBorn = false;
    int jumpCount = 0;
    SDL_Rect box = {
      0,
      0,
      0,
      0
    };

    struct Coord {
      float x;
      float y;
    } acc, pos;

    void Load(int w, int h) {
      box.w = w;
      box.h = h;
    }

    void AddAccelerationX(float f) {
      if (pos.x >= 0) return;
      acc.x += f;
    }

    void SubAccelerationX(float f) {
      acc.x -= f;
    }

    void Jump() {
      if (airBorn) return;
      airBorn = true;
      acc.y += ACCELERATION_PER_TICK * 8;
    }
    
    void Reset() {
      pos.x = 0;
      pos.y = 0;
      acc.x = 0;
      acc.y = 0;
    }

    void JumpPhysics(Platform p) {
      if (acc.y > 0) {
        pos.y += PLAYER_FORCE * acc.y;
        acc.y -= RESISTANCE;
      } else if(pos.y > 0) {
        pos.y -= PLAYER_FORCE * 4;
      } else {
        airBorn = false;
      }

      /*int tmp = pos.y + 0;
      if (acc.y > 0) {
        tmp = pos.y + PLAYER_FORCE * acc.y;
        acc.y -= RESISTANCE;
      } else if (pos.y > 0) {
        tmp = pos.y - PLAYER_FORCE * 4;
      } else {
        airBorn = false;
      }

      box.y = tmp;
      if (checkCollision(box, p)) {
        box.y = pos.y;
        acc.y = 0;
        return;
      }

      pos.y = tmp;*/
    }

    void MovePhysics(Platform p) {
      if (acc.x > 0) acc.x -= RESISTANCE;
      else if (acc.x < 0) acc.x += RESISTANCE;
      if (abs(acc.x) < RESISTANCE) acc.x = 0;

      if (AreSame(acc.x, 0)) return;

      // printf("%f\n", acc.x);

      // Position player want to go
      int tmp = pos.x + (PLAYER_FORCE * acc.x);
      // Relative position of box
      int platformRelativeX = p.box.x;
      p.box.x = p.onMapPlacementX + tmp;
      // Check if can pass by
      if (checkCollision(p.box, box)) {
        // Restore box position
        p.box.x = platformRelativeX;
        // Remove any x acceleration on player
        acc.x = 0;
        return; // Prevent any changes to player's pos
      }

      // Change position
      pos.x = tmp;

      if (pos.x > 0) pos.x = 0;
      if (pos.x >= 0 && acc.x >= 0) acc.x = 0;
    }

    void OnPhysics(Platform p) {
      box.y = -pos.y + 300;
      printf("Player: %d, %d, %d, %d\n", box.x, box.y, box.w, box.h);
      printf("Platfus: %d, %d, %d, %d\n", p.box.x, p.box.y, p.box.w, p.box.h);
      printf("%s\n", checkCollision(box, p.box) ? "true" : "false");
      JumpPhysics(p);
      MovePhysics(p);
    }
  } player;

    void Physics() {
    // Follow player movement
    for (int i = 0; i < 10; i++) {
      Platform* p = &platforms[i];
      int destX = p->onMapPlacementX + player.pos.x;
      p->box.x = destX;
    }

    player.OnPhysics(platforms[0]);
  }

  bool Load() {

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

    //renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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

    //player.pos.y = 200;

    printf("Successfully loaded\n");
    return true;
  }

  bool LoadMedia() {
    if (!LoadSurface("cs8x8.bmp", &charsetSurface)) return false;
    SDL_SetColorKey(charsetSurface, true, 0x000000);

    if (!LoadOptimizedSurface("map.bmp", &screenSurface, &mapSurface)) return false;
    if (!LoadOptimizedSurface("juan.bmp", &screenSurface, &juanSurface)) return false;
    if (!LoadOptimizedSurface("juan'splatform.bmp", &screenSurface, &platformSurface)) return false;
    player.Load(juanSurface->w, juanSurface->h);

    printf("Successfully loaded media\n");
    return true;
  }

  void SetPlatform(int i, int x, int y) {
    Platform p = {{x, y, platformSurface->w, platformSurface->h}, x};
    platforms[i] = p;
  }

  void LoadLevel() { 
    SetPlatform(0, 300, 390);
    SetPlatform(1, 200, 60);
    SetPlatform(2, 900, 270);
    SetPlatform(3, 1500, 400);
    SetPlatform(4, 2000, 350);
    SetPlatform(5, 2400, 200);
    SetPlatform(6, 2900, 400);
    SetPlatform(7, 3500, 350);
    SetPlatform(8, 3900, 400);
    SetPlatform(9, 4300, 300);
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
    // keyboard mocarz obluhuje
    const Uint8 *key = SDL_GetKeyboardState(NULL);
    if (key[SDL_SCANCODE_RIGHT]) {
      player.SubAccelerationX(ACCELERATION_PER_TICK);
    }
    if (key[SDL_SCANCODE_LEFT]) {
      player.AddAccelerationX(ACCELERATION_PER_TICK);
    }
    if (key[SDL_SCANCODE_Z]) {
      player.Jump();
    }
    if (key[SDL_SCANCODE_ESCAPE]) {
      quit = true;
    }
    if (key[SDL_SCANCODE_N]) {
      player.Reset();
      startTime = SDL_GetTicks();
    }
    // Event handler
    SDL_Event e{};
    while (SDL_PollEvent(&e) != 0) {
      // User requests quit
      if (e.type == SDL_QUIT) {
        quit = true;
      }
    }
  }

  void DrawUI() {
    DrawRectangle(screenSurface, 4, 4, SCREEN_WIDTH - 8, 36, green, black);
    DrawRectangle(screenSurface, 4, 45, SCREEN_WIDTH - 8, 431, green, NULL);

    char text[128];
    sprintf(text, "FPS: %.0f Time: %.0f sec", fps, (SDL_GetTicks() - startTime)/1000.0);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    sprintf(text, "accelerationX: %.0f, posX: %.0f,", player.acc.x, player.pos.x);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26, text, charsetSurface);
    /*sprintf(text, "FPS: %.0f Time: %.0f sec", fps, (SDL_GetTicks() - startTime) / 1000.0);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    sprintf(text, "accelerationY: %.0f, posY: %.0f,", player.acc.y, player.GetPos().y);
    sprintf(text, "accelerationY: %f,   posY: %f", player.acc.y, player.GetPos().y);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26, text, charsetSurface);*/

  }

  void DrawBackground() {

    int x = (int)player.pos.x % mapSurface->w;

    if (-x == mapSurface->w) {
      x = 0;
    }

    //printf("x: %d\ty: %d\n", x, 0);

    BetterDrawSurface(screenSurface, mapSurface, x, 46);
    BetterDrawSurface(screenSurface, mapSurface, x + mapSurface->w, 46);
  }

  void DrawPlatforms() {
    //int x = (int)player.GetPos().x % mapSurface->w;
    //printf("x: %d\ty: %d\n", x, 0);

    //BetterDrawSurface(screenSurface, platformSurface, x, 261);
    //BetterDrawSurface(screenSurface, platformSurface, x + 100, 300);
    
    for (int i = 0; i < 10; i++) {
      Platform p = platforms[i];
      //int destX = p.onMapPlacementX + player.pos.x;
      BetterDrawSurface(screenSurface, platformSurface, p.box.x, p.box.y);

      // Display hit boxes for debug purposes
      if (p.box.x < 0 || p.box.x + p.box.w > SCREEN_WIDTH || p.box.y < 0 || p.box.y + p.box.h > SCREEN_HEIGHT)
        continue;

      DrawRectangle(screenSurface, p.box.x, p.box.y, p.box.w, p.box.h, red, red);
    }
  }

  void DrawPlayer() { 
    BetterDrawSurface(screenSurface, juanSurface, 0 , 300 - player.pos.y);
    DrawRectangle(screenSurface, player.box.x, player.box.y, player.box.w, player.box.h, green, black);
  }

  void Render() {

    // Clear screen
    SDL_FillRect(screenSurface, NULL, black);

    DrawUI();
    
    DrawBackground();

    DrawPlayer();

    DrawPlatforms();

    SDL_UpdateTexture(screenTexture, NULL, screenSurface->pixels, screenSurface->pitch);
    //		SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  void Loop() {
    // The frames per second timer
    LTimer fpsTimer;

    //The frames per second cap timer
    LTimer capTimer;

    // Start counting frames per second
    int countedFrames = 0;
    fpsTimer.start();

    // While application is running
    while (!quit) {
      // Handle events on queue
      capTimer.start();
      HandleEvents();

      // Handle physics
      Physics();

      // Calculate fps
      fps = countedFrames / (fpsTimer.getTicks() / (float) 1000);
      if (fps > 2000000) {
        fps = 0;
      }

      // Render
      Render();
      ++countedFrames;

      int frameTicks = capTimer.getTicks();
      if (frameTicks < SCREEN_TICKS_PER_FRAME) {
        SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
      }
    }
  }

 public:
  void Start() { 
    if (!Load()) return;
    if (!LoadMedia()) return;
    LoadLevel();

    Loop();

    Close();
  }
};