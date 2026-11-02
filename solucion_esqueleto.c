// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

int strLength(char* s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

char* strDuplicate(char* src) {
    int len = strLength(src);
    // len + 1 porque hay que guardar tambien el '\0'
    char* res = malloc(len + 1);
    if (res == NULL) return NULL;

    int i = 0;
    while (i <= len) {
        res[i] = src[i];
        i++;
    }
    return res;
}

int strCompare(char* s1, char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s1[i] == s2[i]) {
        i++;
    }
    if (s1[i] == s2[i]) return 0;
    // si s1 es prefijo de s2, s1[i] es '\0' (que vale 0) y da menor
    if (s1[i] < s2[i]) return 1;
    return -1;
}

char* strConcatenate(char* src1, char* src2) {
    int len1 = strLength(src1);
    int len2 = strLength(src2);
    char* res = malloc(len1 + len2 + 1);

    if (res != NULL) {
        int i = 0;
        while (i < len1) {
            res[i] = src1[i];
            i++;
        }
        int j = 0;
        while (j <= len2) {
            res[len1 + j] = src2[j];
            j++;
        }
    }
    // el enunciado pide liberar los dos originales (los liberamos despues de copiarlos)
    free(src1);
    free(src2);
    return res;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

// Lo declaramos aca arriba porque gameBoardNew lo usa si falla un malloc
void gameBoardDelete(GameBoard* board);

GameBoard* gameBoardNew() {
    GameBoard* board = malloc(sizeof(GameBoard));
    if (board == NULL) return NULL;

    board->first_row = NULL;
    board->queue_head = NULL;
    board->queue_tail = NULL;
    board->first_particle = NULL;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->current_piece.active = 0;
    board->score = 0;
    board->tick_timer = 0;

    int i = 0;
    while (i < GRID_ROWS) {
        RowNode* row = malloc(sizeof(RowNode));
        if (row == NULL) {
            // si falla, liberamos lo que ya habiamos creado
            gameBoardDelete(board);
            return NULL;
        }
        int c = 0;
        while (c < GRID_COLS) {
            row->cells[c] = 0;
            c++;
        }
        row->next = board->first_row;
        board->first_row = row;
        i++;
    }
    return board;
}

void gameBoardDelete(GameBoard* board) {
    if (board == NULL) return;

    // En cada lista guardamos el siguiente antes de hacer free,
    // porque despues del free ya no podemos leer el next.
    RowNode* row = board->first_row;
    while (row != NULL) {
        RowNode* sig = row->next;
        free(row);
        row = sig;
    }

    PieceQueueNode* pieza = board->queue_head;
    while (pieza != NULL) {
        PieceQueueNode* sig = pieza->next;
        free(pieza);
        pieza = sig;
    }

    ParticleNode* part = board->first_particle;
    while (part != NULL) {
        ParticleNode* sig = part->next;
        free(part);
        part = sig;
    }

    free(board);
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    if (board == NULL) return;
    ParticleNode* part = malloc(sizeof(ParticleNode));
    if (part == NULL) return;

    part->x = x;
    part->y = y;
    part->vel_x = ((rand() % 100) / 10.0f) - 5.0f;
    part->vel_y = ((rand() % 50) / -10.0f) - 2.0f;
    part->color = color;
    part->life_timer = 30 + (rand() % 30);

    part->next = board->first_particle;
    board->first_particle = part;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    if (board == NULL) return 0;

    int lineas = 0;
    RowNode* prev = NULL;              // la fila anterior a curr (NULL si curr es la primera)
    RowNode* curr = board->first_row;
    int r = 0;

    while (curr != NULL) {
        RowNode* sig = curr->next;

        int llena = 1;
        int c = 0;
        while (c < GRID_COLS) {
            if (curr->cells[c] == 0) llena = 0;
            c++;
        }

        if (llena) {
            // Pedimos la fila nueva ANTES de borrar la vieja: si malloc falla,
            // no borramos nada y el tablero sigue teniendo 20 filas.
            RowNode* nueva = malloc(sizeof(RowNode));
            if (nueva == NULL) return lineas;
            c = 0;
            while (c < GRID_COLS) {
                nueva->cells[c] = 0;
                c++;
            }

            c = 0;
            while (c < GRID_COLS) {
                gameBoardAddParticle(board,
                                     OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                                     OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2,
                                     curr->cells[c]);
                c++;
            }

            // Sacamos curr de la lista: el anterior pasa a apuntar a sig.
            // No se copian celdas, solo se cambia un puntero.
            if (prev == NULL) {
                board->first_row = sig;
            } else {
                prev->next = sig;
            }
            free(curr);

            nueva->next = board->first_row;
            board->first_row = nueva;

            // si borramos la primera fila, ahora la anterior a sig es la nueva
            if (prev == NULL) prev = nueva;

            lineas++;
        } else {
            prev = curr;
        }

        curr = sig;
        r++;
    }
    return lineas;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    if (board == NULL) return;
    PieceQueueNode* nodo = malloc(sizeof(PieceQueueNode));
    if (nodo == NULL) return;
    nodo->type = type;
    nodo->next = NULL;

    if (board->queue_tail == NULL) {
        board->queue_head = nodo;
    } else {
        board->queue_tail->next = nodo;
    }
    board->queue_tail = nodo;
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board == NULL || board->queue_head == NULL) return 0;

    PieceQueueNode* primero = board->queue_head;
    *out_type = primero->type;
    board->queue_head = primero->next;
    // si la cola quedo vacia, tail tambien va en NULL
    // (si no, quedaria apuntando a un nodo liberado)
    if (board->queue_head == NULL) board->queue_tail = NULL;
    free(primero);
    return 1;
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS (COMPLETAR) =========

int fallos = 0;

void chequear(int ok, char* nombre) {
    if (ok) {
        printf("OK    %s\n", nombre);
    } else {
        printf("FALLO %s\n", nombre);
        fallos++;
    }
}

int contarFilas(GameBoard* board) {
    int n = 0;
    RowNode* row = board->first_row;
    while (row != NULL) {
        n++;
        row = row->next;
    }
    return n;
}

int filaVacia(RowNode* row) {
    int c = 0;
    while (c < GRID_COLS) {
        if (row->cells[c] != 0) return 0;
        c++;
    }
    return 1;
}

void llenarFila(GameBoard* board, int index) {
    RowNode* row = getRowAt(board, index);
    int c = 0;
    while (c < GRID_COLS) {
        row->cells[c] = (c % 7) + 1;
        c++;
    }
}

void correrTests() {
    char* s;

    s = strDuplicate("");
    chequear(s != NULL && s[0] == '\0', "strDuplicate vacio");
    free(s);

    s = strDuplicate("x");
    chequear(s != NULL && s[0] == 'x' && s[1] == '\0', "strDuplicate un caracter");
    free(s);

    s = strDuplicate("Hola TD2!");
    chequear(s != NULL && strLength(s) == 9 && strCompare(s, "Hola TD2!") == 0,
             "strDuplicate distintos caracteres");
    free(s);

    chequear(strCompare("", "") == 0, "strCompare dos vacios");
    chequear(strCompare("a", "a") == 0, "strCompare un caracter iguales");
    chequear(strCompare("a", "b") == 1, "strCompare a vs b");
    chequear(strCompare("casa", "perro") == 1, "strCompare casa vs perro");
    chequear(strCompare("perro", "casa") == -1, "strCompare perro vs casa");
    chequear(strCompare("casa", "casas") == 1, "strCompare prefijo");

    // strConcatenate libera sus argumentos, por eso se crean con strDuplicate
    s = strConcatenate(strDuplicate(""), strDuplicate("abc"));
    chequear(s != NULL && strCompare(s, "abc") == 0, "strConcatenate vacio + abc");
    free(s);

    s = strConcatenate(strDuplicate("hola "), strDuplicate("mundo"));
    chequear(s != NULL && strCompare(s, "hola mundo") == 0, "strConcatenate hola + mundo");
    free(s);

    GameBoard* board = gameBoardNew();
    int tipos[5] = {3, 0, 6, 1, 5};
    int i = 0;
    while (i < 5) {
        gameBoardEnqueuePiece(board, tipos[i]);
        i++;
    }
    int orden_ok = 1;
    int t;
    i = 0;
    while (i < 5) {
        if (gameBoardDequeuePiece(board, &t) != 1 || t != tipos[i]) orden_ok = 0;
        i++;
    }
    chequear(orden_ok, "cola: orden FIFO");
    t = -1;
    chequear(gameBoardDequeuePiece(board, &t) == 0 && t == -1, "cola: 6to pop devuelve 0");
    chequear(board->queue_head == NULL && board->queue_tail == NULL, "cola: head y tail en NULL");
    gameBoardEnqueuePiece(board, 4);
    chequear(gameBoardDequeuePiece(board, &t) == 1 && t == 4, "cola: push despues de vaciarla");
    gameBoardDelete(board);

    board = gameBoardNew();
    llenarFila(board, 19);
    RowNode* primera_vieja = board->first_row;
    chequear(gameBoardCheckAndClearLines(board) == 1, "tablero: fila 19 devuelve 1");
    chequear(board->first_row != primera_vieja && filaVacia(board->first_row),
             "tablero: first_row es la fila nueva vacia");
    chequear(contarFilas(board) == 20, "tablero: sigue con 20 filas");
    gameBoardDelete(board);

    board = gameBoardNew();
    llenarFila(board, 0);
    llenarFila(board, 1);
    chequear(gameBoardCheckAndClearLines(board) == 2, "tablero: filas 0 y 1 devuelve 2");
    chequear(contarFilas(board) == 20, "tablero: sigue con 20 filas");
    gameBoardDelete(board);

    printf("Fallos: %d\n", fallos);
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
