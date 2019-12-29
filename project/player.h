#ifndef player_included
#define player_included

typedef struct player_s{
  void (*getBounds)(const struct player_s *player, const Vec2 *centerPos, Region *bounds);
  int (*check)(const struct player_s *shape, const Vec2 *centerPos, const Vec2 *pixel);
} player;

void playerGetBounds(const player *player, const Vec2 *center, Region *bounds);

int drawPlayer(const player *player, const Vec2 *center, const Vec2 *pixel);

#endif // included
