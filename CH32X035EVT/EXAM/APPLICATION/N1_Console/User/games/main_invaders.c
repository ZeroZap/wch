#include "game_runtime.h"

extern const game_desc_t game_invaders;

int main(void)
{
    game_runtime_run(&game_invaders);
    return 0;
}
