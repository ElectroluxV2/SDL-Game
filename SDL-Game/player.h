struct Player {

  const float RESISTANCE = 0.128; // 0.128
  const float PLAYER_FORCE = 0.5; // 0.5
  const float ACCELERATION_PER_TICK = 10.4; // 0.4

  struct Coord {
    float x;
    float y;
  }acc, pos;

  void AddAccelerationX(float f) {
    if (pos.x >= 0) return;
    acc.x += f;
  }

  void SubAccelerationX(float f) {
    acc.x -= f;
  }

  Coord GetPos() {
    return pos;
  }

  void ResetPos() {
    pos.x = 0;
    pos.y = 0;
  }

  void OnPhysics() {
    if (acc.x > 0) acc.x -= RESISTANCE;
    else if (acc.x < 0) acc.x += RESISTANCE;

    if (acc.x == 0) return;

    pos.x += PLAYER_FORCE * acc.x;
    if (pos.x > 0) pos.x = 0;
  }
};