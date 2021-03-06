const float RESISTANCE = 0.5;
const float DOLPHIN_RESISTANCE = 0.01;
const float PLAYER_FORCE = 0.3;
const float DOLPHIN_FORCE = 0.05;
const float ACCELERATION_PER_TICK = 1;

const float JUMP_FORCE = PLAYER_FORCE * 3;
const float GRAVITY_FORCE = JUMP_FORCE * 5;
const float MAX_JUMP_TIME = 0.2;  // Factor of 1 (fps based) second
const float MAX_DASH_TIME = 20;
const int DOLPHIN_SPEED = 3;

const bool DEBUG = false;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

const int BUFFER_SIZE = 255;

struct Boundaries {
  int minX, maxX, minY, maxY;
};
Boundaries dolphinBoundaries {0, SCREEN_WIDTH, SCREEN_HEIGHT - 100, SCREEN_HEIGHT};

class Game {
  // Physic logic 
  bool collisionDetected = false;

  // Jump logic
  int ticksTimePressed = 0;
  bool preventToLongJump = false;

  // Dash logic
  int ticksPassed = 0;

  // The window we'll be rendering to
  SDL_Window* window = NULL;

  // The surface contained by the window
  SDL_Surface* screenSurface = NULL;

  // The image we will load and show on the screen
  SDL_Surface* mapSurface = NULL;

  // Contains all possible characters
  SDL_Surface* charsetSurface = NULL;
   
  // Conatainer for sprites (animations) 
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
      return surfaces.Get(state);
    }
  };

  // Platforms
  SDL_Surface* platformSurface = NULL;
  SDL_Surface* platformSurfaceWhenPlayerIsOnIt = NULL;

  // Platfroms array
  struct Object {
    SDL_Rect box = {0, 0, 0, 0};
    int onMapPlacementX = 0;
    int state = 0;
  };
  Vector<Object> platforms;

  // Obstacles
  Sprite angryCat;

  // Obstacles array
  Vector<Object> obstacles;

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

  // width of a stage
  bool countWidth = true;
  int width = 0;
  int stage = 0;

  // false -> steering with arrows, true -> automatic movment
  bool mode = false;

  // Camera follow player on Y axis
  int ypad = 0;
  int tagretypad = 0; // But smooth 

  // Dolphins
  Sprite dolphinSprite;
  unsigned dolphinCount = 0;
  struct Dolphin {
    Sprite sprite; // Contains only own timer and pointers to surfaces
    int moveTimer = 0;
    struct Coord {
      float x;
      float y;
    } acc, pos;

    void Update(int timeUnit) {
      // Do not add acceleration every frame
      moveTimer += timeUnit;
      if (moveTimer >= 5000) {
        moveTimer = 0;

        // Random movement
        acc.x += ACCELERATION_PER_TICK * (float((rand() % DOLPHIN_SPEED)));
        acc.x -= ACCELERATION_PER_TICK * (float((rand() % DOLPHIN_SPEED)));
        acc.y += ACCELERATION_PER_TICK * (float((rand() % DOLPHIN_SPEED)));
        acc.y -= ACCELERATION_PER_TICK * (float((rand() % DOLPHIN_SPEED)));

        // printf("x: %f, y: %f\n", acc.x, acc.y);
      }

      // Acceleration
      if (acc.x > 0) acc.x -= DOLPHIN_RESISTANCE;
      else if (acc.x < 0) acc.x += DOLPHIN_RESISTANCE;
      if (abs(acc.x) < DOLPHIN_RESISTANCE) acc.x = 0;

      if (acc.y > 0) acc.y -= DOLPHIN_RESISTANCE;
      else if (acc.y < 0) acc.y += DOLPHIN_RESISTANCE;
      if (abs(acc.y) < DOLPHIN_RESISTANCE) acc.y = 0;

      // Change pos
      pos.x += DOLPHIN_FORCE * acc.x;
      pos.y += DOLPHIN_FORCE * acc.y;

      // Up to boundaries
      if (pos.x < dolphinBoundaries.minX) {
        pos.x = dolphinBoundaries.minX;
        acc.x = -acc.x;
      }

      if (pos.x + sprite.surfaces.Get(0)->w > dolphinBoundaries.maxX) {
        pos.x = dolphinBoundaries.maxX - sprite.surfaces.Get(0)->w;
        acc.x = -acc.x;
      }

      if (pos.y < dolphinBoundaries.minY) {
        pos.y = dolphinBoundaries.minY;
        acc.y = -acc.y;
      }

      if (pos.y + sprite.surfaces.Get(0)->h > dolphinBoundaries.maxY) {
        pos.y = dolphinBoundaries.maxY - sprite.surfaces.Get(0)->h;
        acc.y = -acc.y;
      }
    }
  };
  Vector<Dolphin> dolphins;

  // Player x
  struct Player {
    Sprite normalState;
    Sprite jumpState;
    Sprite fallState;
    Sprite dashState;
    int state = 0;

    bool airBorn = false;
    bool dashing = false;
    int jumpCount = 0;
    int dashCount = 0;
    int dashTimer = 0;
    int cooldownDash = 0;
    int score = 0;
    int obstaclesDestoryed = 0;

    SDL_Rect box = {0, 0, 0, 0};

    struct Coord {
      float x;
      float y;
    } acc, pos{0, -20};
    
    void Load(int w, int h, int hitBoxOffset) {
      box.w = w - hitBoxOffset;
      box.h = h - hitBoxOffset;

      normalState.nextFrameEvery = 5;
      fallState.nextFrameEvery = 1;
      jumpState.nextFrameEvery = 3;
      dashState.nextFrameEvery = 4;
    }

    void Die() {
      score = 0;
      Reset();
    }

    void AddAccelerationX(float f) {
      if (acc.x >= 100) return;
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
      if (cooldownDash > 100000) cooldownDash = 0;
      else return;
      dashing = true;
      state = 3;
      acc.x += 50;
      dashTimer = 0;
    }
    
    void JumpPhysics(Vector<Object*> platforms) {
      float tmp = pos.y;  // Do not change player pos before colision test
      if (!dashing) {     // if dashing ignore changes on y
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
      }

      // Remove left acceleration
      if (abs(acc.y) < RESISTANCE) acc.y = 0;

      // Change player's box y
      int playersRelativeY = box.y;

      box.y = -tmp + 300;

      // Check colision
      // Every platformG
      for (Object* p : platforms) {
        // Check if can pass by
        if (checkCollision(box, p->box)) {
          // Reenable jump on floor hit
          jumpCount = 0;

          // When player is not on platform
          state = 0;

          // Restore player's box position
          box.y = playersRelativeY;
          // Remove any y acceleration on player
          acc.y = 0;

          // When player is on platform
          p->state = 1;

          // Prevent any changes to player's pos
          return;
        } else {
          p->state = 0;
        }
      }

      // Change position
      pos.y = tmp;
    }

    void MovePhysics(Vector<Object*> platforms) {
      if (!dashing) {  // if dashing ignore changes on x
        if (acc.x > 0)
          acc.x -= RESISTANCE;
        else if (acc.x < 0)
          acc.x += RESISTANCE;
        if (abs(acc.x) < RESISTANCE) acc.x = 0;
        if (AreSame(acc.x, 0)) return;
      }

      // Position player want to go
      float tmp = pos.x + (acc.x * PLAYER_FORCE) * 0.6;
      // Every platform
      for (Object* p : platforms) {
        // Relative postion of box
        int platformRelatvieX = p->box.x;
        p->box.x = p->onMapPlacementX - tmp;
        // Check if can pass by
        if (checkCollision(p->box, box)) {
          // Restore box position
          p->box.x = platformRelatvieX;
          // Remove any x acceleration on player
          acc.x = 0;
          Die();
          return;
        }
      }
      // Change position
      pos.x = tmp;

      if (pos.x < 0) pos.x = 0;
      if (pos.x <= 0 && acc.x <= 0) acc.x = 0;
    }
    
    void DashPhysics(Vector<Object> obstacles) {
      float tmp = pos.x + (acc.x * PLAYER_FORCE);

      for (Object o : obstacles) {
        // Relative postion of box
        int platformRelatvieX = o.box.x;
        o.box.x = o.onMapPlacementX - tmp;
        // Check if can pass by
        if (checkCollision(o.box, box) && !dashing) {
          o.box.x = platformRelatvieX;
          Die();
        } 
      }
    }

    void OnPhysics(Vector<Object*> platforms, Vector<Object> obstacles, int timeUnit) {
      if (dashing) {
        if (dashTimer >= timeUnit * MAX_DASH_TIME) {
          dashTimer = 0;
          dashing = false;
          acc.x -= 50;
          dashCount++;
          jumpCount--;
        } else dashTimer += timeUnit;
      } else cooldownDash += timeUnit;
      DashPhysics(obstacles);
      JumpPhysics(platforms);
      MovePhysics(platforms);
    }

    void Reset() {
      pos.x = 0;
      pos.y = -20;
      acc.x = 0;
      acc.y = 0;
    }

    SDL_Surface* GetSurface() {
      if (state == 0)
        return normalState.GetSurface();
      else if (state == 1)
        return jumpState.GetSurface();
      else if (state == 2)
        return fallState.GetSurface();
      else
        return dashState.GetSurface();
    }
  } player;

  void Physics(int timeUnit) {
    int tmp = -player.pos.y + 300 - SCREEN_HEIGHT + 300;

    if (player.box.y - ypad > 350) tagretypad = tmp;
    if (player.box.y - ypad < 50) tagretypad = tmp;

    if (tagretypad > ypad)
      ypad += 0.1 * abs(ypad - tagretypad);
    else if (tagretypad < ypad)
      ypad -= 0.1 * abs(ypad - tagretypad);
    else
      ypad = tagretypad;

    if (abs(ypad - tagretypad) < 0.1 * abs(ypad - tagretypad))
      ypad = tagretypad;


    Vector<Object*> toCheck;
    // Follow player movement
    for (Object& p : platforms) {
      int destX = p.onMapPlacementX - player.pos.x;
      p.box.x = destX;

      if (p.box.x + p.box.w < 0) continue;
      if (p.box.x - p.box.w > SCREEN_WIDTH) continue;

      if (p.box.y - ypad < 0) continue;
      if (p.box.y - ypad > SCREEN_HEIGHT) continue;

      toCheck.push_back(&p);
    }
    for (Object& o : obstacles) {
      int destX = o.onMapPlacementX - player.pos.x;
      o.box.x = destX;
    }

    // Folow on y axyis
    player.box.y = -player.pos.y + 300;
    player.OnPhysics(toCheck, obstacles, timeUnit);

    int calc = width - SCREEN_WIDTH + stage * (width - SCREEN_WIDTH);
    if (player.pos.x > calc) {
      stage++;
      ReadConfig("config.yml", width * stage);
    }

    if (player.pos.y < -400) {
      player.Reset();
    }

    // Calc score
    int aScore = (int)player.pos.x / 100;
    if (aScore > player.score)
      player.score = aScore;

    // Dolphin count is depended on score
    int dolphinScore = 0;
  }

  void ReadConfig(const char* filename, int multi = 0) {
    FILE* config = fopen(filename, "r");
    if (!config) printf("Config file not found!\n");

    char buffer[BUFFER_SIZE];
    bool xLoaded = false, yLoaded = false, qLoaded = false;
    int action = 0, x = 0, y = 0, quantity = 0;
    int deltaX = 0, endX = 0;
    while (fgets(buffer, BUFFER_SIZE, config)) {
      switch (buffer[0]) {
      case '#':
        continue;
        break;

      case 's':
        action = 1;
        break;

      case 'l':
        action = 2;
        break;

      case 'X':
        action = 3;
        break;

      case 'Y':
        action = 4;
        break;

      case '\n':
        xLoaded = false;
        yLoaded = false;
        qLoaded = false;
        break;
      }
      if (buffer[2] == '-') {
        x = atoi(buffer + 3) + multi;
        xLoaded = true;
      } else if (xLoaded && !yLoaded) {
        y = atoi(buffer + 3);
        yLoaded = true;
        // printf("x: %d, y: %d\n", x, y);
      } else if (yLoaded && (action == 2 || action == 4)) {
        quantity = atoi(buffer + 3);
        qLoaded = true;
        // printf("Long platform placed!\n");
        if (action == 2) {
          LongBoi(quantity, x, y);
          deltaX = x - endX;
          if(countWidth) width += quantity * platformSurface->w + deltaX; 
          endX = x + quantity * platformSurface->w;
        }
        if (action == 4) SetStalactite(quantity, x, y);
      }

      if (xLoaded && yLoaded && !qLoaded) {
        if (action == 1) {
          // printf("Platform placed!\n");
          SetPlatform(x, y);
        }

        if (action == 3) {
          // printf("Obstacle placed!\n");
          SetObstacle(x, y);
        }
      }
    }
    printf("width: %d\n", width);
    countWidth = false;
    fclose(config);
  }

  bool Load() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("SDL_Init error: %s\n", SDL_GetError());
      return false;
    }

    // Create window
    window = SDL_CreateWindow("Robot unicorn atack", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
      printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
      return false;
    }

    // renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED |
    // SDL_RENDERER_PRESENTVSYNC);
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
    screenSurface =
        SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000,
                             0x0000FF00, 0x000000FF, 0xFF000000);

    // Get screen texture
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                                      SCREEN_HEIGHT);

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
    // charset
    if (!LoadSurface("cs8x8.bmp", &charsetSurface)) return false;
    SDL_SetColorKey(charsetSurface, true, 0x000000);

    // background
    if (!LoadOptimizedSurface("map.bmp", &screenSurface, &mapSurface)) return false;

    // players animation
    if (!LoadOptimizedSurface("juan_normal_0.bmp", &screenSurface, player.normalState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_normal_1.bmp", &screenSurface, player.normalState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_normal_2.bmp", &screenSurface, player.normalState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_normal_3.bmp", &screenSurface, player.normalState.surfaces.Next())) return false;

    if (!LoadOptimizedSurface("juan_jump_0.bmp", &screenSurface, player.jumpState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_jump_1.bmp", &screenSurface, player.jumpState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_jump_2.bmp", &screenSurface, player.jumpState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_jump_3.bmp", &screenSurface, player.jumpState.surfaces.Next())) return false;

    if (!LoadOptimizedSurface("juan_fall_0.bmp", &screenSurface, player.fallState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_fall_1.bmp", &screenSurface, player.fallState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_fall_2.bmp", &screenSurface, player.fallState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_fall_3.bmp", &screenSurface, player.fallState.surfaces.Next())) return false;

    if (!LoadOptimizedSurface("juan_dash_0.bmp", &screenSurface, player.dashState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_dash_1.bmp", &screenSurface, player.dashState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_dash_2.bmp", &screenSurface, player.dashState.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("juan_dash_3.bmp", &screenSurface, player.dashState.surfaces.Next())) return false;

    // platform animation
    if (!LoadOptimizedSurface("juan'splatform.bmp", &screenSurface, &platformSurface)) return false;
    if (!LoadOptimizedSurface("juan'splatform.bmp", &screenSurface, &platformSurface)) return false;
    if (!LoadOptimizedSurface("juan'splatform_but_angry.bmp", &screenSurface, &platformSurfaceWhenPlayerIsOnIt)) return false;

    // dolphin animation
    if (!LoadOptimizedSurface("d0.bmp", &screenSurface, dolphinSprite.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("d1.bmp", &screenSurface, dolphinSprite.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("d2.bmp", &screenSurface, dolphinSprite.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("d3.bmp", &screenSurface, dolphinSprite.surfaces.Next())) return false;

    // obstacle animation
    if (!LoadOptimizedSurface("angry_cat_0.bmp", &screenSurface, angryCat.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("angry_cat_1.bmp", &screenSurface, angryCat.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("angry_cat_2.bmp", &screenSurface, angryCat.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("angry_cat_3.bmp", &screenSurface, angryCat.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("angry_cat_4.bmp", &screenSurface, angryCat.surfaces.Next())) return false;
    if (!LoadOptimizedSurface("angry_cat_5.bmp", &screenSurface, angryCat.surfaces.Next())) return false;

    // Add 15% offset
    player.Load(player.normalState.surfaces.Get(0)->w, player.normalState.surfaces.Get(0)->h, platformSurface->h * 0.15);

    printf("Successfully loaded media\n");
    return true;
  }

  void SetPlatform(int x, int y) {
    Object p = {{x, y, platformSurface->w, platformSurface->h}, x};
    platforms.push_back(p);
  }

  // function to create set of platforms with equal y
  void LongBoi(int howManyShortBoys, int x, int y) {
    for (int i = 0; i < howManyShortBoys; i++) {
      SetPlatform(x, y);
      x += platformSurface->w;
    }
  }

  void SetObstacle(int x, int y) {
    Object o = {{x, y, angryCat.surfaces[0]->w, angryCat.surfaces[0]->h}, x};
    obstacles.push_back(o);
    angryCat.nextFrameEvery += obstacles.count;
  }

  void SetStalactite(int count, int x, int y) {
    for (int i = 0; i < count; i++) {
      SetPlatform(x, y);
      y += angryCat.surfaces[0]->h;
    }
  }

/*void LoadLevel() {
    player.pos.y = 150;
    SetPlatform(0, 1000);
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
    for (int i = 0; i < 100; i++) {
      for (int j = 0; j < 100; j++) {
        // printf("Y: %i\n", 400 - (25 * j));
        SetPlatform(50 * j + platformSurface->w * i, 400 - (25 * j) + 1000);
      }
    }
     LongBoi(100, 0, 380);
  }*/
  // using config file ("config.yml") to load level

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

    for (SDL_Surface* s : angryCat.surfaces) {
      FreeSurface(&s);
    }

    for (SDL_Surface *s : dolphinSprite.surfaces) {
      FreeSurface(&s);
    }

    // Destroy window
    SDL_DestroyWindow(window);
    window = NULL;

    // Quit SDL subsystems
    SDL_Quit();
    printf("Successfully closed\n");
  }

  bool pressedOnce = false;
  int timeToAccelerateAutoMode = 0;
  void HandleEvents(int ticks) {
    // keyboard event handle
    const Uint8* key = SDL_GetKeyboardState(NULL);
    if (key[SDL_SCANCODE_D]) {
      pressedOnce = true;
    } else if (pressedOnce) {
      mode = !mode;
      pressedOnce = false;
    }
    if (key[SDL_SCANCODE_RIGHT] || mode) {
      if (mode) {
        timeToAccelerateAutoMode += ticks;
        if (timeToAccelerateAutoMode > 15) {
          player.AddAccelerationX(ACCELERATION_PER_TICK);
          timeToAccelerateAutoMode = 0;
        }
      } else {
        player.AddAccelerationX(ACCELERATION_PER_TICK);
      }
    }
    if (key[SDL_SCANCODE_LEFT]) {
      player.SubAccelerationX(ACCELERATION_PER_TICK);
    }
    if (key[SDL_SCANCODE_Z]) {
      ticksTimePressed += ticks;
      if (!preventToLongJump) player.Jump();

      if (ticksTimePressed >= MAX_JUMP_TIME * fps * ticks &&
          !preventToLongJump) {  // seconds in fps
        preventToLongJump = true;
      }
    } else {
      preventToLongJump = false;
      if (ticksTimePressed > ticks) {
        ticksTimePressed = 0;
        player.IncrementJump();
      };
    }
    if (key[SDL_SCANCODE_X]) {
      player.Dash();
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
    DrawRectangle(screenSurface, 0, 0, SCREEN_WIDTH, 45, black, black);
    DrawRectangle(screenSurface, 4, 4, SCREEN_WIDTH - 8, 36, green, black);

    char text[128];
    sprintf(text, "FPS: %.0f Time: %.0f sec", fps,
            (SDL_GetTicks() - startTime) / 1000.0);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 10,
               text, charsetSurface);

    sprintf(text, "score: %i", player.score);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26,
               text, charsetSurface);

    /*sprintf(text, "accelerationX: %.0f, posX: %.0f, ", player.acc.x,
    player.pos.x); DrawString(screenSurface, screenSurface->w / 2 - strlen(text)
    * 8 / 2, 26, text, charsetSurface); sprintf(text, "FPS: %.0f Time: %.0f
    sec", fps, (SDL_GetTicks() - startTime) / 1000.0); DrawString(screenSurface,
    screenSurface->w / 2 - strlen(text) * 8 / 2, 10, text, charsetSurface);

    sprintf(text, "accelerationY: %.0f, posY: %.0f,", player.acc.y,
    player.GetPos().y); sprintf(text, "accelerationY: %f,   posY: %f",
    player.acc.y, player.GetPos().y);
    DrawString(screenSurface, screenSurface->w / 2 - strlen(text) * 8 / 2, 26,
    text, charsetSurface);*/
  }

  void DrawBackground() {
    int x = -((int)player.pos.x % mapSurface->w);

    if (-x == mapSurface->w) {
      x = 0;
    }

    BetterDrawSurface(screenSurface, mapSurface, x, 46);
    BetterDrawSurface(screenSurface, mapSurface, x + mapSurface->w, 46);
  }

  void DrawPlatforms() {
    for (Object p : platforms) {
      // Box has relative to screen values
      BetterDrawSurface(
          screenSurface,
          p.state == 0 ? platformSurface : platformSurfaceWhenPlayerIsOnIt,
          p.box.x, p.box.y - ypad);

      // Display hit boxes for debug purposes
      if (DEBUG) {
        if (p.box.x < 0 || p.box.x + p.box.w > SCREEN_WIDTH || p.box.y - ypad < 0 || p.box.y - ypad + p.box.h > SCREEN_HEIGHT)
          continue;
        DrawRectangle(screenSurface, p.box.x, p.box.y - ypad, p.box.w, p.box.h, red,
                      red);
      }
    }
  }

  void DrawObstacle() {
    for (Object o : obstacles) {
      BetterDrawSurface(screenSurface, angryCat.GetSurface(), o.box.x, o.box.y - ypad);

      if (DEBUG) {
        if (o.box.x < 0 || o.box.x + o.box.w > SCREEN_WIDTH || o.box.y < 0 ||
          o.box.y + o.box.h > SCREEN_HEIGHT)
          continue;
        DrawRectangle(screenSurface, o.box.x, o.box.y, o.box.w, o.box.h, red,
          red);
      }
    }
  }

  void DrawPlayer() {
    BetterDrawSurface(screenSurface, player.GetSurface(), 0, 300 - player.pos.y - ypad);
    if (DEBUG) {
      if (player.box.x < 0 || player.box.x + player.box.w > SCREEN_WIDTH ||
          player.box.y - ypad < 0 ||
          player.box.y + player.box.h - ypad > SCREEN_HEIGHT)
        return;
      DrawRectangle(screenSurface, player.box.x, player.box.y - ypad,
                    player.box.w, player.box.h, green, black);
    }
  }

  void DrawDolphins() {
    for (Dolphin& d : dolphins) {
      BetterDrawSurface(screenSurface, d.sprite.GetSurface(), d.pos.x, d.pos.y);
    }
  }

  void HandleDolphins(int timeUnit) { 
    // Handle change of dolphins
    int mDolphinCount = player.score / 100;
    if (mDolphinCount > dolphinCount) {
      // Add new dolphins
      for (int i = 0; i < mDolphinCount - dolphinCount; i++) {
        printf("Added dolphin\n");
        Dolphin d{};
        d.sprite.surfaces = dolphinSprite.surfaces; // Make copy
        dolphins.push_back(d);
      }

      dolphinCount = mDolphinCount;
    } else if (mDolphinCount < dolphinCount) {
      dolphins.Erase();
    }

    // Random movement
    for (Dolphin& d : dolphins) {
      d.Update(timeUnit);
    }
  }

  void Render() {
    // Clear screen
    SDL_FillRect(screenSurface, NULL, black);

    DrawBackground();

    DrawPlayer();

    DrawPlatforms();


    DrawObstacle();

    DrawDolphins();


    DrawUI();

    SDL_UpdateTexture(screenTexture, NULL, screenSurface->pixels,
                      screenSurface->pitch);
    //		SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  void Loop() {
    // The frames per second timer
    Timer fpsTimer;

    // The frames per second cap timer
    Timer capTimer;

    // The time between ticks
    Timer tickTimer;

    // Start counting frames per second
    int countedFrames = 0;
    fpsTimer.start();

    tickTimer.start();

    // While application is running
    while (!quit) {
      // Handle events on queue
      capTimer.start();
      HandleEvents(tickTimer.getTicks());

      int timeUnit = tickTimer.getTicks() * (int)fps;

      // Handle physics
      Physics(timeUnit);

      // Handle dolphins
      HandleDolphins(timeUnit);
      tickTimer.start();

      // Calculate fps
      fps = countedFrames / (fpsTimer.getTicks() / (float)1000);
      if (fps > 2000000) { // prevention of fps being too large
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
    ReadConfig("config.yml");
    // LoadLevel();

    Loop();

    Close();
  }
};