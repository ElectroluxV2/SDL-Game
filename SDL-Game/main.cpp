#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cfloat>
#include <cctype>
#include "functions.h"
#include "vector.h"
#include "player.h"
#include "game.h"

int main(int argc, char *args[]) {

  srand(time(NULL));
  Game g;
  g.Start();

  return 0;
}