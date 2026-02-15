#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

typedef struct {
  i32 x;
  i32 y;
} Vec2i32;

typedef struct {
  Texture2D texture;
  u32 cell_size;
  u32 cols;
  f32 scale;
} TextureAtlas;

typedef enum { STATE_PLAYING, STATE_LOST, STATE_WON, STATE_INFINITE } GameState;

typedef struct {
  Vec2i32 size;
  u32 **board;
  u32 **revealed;
  GameState state;
} Game;

typedef enum {
  CELL_EMPTY = 0,
  CELL_ONE = 1,
  CELL_TWO = 2,
  CELL_THREE = 3,
  CELL_FOUR = 4,
  CELL_FIVE = 5,
  CELL_SIX = 6,
  CELL_SEVEN = 7,
  CELL_EIGHT = 8,
  CELL_HIDDEN = 9,
  CELL_FLAG = 10,
  CELL_BOMB = 12,
  CELL_BOMB_EXPLODED = 11
} CellType;

void game_init(Game *game, u32 width, u32 height);
void game_free(Game *game);
void map_generate(Game *game, i32 seed);
void cell_uncover(Game *game, i32 x, i32 y);
void cell_draw(TextureAtlas textureAtlas, u32 index, Vector2 pos, i32 padding);

i32 main(i32 argc, char *argv[]) {

  Game game;
  game_init(&game, 16, 16);

  TextureAtlas textureAtlas = {.cell_size = 32, .cols = 8, .scale = 1.5f};

  i32 padding = 32;
  u32 scaled_cell_size = textureAtlas.cell_size * textureAtlas.scale;

  u32 screen_width = scaled_cell_size * game.size.x + padding * 2;
  u32 screen_height = scaled_cell_size * game.size.y + padding * 2;

  InitWindow(screen_width, screen_height, "minesweeper");

  textureAtlas.texture = LoadTexture("atlas2.png");

  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

  map_generate(&game, 300);

  Color bg_color = {200, 200, 200, 255};

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(bg_color);

    for (u32 y = 0; y < game.size.y; y++) {
      for (u32 x = 0; x < game.size.x; x++) {
        cell_draw(textureAtlas, game.revealed[x][y], (Vector2){x, y}, padding);
      }
    }

    Vec2i32 cell_coords = {.x = (GetMousePosition().x - padding) / scaled_cell_size,
                           .y = (GetMousePosition().y - padding) / scaled_cell_size};
    Vec2i32 current_cell;

    switch (game.state) {
    case STATE_PLAYING:

      if (cell_coords.x >= 0 && cell_coords.x <= game.size.x && cell_coords.y >= 0 && cell_coords.y <= game.size.y) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          cell_uncover(&game, cell_coords.x, cell_coords.y);

          if (game.revealed[cell_coords.x][cell_coords.y] != CELL_FLAG) {
            game.revealed[cell_coords.x][cell_coords.y] = game.board[cell_coords.x][cell_coords.y];
          }

          if (game.revealed[cell_coords.x][cell_coords.y] == CELL_BOMB) {
            current_cell = cell_coords;
            game.state = STATE_LOST;
          }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
          if (game.revealed[cell_coords.x][cell_coords.y] == CELL_FLAG) {
            game.revealed[cell_coords.x][cell_coords.y] = CELL_HIDDEN;
          } else if (game.revealed[cell_coords.x][cell_coords.y] == CELL_HIDDEN) {
            game.revealed[cell_coords.x][cell_coords.y] = CELL_FLAG;
          }
        }
      }

      break;
    case STATE_LOST:
      bg_color = (Color){255, 0, 0, 255};

      for (u32 y = 0; y < game.size.y; y++) {
        for (u32 x = 0; x < game.size.x; x++) {
          if (game.board[x][y] == CELL_BOMB && game.revealed[x][y] == CELL_HIDDEN) {
            game.revealed[x][y] = game.board[x][y];
          }
        }
      }

      game.revealed[current_cell.x][current_cell.y] = CELL_BOMB_EXPLODED;

      game.state = STATE_INFINITE;
      break;
    case STATE_WON:
      DrawText("You Won :)", 0, 0, 16, BLACK);
      game.state = STATE_INFINITE;
      break;
    case STATE_INFINITE:
      break;
    }

    EndDrawing();
  }

  game_free(&game);
  UnloadTexture(textureAtlas.texture);
  CloseWindow();
  return 0;
}

void game_init(Game *game, u32 width, u32 height) {
  game->size = (Vec2i32){width, height};
  game->board = malloc(sizeof(u32 *) * width);
  game->revealed = malloc(sizeof(u32 *) * width);
  game->state = STATE_PLAYING;

  for (u32 x = 0; x < width; x++) {
    game->board[x] = malloc(sizeof(u32) * height);
    game->revealed[x] = malloc(sizeof(u32) * height);
  }
}

void game_free(Game *game) {
  for (u32 x = 0; x < game->size.x; x++) {
    free(game->board[x]);
    free(game->revealed[x]);
  }

  free(game->board);
  free(game->revealed);
  free(game);
}

void map_generate(Game *game, i32 seed) {

  SetRandomSeed(seed);

  for (u32 y = 0; y < game->size.y; y++) {
    for (u32 x = 0; x < game->size.x; x++) {
      game->revealed[x][y] = CELL_HIDDEN;
      if (GetRandomValue(0, 5) == 0) {
        game->board[x][y] = CELL_BOMB;
      } else {
        game->board[x][y] = 0;
      }
    }
  }

  for (u32 y = 0; y < game->size.y; y++) {
    for (u32 x = 0; x < game->size.x; x++) {

      if (game->board[x][y] != CELL_BOMB)
        continue;

      for (i32 dy = -1; dy <= 1; dy++) {
        for (i32 dx = -1; dx <= 1; dx++) {

          if (dx == 0 && dy == 0)
            continue;

          i32 nx = (i32)x + dx;
          i32 ny = (i32)y + dy;

          if (nx < 0 || nx >= (i32)game->size.x || ny < 0 || ny >= (i32)game->size.y)
            continue;

          if (game->board[nx][ny] != CELL_BOMB) {
            game->board[nx][ny]++;
          }
        }
      }
    }
  }
}

void cell_uncover(Game *game, i32 x, i32 y) {
  if (x < 0 || x >= game->size.x || y < 0 || y >= game->size.y)
    return;

  if (game->revealed[x][y] != CELL_HIDDEN)
    return;

  game->revealed[x][y] = game->board[x][y];

  if (game->board[x][y] == CELL_EMPTY) {
    for (i32 dy = -1; dy <= 1; dy++) {
      for (i32 dx = -1; dx <= 1; dx++) {
        if (dx == 0 && dy == 0)
          continue;
        cell_uncover(game, x + dx, y + dy);
      }
    }
  }
}

void cell_draw(TextureAtlas textureAtlas, u32 index, Vector2 pos, i32 padding) {
  u32 scaled_cell_size = textureAtlas.cell_size * textureAtlas.scale;

  BeginScissorMode(padding + pos.x * scaled_cell_size, padding + pos.y * scaled_cell_size, scaled_cell_size,
                   scaled_cell_size);

  DrawTextureEx(textureAtlas.texture,
                (Vector2){padding + pos.x * scaled_cell_size - ((index % textureAtlas.cols) * scaled_cell_size),
                          padding + pos.y * scaled_cell_size - ((u32)(index / textureAtlas.cols) * scaled_cell_size)},
                0.0f, textureAtlas.scale, WHITE);

  EndScissorMode();
}
