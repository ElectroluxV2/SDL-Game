// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
                SDL_Surface* charset) {
  int px, py, c;
  SDL_Rect s, d;
  s.w = 8;
  s.h = 8;
  d.w = 8;
  d.h = 8;
  while (*text) {
    c = *text & 255;
    px = (c % 16) * 8;
    py = (c / 16) * 8;
    s.x = px;
    s.y = py;
    d.x = x;
    d.y = y;
    SDL_BlitSurface(charset, &s, screen, &d);
    x += 8;
    text++;
  }
}

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
  SDL_Rect dest;
  dest.x = x - sprite->w / 2;
  dest.y = y - sprite->h / 2;
  dest.w = sprite->w;
  dest.h = sprite->h;
  SDL_BlitSurface(sprite, NULL, screen, &dest);
}

void BetterDrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
  SDL_Rect dest;
  dest.x = 0 + x;
  dest.y = y;
  dest.w = sprite->w;
  dest.h = sprite->h;
  SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
  int bpp = surface->format->BytesPerPixel;
  Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
  *(Uint32*)p = color;
};

// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1)
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0)
// line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy,
              Uint32 color) {
  for (int i = 0; i < l; i++) {
    DrawPixel(screen, x, y, color);
    x += dx;
    y += dy;
  };
};

// rysowanie prostokπta o d≥ugoúci bokÛw l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
                   Uint32 outlineColor, Uint32 fillColor) {

  DrawLine(screen, x, y, k, 0, 1, outlineColor);
  DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
  DrawLine(screen, x, y, l, 1, 0, outlineColor);
  DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
  for(int i = y + 1; i < y + k - 1; i++)
          DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

bool LoadOptimizedSurface(const char* path, SDL_Surface** screenSurface, SDL_Surface** dest) {
  // The final optimized image
  // Dest

  // Load image at specified path
  SDL_Surface* loadedSurface = SDL_LoadBMP(path);
  if (loadedSurface == NULL) {
    printf("Unable to load image %s! SDL Error: %s\n", path, SDL_GetError());
    return false;
  }

  // Convert surface to screen format
  *dest = SDL_ConvertSurface(loadedSurface, (*screenSurface)->format, 0);

  if (*dest == NULL) {
    printf("Unable to optimize image %s! SDL Error: %s\n", path, SDL_GetError());
    return false;
  }

  // Get rid of old loaded surface
  SDL_FreeSurface(loadedSurface);
 
  return true;
}

bool LoadSurface(const char* path, SDL_Surface** dest) {
  *dest = SDL_LoadBMP(path);
  if (*dest == NULL) {
    printf("Unable to load image %s! SDL Error: %s\n", path, SDL_GetError());
    return false;
  }

  return true;
}

void FreeSurface(SDL_Surface** surf) {
  SDL_FreeSurface(*surf);
  *surf = NULL;
}
 
// The application time based timer
class LTimer {
 public:
  // Initializes variables
  LTimer() {
    mStartTicks = 0;
    mPausedTicks = 0;

    mPaused = false;
    mStarted = false;
  }

  // The various clock actions
  void start() {
    mStarted = true;

    // Unpause the timer
    mPaused = false;

    // Get the current clock time
    mStartTicks = SDL_GetTicks();
    mPausedTicks = 0;
  }
  void stop() {
    mStarted = false;

    // Unpause the timer
    mPaused = false;

    // Clear tick variables
    mStartTicks = 0;
    mPausedTicks = 0;
  }
  void pause() {
    if (mStarted && !mPaused) {
      mPaused = true;

      mPausedTicks = SDL_GetTicks() - mStartTicks;
      mStartTicks = 0;
    }
  }
  void unpause() {
    if (mStarted && mPaused) {
      // Unpause the timer
      mPaused = false;

      // Reset the starting ticks
      mStartTicks = SDL_GetTicks() - mPausedTicks;

      // Reset the paused ticks
      mPausedTicks = 0;
    }
  }

  // Gets the timer's time
  Uint32 getTicks() {
    // The actual timer time
    Uint32 time = 0;

    // If the timer is running
    if (mStarted) {
      // If the timer is paused
      if (mPaused) {
        // Return the number of ticks when the timer was paused
        time = mPausedTicks;
      }
      else {
        // Return the current time minus the start time
        time = SDL_GetTicks() - mStartTicks;
      }
    }

    return time;
  }

  bool isStarted() {
    // Timer is running and paused or unpaused
    return mStarted;
  }
  bool isPaused() {
    // Timer is running and paused
    return mPaused && mStarted;
  }

 private:
  // The clock time when the timer started
  Uint32 mStartTicks;

  // The ticks stored when the timer was paused
  Uint32 mPausedTicks;

  // The timer status
  bool mPaused;
  bool mStarted;
};

bool checkCollision(SDL_Rect a, SDL_Rect b)
{
  //The sides of the rectangles
  int leftA, leftB;
  int rightA, rightB;
  int topA, topB;
  int bottomA, bottomB;

  //Calculate the sides of rect A
  leftA = a.x;
  rightA = a.x + a.w;
  topA = a.y;
  bottomA = a.y + a.h;

  //Calculate the sides of rect B
  leftB = b.x;
  rightB = b.x + b.w;
  topB = b.y;
  bottomB = b.y + b.h;

  //If any of the sides from A are outside of B
  if (bottomA <= topB)
  {
    return false;
  }

  if (topA >= bottomB)
  {
    return false;
  }

  if (rightA <= leftB)
  {
    return false;
  }

  if (leftA >= rightB)
  {
    return false;
  }

  //If none of the sides from A are outside B
  return true;
}

bool AreSame(double a, double b)
{
    return fabs(a - b) < FLT_EPSILON;
}