#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

#define MAX_TILES 10000
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define FLAG_NUM 10
#define DEFAULT_TILE_SIZE 32
#define BASE_HP 3

typedef enum FLAGS {
    SOLID
} FLAG;

typedef enum TILE_TYPE {
    SOLID_DEFAULT,
    SPIKE,
    RANDOM,
    FINISH
} TILE_TYPE;

typedef struct bounds {
    float x, y, w, h;
} Bounds;

typedef struct butao {
    Bounds bounds;
    bool active;
} Butao;

typedef struct player {
    int32_t hp;
    Bounds bounds;

    bool moveRight;
    bool moveLeft;

    bool grounded;
    bool jump;

    float velY;
    float velX;

    float maxVel;
    float spdMltpr;
} Player;

typedef struct tile {
    TILE_TYPE tile_type;
    bool flags[FLAG_NUM];

    int32_t id;

    Bounds bounds;
} Tile;

typedef struct TileSet {
    Tile tiles[MAX_TILES];
    size_t num_tiles;
} TileSet;

typedef struct game {
    bool running;

    TileSet* tile_set;

    float gravity;

    Player player;

    int32_t screenOffsetX;
    int32_t screenOffsetY;

    bool mapping;

    int32_t currentTileType;
    int32_t nextTileId;
} Game;

bool boundIntersect(const Bounds* first, const Bounds* second) {
    return (
        first->x < second->x + second->w &&
        first->x + first->w > second->x &&
        first->y < second->y + second->h &&
        first->y + first->h > second->y
    );
}

// parte de baixo do top tocando parte de cima do bottom
bool boundIntersectBottom(const Bounds* top, const Bounds* bottom) {
    return (
        top->y + top->h >= bottom->y &&
        top->y + top->h <= bottom->y + 10 &&
        top->x + top->w > bottom->x &&
        top->x < bottom->x + bottom->w
    );
}

void boundPush(Bounds* to_push, const Bounds* fixed) {
    float left   = (to_push->x + to_push->w) - fixed->x;
    float right  = (fixed->x + fixed->w) - to_push->x;
    float top    = (to_push->y + to_push->h) - fixed->y;
    float bottom = (fixed->y + fixed->h) - to_push->y;

    float min = left;
    int dir = 0;

    if (right < min) {
        min = right;
        dir = 1;
    }

    if (top < min) {
        min = top;
        dir = 2;
    }

    if (bottom < min) {
        min = bottom;
        dir = 3;
    }

    switch (dir) {
        case 0:
            to_push->x -= left;
            break;
        case 1:
            to_push->x += right;
            break;
        case 2:
            to_push->y -= top;
            break;
        case 3:
            to_push->y += bottom;
            break;
    }
}

void moveBounds(Bounds* bounds, float x_amount, float y_amount) {
    bounds->x += x_amount;
    bounds->y += y_amount;
}

void dealDamage(Player* p, int32_t amount) {
    p->hp -= amount;
    printf("Player tomou %d de dano\n", amount);
}

int adjustToGrid(float value, int32_t gridSize) {
    return ((int)value / gridSize) * gridSize;
}

Tile getTileSpecs(TILE_TYPE type, int32_t id, float x, float y) {
    Tile tile = {0};

    tile.bounds = (Bounds){ x, y, DEFAULT_TILE_SIZE, DEFAULT_TILE_SIZE };
    tile.id = id;
    tile.tile_type = type;

    switch (type) {
        case SOLID_DEFAULT:
            tile.flags[SOLID] = true;
            break;
        case SPIKE:
            tile.flags[SOLID] = false;
            break;
        case RANDOM:
            tile.flags[SOLID] = true;
            break;
        case FINISH:
            tile.flags[SOLID] = false;
            break;
        default:
            tile.flags[SOLID] = true;
            break;
    }

    return tile;
}

TileSet* importTileSet(const char* filename) {
    if (!filename)
        return NULL;

    TileSet* set = malloc(sizeof(TileSet));

    if (!set)
        return NULL;

    FILE* src = fopen(filename, "rb");

    if (!src) {
        perror("Arquivo de nivel inexistente");
        set->num_tiles = 0;
        return set;
    }

    fread(&set->num_tiles, sizeof(size_t), 1, src);
    fread(set->tiles, sizeof(Tile), set->num_tiles, src);

    fclose(src);

    return set;
}

bool exportTileSet(const char* filename, TileSet* set) {
    if (!filename || !set)
        return false;

    FILE* out = fopen(filename, "wb");

    if (!out) {
        perror("Falha ao abrir arquivo");
        return false;
    }

    fwrite(&set->num_tiles, sizeof(size_t), 1, out);
    fwrite(set->tiles, sizeof(Tile), set->num_tiles, out);

    fclose(out);

    return true;
}

void drawTiles(TileSet* set) {
    if (!set)
        return;

    Tile* current;

    for (size_t i = 0; i < set->num_tiles; i++) {
        current = &set->tiles[i];

        float x  = current->bounds.x;
        float y  = current->bounds.y;
        float x2 = x + current->bounds.w;
        float y2 = y + current->bounds.h;

        switch (current->tile_type) {
            case SOLID_DEFAULT:
                al_draw_filled_rectangle(x, y, x2, y2, al_map_rgb(120, 120, 120));
                break;
            case SPIKE:
                al_draw_filled_rectangle(x, y, x2, y2, al_map_rgb(255, 0, 0));
                break;
            case FINISH:
                al_draw_filled_rectangle(x, y, x2, y2, al_map_rgb(0, 255, 0));
                break;
            default:
                al_draw_filled_rectangle(x, y, x2, y2, al_map_rgb(100, 100, 255));
                break;
        }
    }
}

void addTile(TileSet* set, int32_t x, int32_t y, int32_t* id, TILE_TYPE type) {
    if (!set || !id) {
        printf("addTile: set/id invalido\n");
        return;
    }

    if (*id >= MAX_TILES) {
        printf("Limite maximo de tiles atingido\n");
        return;
    }

    set->tiles[*id] = getTileSpecs(type, *id, x, y);
    set->num_tiles++;
    (*id)++;
}

void updateTiles(TileSet* set, Player* p) {
    if (!set || !p)
        return;

    p->grounded = false;

    for (size_t i = 0; i < set->num_tiles; i++) {
        Tile* current = &set->tiles[i];

        switch (current->tile_type) {
            case SPIKE:
                if (boundIntersect(&p->bounds, &current->bounds)) {
                    dealDamage(p, 1);
                }
                break;

            case FINISH:
                break;

            default:
                if (boundIntersectBottom(&p->bounds, &current->bounds)) {
                    p->grounded = true;
                    p->velY = 0;
                }

                if (current->flags[SOLID] && boundIntersect(&p->bounds, &current->bounds)) {
                    boundPush(&p->bounds, &current->bounds);
                }
                break;
        }
    }
}

int main() {
    Game* game = malloc(sizeof(Game));

    if (!game)
        return -1;

    char level_name[] = "nivel_1.tls";
    
    // game
    game->running = true;
    game->gravity = 1.0f;

    game->screenOffsetX = 0;
    game->screenOffsetY = 0;

    game->mapping = false;

    game->currentTileType = SOLID_DEFAULT;
    game->nextTileId = 0;

    // player
    Player* player = &game->player;

    player->hp = BASE_HP;

    player->bounds.x = 100;
    player->bounds.y = 100;
    player->bounds.w = 32;
    player->bounds.h = 32;

    player->moveLeft  = false;
    player->moveRight = false;

    player->grounded = false;
    player->jump     = false;

    player->velX = 0;
    player->velY = 0;

    player->maxVel   = 10.0f;
    player->spdMltpr = 0.1f;

    // tileset
    game->tile_set = malloc(sizeof(TileSet));

    if (!game->tile_set) {
        perror("Erro ao alocar tileset");
        free(game);
        return -1;
    }

    game->tile_set->num_tiles = 0;

    // allegro
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    al_init_font_addon();

    ALLEGRO_DISPLAY*     display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
    ALLEGRO_TIMER*       timer   = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue   = al_create_event_queue();
    ALLEGRO_FONT*        font    = al_create_builtin_font();

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    al_start_timer(timer);
    
    //LEVEL LOAD //temp
    {
    TileSet* imported = importTileSet(level_name);
                if (imported) {
                    free(game->tile_set);
                    game->tile_set   = imported;
                    game->nextTileId = game->tile_set->num_tiles;
                }
    }

    while (game->running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {

            // sala direita
            if ((player->bounds.x + player->bounds.w / 2) > SCREEN_WIDTH) {
                game->screenOffsetX -= SCREEN_WIDTH;
                player->bounds.x    -= SCREEN_WIDTH;
                for (size_t i = 0; i < game->tile_set->num_tiles; i++)
                    game->tile_set->tiles[i].bounds.x -= SCREEN_WIDTH;
            }

            // sala esquerda
            if ((player->bounds.x - player->bounds.w / 2) < 0) {
                game->screenOffsetX += SCREEN_WIDTH;
                player->bounds.x    += SCREEN_WIDTH;
                for (size_t i = 0; i < game->tile_set->num_tiles; i++)
                    game->tile_set->tiles[i].bounds.x += SCREEN_WIDTH;
            }

            // sala cima
            if ((player->bounds.y - player->bounds.h / 2) < 0) {
                game->screenOffsetY += SCREEN_HEIGHT;
                player->bounds.y    += SCREEN_HEIGHT;
                for (size_t i = 0; i < game->tile_set->num_tiles; i++)
                    game->tile_set->tiles[i].bounds.y += SCREEN_HEIGHT;
            }

            // sala baixo
            if ((player->bounds.y + player->bounds.h / 2) > SCREEN_HEIGHT) {
                game->screenOffsetY -= SCREEN_HEIGHT;
                player->bounds.y    -= SCREEN_HEIGHT;
                for (size_t i = 0; i < game->tile_set->num_tiles; i++)
                    game->tile_set->tiles[i].bounds.y -= SCREEN_HEIGHT;
            }            

            // movimento horizontal
            if (player->moveRight) {
                if (player->velX >= player->maxVel) {
                    player->velX = player->maxVel;
                    player->bounds.x += 1 + player->velX;
                } else {
                    if(player->velX < 0)
                        player->velX = 0.0f;
                    player->bounds.x += 1 + player->velX;
                    player->velX += player->maxVel * player->spdMltpr;
                }
            }

            if (player->moveLeft) {
                if (player->velX <= -player->maxVel) {
                    player->velX = -player->maxVel;
                    player->bounds.x -= 1 - player->velX;
                } else {
                    if(player->velX > 0)
                        player->velX = 0.0f;
                    player->bounds.x -= 1 - player->velX;
                    player->velX -= player->maxVel * player->spdMltpr;
                }
            }

            // gravidade/salto
            if (!player->grounded) {
                // subindo
                if(player->velY < 0){

                    if(player->jump){
                        player->velY += game->gravity;
                    }

                    if(!player->jump){
                        if (player->velY <= -player->maxVel) {
                            player->jump = true;
                        } else {
                            player->velY     += -player->maxVel * player->spdMltpr;
                            player->bounds.y -= 1 - player->velY;
                        }
                    }
                    player->bounds.y -= 1 - player->velY;


                } else {
                    // caindo
                    if (player->velY >= player->maxVel) {
                        player->velY = player->maxVel;
                        player->bounds.y += game->gravity + player->velY;
                    } else {
                        player->bounds.y += game->gravity + player->velY;
                        player->velY += player->maxVel * player->spdMltpr;
                    }
                }
            }

            // tiles
            updateTiles(game->tile_set, player);

            // render
            al_clear_to_color(al_map_rgb(20, 20, 20));

            drawTiles(game->tile_set);

            al_draw_filled_rectangle(
                player->bounds.x,
                player->bounds.y,
                player->bounds.x + player->bounds.w,
                player->bounds.y + player->bounds.h,
                al_map_rgb(255, 255, 255)
            );


                //TEXTOS
            al_draw_textf(
                font, al_map_rgb(255, 255, 255), 10, 10, 0,
                "Tiles: %d", (int)game->tile_set->num_tiles);
            
            if(game->mapping){
                al_draw_textf(
                    font, al_map_rgb(255, 255, 255), 10, 20, 0,
                    "Current tile type: %d", (int)game->currentTileType);

                al_draw_textf(
                    font,
                    al_map_rgb(255, 255, 255), 10, 30, 0,
                    "Player Grounded: %d", (int)player->grounded);

            }
            

            al_flip_display();

        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            game->running = false;

        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {

            if (ev.keyboard.keycode == ALLEGRO_KEY_A)
                player->moveLeft = true;

            if (ev.keyboard.keycode == ALLEGRO_KEY_D)
                player->moveRight = true;

            if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                if (player->grounded) {
                    printf("Salto\n");
                    player->velY     = -1;
                    player->grounded = false;
                    player->jump     = false;
                }
            }

            if (ev.keyboard.keycode == ALLEGRO_KEY_LCTRL)
                game->mapping = !game->mapping;

            if (ev.keyboard.keycode == ALLEGRO_KEY_1)
                game->currentTileType = SOLID_DEFAULT;

            if (ev.keyboard.keycode == ALLEGRO_KEY_2)
                game->currentTileType = SPIKE;

            if (ev.keyboard.keycode == ALLEGRO_KEY_3)
                game->currentTileType = FINISH;

        } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {

            if (ev.keyboard.keycode == ALLEGRO_KEY_A)
                player->moveLeft = false;

            if (ev.keyboard.keycode == ALLEGRO_KEY_D)
                player->moveRight = false;
        }

        if (game->mapping) {
            if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                int mx = ev.mouse.x;
                int my = ev.mouse.y;

                if (ev.mouse.button == 1) {
                    addTile(
                        game->tile_set,
                        adjustToGrid(mx, DEFAULT_TILE_SIZE),
                        adjustToGrid(my, DEFAULT_TILE_SIZE),
                        &game->nextTileId,
                        game->currentTileType
                    );
                }
            }
            
            if(ev.type == ALLEGRO_EVENT_KEY_DOWN){
                if(ev.keyboard.keycode == ALLEGRO_KEY_END){
                    if(exportTileSet(level_name, game->tile_set))
                        printf("Exported level to: %s\n", level_name);
                }

                if(ev.keyboard.keycode == ALLEGRO_KEY_HOME){
                    TileSet* imported = importTileSet(level_name);
                    if (imported) {
                        free(game->tile_set);
                        game->tile_set   = imported;
                        game->nextTileId = game->tile_set->num_tiles;
                    }
                    
                }
                    
            }
            
            
        }
    }

    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    free(game->tile_set);
    free(game);

    return 0;
}
