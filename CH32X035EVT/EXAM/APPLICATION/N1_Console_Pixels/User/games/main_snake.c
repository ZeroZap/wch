#include "game_runtime.h"

extern const game_desc_t game_snake;

int main(void)
{
    game_runtime_run(&game_snake);
    return 0;
}
