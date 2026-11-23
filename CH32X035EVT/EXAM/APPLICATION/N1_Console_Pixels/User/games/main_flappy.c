#include "game_runtime.h"

extern const game_desc_t game_flappy;

int main(void)
{
    game_runtime_run(&game_flappy);
    return 0;
}
