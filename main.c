#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
  u32 mines;
  i32 mines_left;
  i32 mines_wrong;
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
  CELL_MINE_EXPLODED = 11,
  CELL_MINE = 12,
  CELL_FLAG_WRONG = 13,
} CellType;

void map_generate(Game *game, i32 seed, u32 width, u32 height, u32 mines);
void game_init(Game *game, u32 width, u32 height, u32 mines);
void game_free(Game *game);
void cell_draw(TextureAtlas textureAtlas, u32 index, Vector2 pos, i32 padding);
void cell_uncover(Game *game, i32 x, i32 y);
void cell_evaluate(Game game, u32 *cells_hidden);
void mine_set(Game *game, u32 width, u32 height);

i32 main(i32 argc, char *argv[]) {
  i32 columns = 16;
  i32 rows = 16;
  i32 mines = 40;

  if (mines > (columns * rows) * 0.31) {
    printf("too many mines!\n");
    exit(1);
  }

  if (argc == 4) {
    columns = atoi(argv[1]);
    rows = atoi(argv[2]);
    mines = atoi(argv[3]);
  } else if (!(argc == 4 || argc == 1) || rows < 0 || columns < 0 || mines < 0) {
    printf("usage: %s <columns> <rows> <mines>\n", argv[0]);
    exit(1);
  }

  Game game;
  map_generate(&game, time(NULL), columns, rows, mines);

  TextureAtlas textureAtlas = {.cell_size = 32, .cols = 8, .scale = 1.5f};

  i32 padding = 32;
  u32 scaled_cell_size = textureAtlas.cell_size * textureAtlas.scale;

  u32 screen_width = scaled_cell_size * game.size.x + padding * 2;
  u32 screen_height = scaled_cell_size * game.size.y + padding * 2;

  InitWindow(screen_width, screen_height, "minesweeper");

  textureAtlas.texture = LoadTexture("atlas2.png");

  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

  Color color_gray = {193, 193, 193, 255};
  Color color_red = {228, 32, 21, 255};
  Color color_green = {0, 146, 103, 255};

  Color bg_color = color_gray;

  u32 cells_hidden = game.size.x * game.size.y;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(bg_color);

    for (u32 y = 0; y < game.size.y; y++) {
      for (u32 x = 0; x < game.size.x; x++) {
        cell_draw(textureAtlas, game.revealed[x][y], (Vector2){x, y}, padding);
      }
    }

    cell_evaluate(game, &cells_hidden);

    Vec2i32 cell_coords = {.x = (GetMousePosition().x - padding) / scaled_cell_size,
                           .y = (GetMousePosition().y - padding) / scaled_cell_size};
    Vec2i32 current_cell;

    DrawText(TextFormat("%i", game.mines_left), 0, 0, 20, BLACK);

    switch (game.state) {
    case STATE_PLAYING:
      bg_color = color_gray;

      if (cell_coords.x >= 0 && cell_coords.x <= game.size.x - 1 && cell_coords.y >= 0 &&
          cell_coords.y <= game.size.y - 1) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          cell_uncover(&game, cell_coords.x, cell_coords.y);

          if (game.revealed[cell_coords.x][cell_coords.y] != CELL_FLAG) {
            game.revealed[cell_coords.x][cell_coords.y] = game.board[cell_coords.x][cell_coords.y];
          }

          // if clicked on mine
          if (game.revealed[cell_coords.x][cell_coords.y] == CELL_MINE) {
            current_cell = cell_coords;
            game.state = STATE_LOST;
          }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
          if (game.revealed[cell_coords.x][cell_coords.y] == CELL_FLAG) {
            game.revealed[cell_coords.x][cell_coords.y] = CELL_HIDDEN; // remove flag
            if (game.board[cell_coords.x][cell_coords.y] != CELL_MINE) {
              game.mines_wrong--;
            }
            game.mines_left++;
          } else if (game.revealed[cell_coords.x][cell_coords.y] == CELL_HIDDEN) {
            game.revealed[cell_coords.x][cell_coords.y] = CELL_FLAG; // add flag
            if (game.board[cell_coords.x][cell_coords.y] != CELL_MINE) {
              game.mines_wrong++;
            }
            game.mines_left--;
          }
        }
      }

      // win condition
      if (game.mines_left == 0 && game.mines_wrong == 0 && cells_hidden == 0) {
        game.state = STATE_WON;
      }

      break;
    case STATE_LOST:
      bg_color = color_red;

      for (u32 y = 0; y < game.size.y; y++) {
        for (u32 x = 0; x < game.size.x; x++) {
          if (game.board[x][y] == CELL_MINE && game.revealed[x][y] == CELL_HIDDEN) {
            game.revealed[x][y] = game.board[x][y];
          }

          if (game.board[x][y] != CELL_MINE && game.revealed[x][y] == CELL_FLAG) {
            game.revealed[x][y] = CELL_FLAG_WRONG;
          }
        }
      }

      game.revealed[current_cell.x][current_cell.y] = CELL_MINE_EXPLODED;

      game.state = STATE_INFINITE;
      break;
    case STATE_WON:
      bg_color = color_green;

      game.state = STATE_INFINITE;
      break;
    case STATE_INFINITE:
      DrawText("press r to restart", 50, 0, 20, BLACK);
      if (IsKeyPressed(KEY_R)) {
        u32 mines = game.mines;
        u32 width = game.size.x;
        u32 height = game.size.y;
        game_free(&game);
        map_generate(&game, time(NULL), width, height, mines);
      }
      break;
    }

    EndDrawing();
  }

  game_free(&game);
  UnloadTexture(textureAtlas.texture);
  CloseWindow();
  return 0;
}

void game_init(Game *game, u32 width, u32 height, u32 mines) {
  game->size = (Vec2i32){width, height};
  game->board = malloc(sizeof(u32 *) * width);
  game->revealed = malloc(sizeof(u32 *) * width);
  game->mines = mines;
  game->mines_left = mines;
  game->mines_wrong = 0;
  game->state = STATE_PLAYING;

  for (u32 x = 0; x < width; x++) {
    game->board[x] = calloc(height, sizeof(u32));
    game->revealed[x] = calloc(height, sizeof(u32));
  }
}

void game_free(Game *game) {
  for (u32 x = 0; x < game->size.x; x++) {
    free(game->board[x]);
    free(game->revealed[x]);
  }

  free(game->board);
  free(game->revealed);
}

void map_generate(Game *game, i32 seed, u32 width, u32 height, u32 mines) {
  game_init(game, width, height, mines);

  SetRandomSeed(seed);

  for (u32 n = 0; n < mines; n++) {
    mine_set(game, width, height);
  }

  for (u32 y = 0; y < game->size.y; y++) {
    for (u32 x = 0; x < game->size.x; x++) {
      if (game->board[x][y] != CELL_MINE) {
        game->board[x][y] = CELL_EMPTY;
      }
      game->revealed[x][y] = CELL_HIDDEN;
    }
  }

  for (u32 y = 0; y < game->size.y; y++) {
    for (u32 x = 0; x < game->size.x; x++) {

      if (game->board[x][y] != CELL_MINE)
        continue;

      for (i32 dy = -1; dy <= 1; dy++) {
        for (i32 dx = -1; dx <= 1; dx++) {

          if (dx == 0 && dy == 0)
            continue;

          i32 nx = (i32)x + dx;
          i32 ny = (i32)y + dy;

          if (nx < 0 || nx >= (i32)game->size.x || ny < 0 || ny >= (i32)game->size.y)
            continue;

          if (game->board[nx][ny] != CELL_MINE) {
            game->board[nx][ny]++;
          }
        }
      }
    }
  }
}

void mine_set(Game *game, u32 width, u32 height) {
  u32 x = GetRandomValue(1, width - 1);
  u32 y = GetRandomValue(1, height - 1);

  if (game->board[x][y] == CELL_MINE) {
    mine_set(game, width, height);
  } else {
    game->board[x][y] = CELL_MINE;
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

void cell_evaluate(Game game, u32 *cells_hidden) {
  *cells_hidden = game.size.x * game.size.y;
  for (u32 y = 0; y < game.size.y; y++) {
    for (u32 x = 0; x < game.size.x; x++) {
      if (game.revealed[x][y] != CELL_HIDDEN) {
        *cells_hidden -= 1;
      }
    }
  }
}
