#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <allegro5/allegro.h>

#define MAX_TILES 10000
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

typedef enum TILE_TYPE{
    RANDOM,
} TILE_TYPE;

typedef struct bounds{
    float x,y,w,h;
} Bounds;

typedef struct butao{
    Bounds bounds;
    bool active;
} Butao;

typedef struct player{
    Bounds bounds;
    bool moveRight;
    bool moveLeft;
    bool grounded;
    bool jump;
    float velY;
    float velX;
    float maxVel;
} Player;

typedef struct tile{
    TILE_TYPE tile_type;
    int32_t id;
    Bounds bounds;

} Tile;

typedef struct TileSet{
    Tile tiles[MAX_TILES];
    size_t num_tiles;

} TileSet;

bool boundIntersect(const Bounds* first, const Bounds* second);

bool importTileSet(const char* filename);
bool exportTileSet(const char* filename);
void drawTiles(TileSet* set);
void playerMoveRigtht(Player* p);



int main(){
    


    //definições
    Butao butao_iniciar;
    Butao butao_sair;
    TileSet* tile_set;
    float gravity = 1.0f;
    bool running;
    Player player;
    

    //inicializações
    butao_iniciar.active = false;
    butao_sair.active = false;
    running = true;


    tile_set = malloc(sizeof(TileSet));
    if(!tile_set){
        perror("Erro ao alocar memoria");
        return -1;
    }


    //inicializações allegro
    al_init();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);								//Cria o relógio do jogo; isso indica quantas atualizações serão realizadas por segundo (30, neste caso)
	ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();							//Cria a fila de eventos; todos os eventos (programação orientada a eventos) 
	//ALLEGRO_FONT* font = al_create_builtin_font();									//Carrega uma fonte padrão para escrever na tela (é bitmap, mas também suporta adicionar fontes ttf)
	ALLEGRO_DISPLAY* display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);							//Cria uma janela para o programa, define a largura (x) e a altura (y) da tela em píxeis (320x320, neste caso)

	al_register_event_source(queue, al_get_keyboard_event_source());				//Indica que eventos de teclado serão inseridos na nossa fila de eventos
	al_register_event_source(queue, al_get_display_event_source(display));				//Indica que eventos de tela serão inseridos na nossa fila de eventos
	al_register_event_source(queue, al_get_timer_event_source(timer));	



    while(running){
        //handle events
        ALLEGRO_EVENT ev;

        al_wait_for_event(queue, &ev);
        if(ev.type == ALLEGRO_EVENT_TIMER) {

            //update

            //redraw = true;
        }
        else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        }
        else if(ev.type == ALLEGRO_EVENT_KEY_DOWN){
            //keyboard down

            if(ev.keyboard.keycode == ALLEGRO_KEY_A){
                player.moveLeft = true;
            }

            if(ev.keyboard.keycode == ALLEGRO_KEY_D){
                player.moveRight = true;
            }

            if(ev.keyboard.keycode == ALLEGRO_KEY_SPACE){
                if(player.grounded)
                    player.velY += 1;
            }


        }
        else if(ev.type == ALLEGRO_EVENT_KEY_UP){
            //keyboard up

            if(ev.keyboard.keycode == ALLEGRO_KEY_A){
                player.moveLeft = false;
            }

            if(ev.keyboard.keycode == ALLEGRO_KEY_D){
                player.moveRight = false;
            }


        }


        if(butao_sair.active){
            break;
        }
        if(butao_iniciar.active){
            importTileSet("nivel_1.tsl");
        }


        //render









    }

    //al_destroy_font(font);															//Destrutor da fonte padrão
	al_destroy_display(display);														//Destrutor da tela
	al_destroy_timer(timer);														//Destrutor do relógio
	al_destroy_event_queue(queue);
    free(tile_set);


return 0;

}