#ifndef buzzer_included
#define buzzer_included

/* initialize the buzzer */
void buzzer_init();
/* Recieves a cycle and sets the buzzer */
void buzzer_set_period(short cycles);
/* Plays tetris song notes */
void buzzer_play_game_song();
void buzzer_play_player_move();

#endif // included
 
