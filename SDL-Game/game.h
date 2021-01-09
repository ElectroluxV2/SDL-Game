const float RESISTANCE = 0.5; // 0.128
const float PLAYER_FORCE = 0.5; // 0.5
const float ACCELERATION_PER_TICK = 1; // 0.4

const float JUMP_FORCE = PLAYER_FORCE * 3;
const float GRAVITY_FORCE = JUMP_FORCE * 5;
const float MAX_JUMP_TIME = 0.2;  // Factor of 1 (fps based) second
const float MAX_DASH_TIME = 20;

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

const bool DEBUG = false;

int finishTime = 0;

class Game {
  const int SCREEN_WIDTH = 1920;
  const int SCREEN_HEIGHT = 1080;
  

  int deathCounter = 0;
  int bestProgres = 0;

  // Jump logic
  int ticksTimePressed = 0;
  bool preventToLongJump = false;

  // Dash logic
  int ticksPassed = 0;
  bool pressedOnce = false;

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

  // Struct of in game objects
  struct Platform {
    SDL_Rect box = { 0, 0, 0, 0 };
    int onMapPlacementX = 0;
  };

  // Platfrom array
  Vector<Platform> platforms;

  // Obstacles
  SDL_Surface* obstacleSurface = NULL;

  // Obstacles array
  Vector<Platform> obstacles;

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
    bool dashing = false;
    int jumpCount = 0;
    int dashCount = 0;
    int dashTimer = 0;
    int cooldownDash = 0;
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
      if (jumpCount >= 2 && !DEBUG) {
        return false;
      }

      acc.y += ACCELERATION_PER_TICK;

      return true;
    }

    void Dash() {
      if (dashing) return;
      printf("cooldown: %d\n", cooldownDash);
      if (cooldownDash > 100000) cooldownDash = 0;
      else return;
      dashing = true;
      acc.x += 50;
      dashTimer = 0;
    }

    void JumpPhysics(Vector<Platform> platforms, int platformCount) {
      float tmp = pos.y; // Do not change player pos before colision test
      if (!dashing) {    // if dashing ignore changes on y
        if (acc.y > 0) {
          // Jump
          tmp += JUMP_FORCE * acc.y;
          acc.y -= RESISTANCE;
        }
        else {
          // Fall
          tmp -= GRAVITY_FORCE;
          // Not in air anymore
          airBorn = false;
        }
      }
      

      // Remove left acceleration
      if (abs(acc.y) < RESISTANCE) acc.y = 0;

      // Change player's box y
      int playersRelativeY = box.y;

      box.y = -tmp + 300;

      // Check colision
      // Every platformG
      for (Platform p : platforms) {
        // Check if can pass by
        if (checkCollision(box, p.box)) {
          // Reenable jump on floor hit
          jumpCount = 0;
          
          // Restore player's box position
          box.y = playersRelativeY;
          // Remove any y acceleration on player
          acc.y = 0;
     
          // Prevent any changes to player's pos
          return;
        }
      }

      // Change position
      pos.y = tmp;
    }

    void MovePhysics(Vector<Platform> platforms, int platformCount) {
      if (!dashing) { // if dashing ignore changes on x
        if (acc.x > 0) acc.x -= RESISTANCE;
        else if (acc.x < 0) acc.x += RESISTANCE;
        if (abs(acc.x) < RESISTANCE) acc.x = 0;
        if (AreSame(acc.x, 0)) return;
      }

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

    void OnPhysics(Vector<Platform> platforms, int platformCount, int fps_ticks) {
      if (dashing) {
        if (dashTimer >= fps_ticks * MAX_DASH_TIME) {
          dashTimer = 0;
          dashing = false;
          dashCount++;
          jumpCount--;
        } else dashTimer += fps_ticks;
      } else cooldownDash += fps_ticks;
      JumpPhysics(platforms, platformCount);
      MovePhysics(platforms, platformCount);
    }

    void Reset() {
      pos.x = 0;
      pos.y = 50;
      acc.x = 0;
      acc.y = 0;
    }
  } player;

  void LongBoi(int howManyShortBoys, int x, int y) {
    for (int i = 0; i < howManyShortBoys; i++) {
      SetPlatform(x, y);
      x += platformSurface->w;
    }
  }

  void Physics(int fps_ticks) {
      // Follow player movement
      for (Platform& p : platforms) {
        int destX = p.onMapPlacementX - player.pos.x;
        p.box.x = destX;
      }

      // Folow on y axyis
      player.box.y = -player.pos.y + 300;

      if (player.pos.y < -800) {
        if ((player.pos.x / 13500) * 100.0 > bestProgres) bestProgres = (player.pos.x / 13500) * 100.0;
        startTime = SDL_GetTicks();
        player.Reset();
        deathCounter++;
      }
      if (!(player.pos.y < -800)) player.OnPhysics(platforms, 10, fps_ticks);
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

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

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
    if (!LoadOptimizedSurface("juan'splatform.bmp", &screenSurface, &platformSurface)) return false;

    // Add 15% offset
    player.Load(juanSurface->w, juanSurface->h, platformSurface->h * 0.15);

    printf("Successfully loaded media\n");
    return true;
  }

  void SetPlatform(int x, int y) {
    Platform p = {{x, y, platformSurface->w, platformSurface->h}, x};
    platforms.push_back(p);
  }

  void LoadLevel() {
    player.pos.y = 50;
    LongBoi(3, 0, 350);
    SetPlatform(70, 100);
    SetPlatform(250, 100);
    SetPlatform(3200, 400);
    SetPlatform(3400, 120);
    SetPlatform(5800, 250);
    SetPlatform(7620, 250);
    SetPlatform(8120, 100);
    LongBoi(2, 10720, 400);
    SetPlatform(10650, 150);
    SetPlatform(11050, 100);
    SetPlatform(11450, 150);
    LongBoi(10, 11850, 50);
    for (int i = 0; i < 25; i++) {
      SetPlatform(11850, 50 - (i * platformSurface->h));
    }
    SetPlatform(12000, 230);
    SetPlatform(13000, 230);
    SetPlatform(13500, 50);
    /*SetPlatform(400, 350);
    SetPlatform(800, 300);
    SetPlatform(1000, 250);
    SetPlatform(1400, 350);
    SetPlatform(1800, 270);
    SetPlatform(2200, 400);
    SetPlatform(2600, 350);
    SetPlatform(3000, 400);
    SetPlatform(3400, 300);*/
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

  bool chuj = false;

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
      }
    }
    if (key[SDL_SCANCODE_X]) {
      player.Dash();
    }
    if (key[SDL_SCANCODE_ESCAPE]) {
      quit = true;
    }
    if (key[SDL_SCANCODE_N]) {
      chuj = true;
      startTime = SDL_GetTicks();
      player.Reset();
    } else if (chuj) {
      chuj = false;
      deathCounter++;
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
    sprintf(text, "Death counter: %d, Best progress: %d, FPS: %.0f Time: %.0f sec", deathCounter, bestProgres, fps, (SDL_GetTicks() - startTime)/1000.0);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    sprintf(text, "Progres: %.0f%%", ((player.pos.x / 13500) * 100.0 > 100) ? 100 : (player.pos.x / 13500) * 100.0);
    DrawString(screenSurface, screenSurface->w / 4 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    if (player.pos.x > 13500 && player.pos.y >= 50) {
      if (finishTime == 0) {
        finishTime = (SDL_GetTicks() - startTime) / 1000.0;
      }
      sprintf(text, "jakim chujem to przeszedles, gratuluje... czas: %d sekundy", finishTime);
      DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8/2, 30, text, charsetSurface);

    }

    /*sprintf(text, "accelerationX: %.0f, posX: %.0f, ", player.acc.x, player.pos.x);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26, text, charsetSurface);
    sprintf(text, "FPS: %.0f Time: %.0f sec", fps, (SDL_GetTicks() - startTime) / 1000.0);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);*/

    /*sprintf(text, "accelerationY: %.0f, posY: %.0f,", player.acc.y, player.pos.y);
    sprintf(text, "accelerationY: %f,   posY: %f", player.acc.y, player.pos.y);
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
      BetterDrawSurface(screenSurface, platformSurface, p.box.x, p.box.y);

      // Display hit boxes for debug purposes
      if (DEBUG) {
        if (p.box.x < 0 || p.box.x + p.box.w > SCREEN_WIDTH || p.box.y < 0 || p.box.y + p.box.h > SCREEN_HEIGHT) continue;
        DrawRectangle(screenSurface, p.box.x, p.box.y, p.box.w, p.box.h, red, red);
      }
    }
  }

  void DrawPlayer() { 
    BetterDrawSurface(screenSurface, juanSurface, 0 , 300 - player.pos.y);
    if (DEBUG) {
      if (player.box.x < 0 || player.box.x + player.box.w > SCREEN_WIDTH || player.box.y < 0 || player.box.y + player.box.h > SCREEN_HEIGHT) return;
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

      // Handle physics
      Physics(tickTimer.getTicks() * (int)fps);
      tickTimer.start();

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