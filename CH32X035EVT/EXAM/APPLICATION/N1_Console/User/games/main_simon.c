#include "game_runtime.h"

extern const game_desc_t game_simon;

int main(void)
{
    game_runtime_run(&game_simon);
    return 0;
}
