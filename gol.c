#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    Dead = 0,
    Alive = 1,
} CellStateType;

typedef struct {
    CellStateType state;
    unsigned short how_long;
} CellState;

typedef struct {
    CellState pair[2];
} Cell;

typedef struct {
    Cell* cells;
    unsigned int width;
    unsigned int height;
} World;

Cell* get_cell(World* world, int x, int y) {
    unsigned int normalized_x = div(div(x, world->width).rem + world->width, world->width).rem;
    unsigned int normalized_y = div(div(y, world->height).rem + world->height, world->height).rem;
    return &world->cells[normalized_y * world->width + normalized_x];
}

World new_world(unsigned int width, unsigned int height) {
    Cell* cells = malloc(sizeof(Cell) * width * height);

    for (unsigned int line_counter = 0; line_counter < height; line_counter++) {
        for (unsigned int col_counter = 0; col_counter < width; col_counter++) {
            CellStateType state;

            unsigned short how_long;
            if (div(rand(), 2).rem == 0) {
                state = Alive;
                how_long = 0;
            } else {
                state = Dead;
                how_long = 0;
            };

            cells[line_counter * width + col_counter] =
                (Cell){.pair = {
                           (CellState){state = state, how_long = how_long},
                           (CellState){state = state, how_long = how_long},
                       }};
        }
    }
    return (World){
        .cells = cells,
        .width = width,
        .height = height,
    };
}

int main(int argc, char** argv) {
    srand(time(NULL));
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
        exit(-1);
    };
    World world = new_world(size.ws_col, size.ws_row);
    printf("\x1B[?25l\x1B[1;1H");
    unsigned long long counter = 0;

    while (1) {
        // computing
        ssize_t current_index = div(counter, 2).rem;
        ssize_t next_index = 1 - div(counter, 2).rem;
        for (unsigned lineno = 0; lineno < world.height; lineno++) {
            for (unsigned int rowno = 0; rowno < world.width; rowno++) {
                short alive_around = 0;
                CellState* current_state = &get_cell(&world, rowno, lineno)->pair[current_index];
                CellState* next_state = &get_cell(&world, rowno, lineno)->pair[next_index];

                for (int vertical_shift = -1; vertical_shift <= 1; vertical_shift++) {
                    for (int horizontal_shift = -1; horizontal_shift <= 1; horizontal_shift++) {
                        if (vertical_shift == 0 && horizontal_shift && 0) {
                            continue;
                        }
                        if (get_cell(&world, rowno + horizontal_shift, lineno + vertical_shift)
                                ->pair[current_index]
                                .state == Alive) {
                            alive_around++;
                        }
                    }
                }
                switch (current_state->state) {
                    case Alive:
                        if (alive_around < 2 || alive_around > 3) {
                            next_state->state = Dead;
                            next_state->how_long = 1;
                        } else {
                            next_state->state = Alive;
                        }
                        break;
                    case Dead:
                        if (alive_around == 3) {
                            next_state->state = Alive;
                            next_state->how_long = 0;
                        } else {
                            next_state->state = Dead;
                            next_state->how_long =
                                current_state->how_long < 255 ? 1 + current_state->how_long : 255;
                        }
                        break;
                }
            }
        }
        // rendering
        for (unsigned int cellno = 0; cellno < (world.height * world.width); cellno++) {
            Cell* cell = &world.cells[cellno];
            switch (cell->pair[current_index].state) {
                case Alive:
                    printf("\x1B[48;2;255;255;255m ");
                    break;
                case Dead:
                    unsigned short color = 255 - cell->pair[current_index].how_long;
                    unsigned short red = color / 2;
                    unsigned short green = color / 5;
                    unsigned short blue = color;
                    printf("\x1B[48;2;%d;%d;%dm ", red, green, blue);
                    break;
            };
        }
        printf("\x1B[1;1H");
        counter++;
    }
    return 0;
}