#include "game_runtime.h"

extern const game_desc_t game_breakout;

int main(void)
{
    game_runtime_run(&game_breakout);
    return 0;
}
