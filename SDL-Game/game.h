const float RESISTANCE = 0.5; // 0.128
const float PLAYER_FORCE = 0.5; // 0.5
const float ACCELERATION_PER_TICK = 1; // 0.4

const float JUMP_FORCE = PLAYER_FORCE * 3;
const float GRAVITY_FORCE = JUMP_FORCE * 5;
const float MAX_JUMP_TIME = 0.2;  // Factor of 1 (fps based) second

const bool DEBUG = false;

class Game {
  const int SCREEN_WIDTH = 640;
  const int SCREEN_HEIGHT = 480;

  const int SCREEN_FPS = 60;
  const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

  bool collisionDetected = false;

  // Jump logic
  int ticksTimePressed = 0;
  bool preventToLongJump = false;

  // The window we'll be rendering to
  SDL_Window* window = NULL;

  // The surface contained by the window
  SDL_Surface* screenSurface = NULL;

  // The image we will load and show on the screen
  SDL_Surface* mapSurface = NULL;

  // Contains all possible characters
  SDL_Surface* charsetSurface = NULL;

  // Platforms
  SDL_Surface* platformSurface = NULL;
  SDL_Surface* platformSurfaceWhenPlayerIsOnIt = NULL;

  // Platfrom array
  struct Platform {
    SDL_Rect box = { 0, 0, 0, 0 };
    int onMapPlacementX = 0;
    int state = 0;
  };
  Vector<Platform> platforms;

  struct Sprite {
   private:
    int state = 0;
    int counter = 0;

   public:
    int nextFrameEvery = 10;
    Vector<SDL_Surface*> surfaces;


    SDL_Surface* GetSurface() { 
      if (counter++ == nextFrameEvery) {
        state++;
        if (state >= surfaces.count) {
          state = 0;
        }
        counter = 0;
      }
     

      return *(surfaces.root + state);
    }
  };

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
    Sprite normalState;
    Sprite jumpState;
    Sprite fallState;
    int state = 0;

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

    void Load(int w, int h, int hitBoxOffset) {
      box.w = w - hitBoxOffset;
      box.h = h - hitBoxOffset;

      normalState.nextFrameEvery = 5;
      fallState.nextFrameEvery = 1;
      jumpState.nextFrameEvery = 3;
    }

    void AddAccelerationX(float f) {
      acc.x += f;
    }

    void SubAccelerationX(float f) {
      if (pos.x <= 0) return;
      acc.x -= f;
    }

    void IncrementJump() {
      jumpCount++;

      airBorn = true;
    }

    bool Jump() {
      // Can't jump while jumping
      if (airBorn) return false;

      // Can't jump more than 2 times in row
      if (jumpCount >= 2) {
        return false;
      }

      acc.y += ACCELERATION_PER_TICK;

      return true;
    }

    void JumpPhysics(Vector<Platform> platforms, int platformCount) {
      float tmp = pos.y; // Do not change player pos before colision test
      if (acc.y > 0) {
        // Jump
        state = 1;
        tmp += JUMP_FORCE * acc.y;
        acc.y -= RESISTANCE;
      } else {
        // Fall
        state = 2;
        tmp -= GRAVITY_FORCE;

        // Not in air anymore
        airBorn = false;
      }

      // Remove left acceleration
      if (abs(acc.y) < RESISTANCE) acc.y = 0;

      // Change player's box y
      int playersRelativeY = box.y;

      box.y = -tmp + 300;

      // Check colision
      // Every platformG
      for (Platform& p : platforms) {
        // Check if can pass by
        if (checkCollision(box, p.box)) {
          // Reenable jump on floor hit
          state = 0;
          jumpCount = 0;
          
          // Restore player's box position
          box.y = playersRelativeY;
          // Remove any y acceleration on player
          acc.y = 0;

          p.state = 1;
     
          // Prevent any changes to player's pos
          return;
        } else {
           p.state = 0;
        }
      }

      // Change position
      pos.y = tmp;
    }

    void MovePhysics(Vector<Platform> platforms, int platformCount) {
      if (acc.x > 0) acc.x -= RESISTANCE;
      else if (acc.x < 0) acc.x += RESISTANCE;
      if (abs(acc.x) < RESISTANCE) acc.x = 0;
      if (AreSame(acc.x, 0)) return;

      // Position player want to go
      float tmp = pos.x + (acc.x * PLAYER_FORCE);
      // Every platform
      for (Platform p : platforms) {
        // Relative postion of box
        int platformRelatvieX = p.box.x;
        p.box.x = p.onMapPlacementX - tmp;
        // Check if can pass by
        if (checkCollision(p.box, box)) {
          // Restore box position
          p.box.x = platformRelatvieX;
          // Remove any x acceleration on player
          acc.x = 0;
          return;
        }
      }
      // Change position
      pos.x = tmp;

      if (pos.x < 0) pos.x = 0;
      if (pos.x <= 0 && acc.x <= 0) acc.x = 0;
    }

    void OnPhysics(Vector<Platform> platforms, int platformCount) {
      JumpPhysics(platforms, platformCount);
      MovePhysics(platforms, platformCount);
    }

    void Reset() {
      pos.x = 0;
      pos.y = 0;
      acc.x = 0;
      acc.y = 0;
    }

    SDL_Surface* GetSurface() { 
      if (state == 0)
        return normalState.GetSurface();
      else if (state == 1)
        return jumpState.GetSurface();
      else
        return fallState.GetSurface();
    }
  } player;

  void Physics() {
      // Follow player movement
      for (Platform& p : platforms) {
        int destX = p.onMapPlacementX - player.pos.x;
        p.box.x = destX;
      }

      // Folow on y axyis
      player.box.y = -player.pos.y + 300;

      player.OnPhysics(platforms, 10);
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

    printf("Successfully loaded\n");
    return true;
  }

  bool LoadMedia() {
    if (!LoadSurface("cs8x8.bmp", &charsetSurface)) return false;
    SDL_SetColorKey(charsetSurface, true, 0x000000);

    if (!LoadOptimizedSurface("map.bmp", &screenSurface, &mapSurface)) return false;

    SDL_Surface* tmp{};
    if (!LoadOptimizedSurface("juan_normal_0.bmp", &screenSurface, &tmp)) return false;
    player.normalState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_normal_1.bmp", &screenSurface, &tmp)) return false;
    player.normalState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_normal_2.bmp", &screenSurface, &tmp)) return false;
    player.normalState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_normal_3.bmp", &screenSurface, &tmp)) return false;
    player.normalState.surfaces.push_back(tmp);

    if (!LoadOptimizedSurface("juan_jump_0.bmp", &screenSurface, &tmp)) return false;
    player.jumpState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_jump_1.bmp", &screenSurface, &tmp)) return false;
    player.jumpState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_jump_2.bmp", &screenSurface, &tmp)) return false;
    player.jumpState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_jump_3.bmp", &screenSurface, &tmp)) return false;
    player.jumpState.surfaces.push_back(tmp);

    if (!LoadOptimizedSurface("juan_fall_0.bmp", &screenSurface, &tmp)) return false;
    player.fallState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_fall_1.bmp", &screenSurface, &tmp)) return false;
    player.fallState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_fall_2.bmp", &screenSurface, &tmp)) return false;
    player.fallState.surfaces.push_back(tmp);
    if (!LoadOptimizedSurface("juan_fall_3.bmp", &screenSurface, &tmp)) return false;
    player.fallState.surfaces.push_back(tmp);

    if (!LoadOptimizedSurface("juan'splatform.bmp", &screenSurface, &platformSurface)) return false;
    if (!LoadOptimizedSurface("juan'splatform_but_angry.bmp", &screenSurface, &platformSurfaceWhenPlayerIsOnIt)) return false;

    // Add 15% offset
    player.Load(tmp->w, tmp->h, platformSurface->h * 0.15);
   
    printf("Successfully loaded media\n");
    return true;
  }

  void SetPlatform(int x, int y) {
    Platform p = {{x, y, platformSurface->w, platformSurface->h}, x};
    platforms.push_back(p);
  }

  void LoadLevel() {
    player.pos.y = 150;
    SetPlatform(0, 390);
    SetPlatform(400, 350);
    SetPlatform(800, 300);
    SetPlatform(1000, 250);
    SetPlatform(1400, 350);
    SetPlatform(1800, 270);
    SetPlatform(2200, 400);
    SetPlatform(2600, 350);
    SetPlatform(3000, 400);
    SetPlatform(3400, 300);
  }

  void Close() {
    // Deallocate surface
    FreeSurface(&mapSurface);
    FreeSurface(&charsetSurface);
    FreeSurface(&platformSurfaceWhenPlayerIsOnIt);

    for (SDL_Surface* s : player.normalState.surfaces) {
      FreeSurface(&s);
    }

    for (SDL_Surface* s : player.jumpState.surfaces) {
      FreeSurface(&s);
    }

    for (SDL_Surface* s : player.fallState.surfaces) {
      FreeSurface(&s);
    }

    // Destroy window
    SDL_DestroyWindow(window);
    window = NULL;

    // Quit SDL subsystems
    SDL_Quit();
    printf("Successfully closed\n");
  }

  void HandleEvents(int ticks) {
    // keyboard mocarz obluhuje
    const Uint8 *key = SDL_GetKeyboardState(NULL);
    if (key[SDL_SCANCODE_RIGHT]) {
      player.AddAccelerationX(ACCELERATION_PER_TICK);
    }
    if (key[SDL_SCANCODE_LEFT]) {
      player.SubAccelerationX(ACCELERATION_PER_TICK);
    }
    if (key[SDL_SCANCODE_Z]) {
      ticksTimePressed += ticks;
      if (!preventToLongJump) player.Jump();
   
      if (ticksTimePressed >= MAX_JUMP_TIME * fps * ticks && !preventToLongJump) {  // seconds in fps
        preventToLongJump = true;
      }
    } else {
      preventToLongJump = false;
      if (ticksTimePressed > ticks) {
        ticksTimePressed = 0;
        player.IncrementJump();
      };    
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

    /*sprintf(text, "accelerationX: %.0f, posX: %.0f, ", player.acc.x, player.pos.x);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26, text, charsetSurface);
    sprintf(text, "FPS: %.0f Time: %.0f sec", fps, (SDL_GetTicks() - startTime) / 1000.0);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    sprintf(text, "accelerationY: %.0f, posY: %.0f,", player.acc.y, player.GetPos().y);
    sprintf(text, "accelerationY: %f,   posY: %f", player.acc.y, player.GetPos().y);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26, text, charsetSurface);*/
  }

  void DrawBackground() {

   int x = -((int)player.pos.x % mapSurface->w);

    if (-x == mapSurface->w) {
      x = 0;
    }

    //printf("x: %d\ty: %d\n", x, 0);

    BetterDrawSurface(screenSurface, mapSurface, x, 46);
    BetterDrawSurface(screenSurface, mapSurface, x + mapSurface->w, 46);
  }

  void DrawPlatforms() {   
    for (Platform p : platforms) {
      // Box has relative to screen values
      BetterDrawSurface(screenSurface, p.state == 0 ? platformSurface : platformSurfaceWhenPlayerIsOnIt, p.box.x, p.box.y);

      // Display hit boxes for debug purposes
      if (DEBUG) {
        if (p.box.x < 0 || p.box.x + p.box.w > SCREEN_WIDTH || p.box.y < 0 || p.box.y + p.box.h > SCREEN_HEIGHT) continue;
        DrawRectangle(screenSurface, p.box.x, p.box.y, p.box.w, p.box.h, red, red);
      }
    }
  }

  void DrawPlayer() { 
    BetterDrawSurface(screenSurface, player.GetSurface(), 0 , 300 - player.pos.y);
    if (DEBUG) {
      DrawRectangle(screenSurface, player.box.x, player.box.y, player.box.w, player.box.h, green, black);
    }
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

    // The frames per second cap timer
    LTimer capTimer;

    // The time between ticks
    LTimer tickTimer;

    // Start counting frames per second
    int countedFrames = 0;
    fpsTimer.start();

    tickTimer.start();

    // While application is running
    while (!quit) {
      // Handle events on queue
      capTimer.start();

      HandleEvents(tickTimer.getTicks());
      tickTimer.start();

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

      // Capping the frame rate
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