/*
    Sjeng - a chess variants playing program
    Copyright (C) 2000-2003 Gian-Carlo Pascutto
    Originally based on code from Adrien M. Regimbald, used with permission
    Portions contributed by Vincent Diepeveen, used with permission

    File: sjeng.c
    Purpose: main program, xboard/user interface                  

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sjeng.h"
#include "protos.h"
#include "extvars.h"

#if defined(SPEC_CPU)
#   include <time.h>
#else
#   include <sys/times.h>
#   include <unistd.h>
#endif

char divider[50] = "-------------------------------------------------";
move_s dummy = {0,0,0,0,0,0};

int board[144], moved[144], ep_square, white_to_move, comp_color, wking_loc,
  bking_loc, white_castled, black_castled, result, ply, pv_length[PV_BUFF],
  pieces[62], squares[144], num_pieces, i_depth, fifty, piece_count;

int nodes, raw_nodes, qnodes,  killer_scores[PV_BUFF],
  killer_scores2[PV_BUFF], killer_scores3[PV_BUFF], moves_to_tc, min_per_game,
  sec_per_game, inc, time_left, opp_time, time_cushion, time_for_move, cur_score;

unsigned int history_h[144][144];

unsigned int hash_history[600];
int move_number;

xbool captures, searching_pv, post, time_exit, time_failure;

int xb_mode, maxdepth;

int phase;
int root_to_move;

int my_rating, opp_rating;

char setcode[30];

move_s pv[PV_BUFF][PV_BUFF], killer1[PV_BUFF], killer2[PV_BUFF],
 killer3[PV_BUFF];

move_x path_x[PV_BUFF];
move_s path[PV_BUFF];
 
rtime_t start_time;

int is_promoted[62];

unsigned int NTries, NCuts, TExt;
unsigned int PVS, FULL, PVSF;
unsigned int ext_check;

xbool is_pondering, allow_pondering, is_analyzing;

unsigned int bookidx;

int Variant;
int Giveaway;

char my_partner[STR_BUFF];
xbool have_partner;
xbool must_sit;
xbool go_fast;

int fixed_time;

int book_ply;
int use_book;
char opening_history[STR_BUFF];

char *aarch64_fn = "./enclave/sjeng_aarch64";
char *x86_64_fn  = "./enclave/sjeng_x86_64";
int is_migrated = 0;
void check_migrate(void (*callback)(void *), void *callback_data);

int main (int argc, char *argv[]) {

  char input[STR_BUFF], *p, output[STR_BUFF];
  char readbuff[STR_BUFF];
  move_s move, comp_move;
  int depth = 4;
  xbool force_mode, show_board;
  move_s game_history[600];
  move_x game_history_x[600];
  int is_edit_mode, edit_color;
  int pingnum;
  int braindeadinterface;
  int automode;
  rtime_t xstart_time;
  
  read_rcfile();
  initialize_zobrist();
 
  Variant = Normal;
  /*Variant = Crazyhouse;*/

  memcpy(material, std_material, sizeof(std_material));
  /*memcpy(material, zh_material, sizeof(zh_material));*/

  init_game ();

  initialize_hash();
  clear_tt();
  reset_ecache();
    
  ECacheProbes = 0;
  ECacheHits = 0;
  TTProbes = 0;
  TTStores = 0;
  TTHits = 0;
  bookidx = 0;
  total_moves = 0;
  ply = 0;
  braindeadinterface = 0;
  moves_to_tc = 40;
  min_per_game = 5;
  time_left = 30000;
  my_rating = opp_rating = 2000;
  maxdepth = 40;
  maxposdiff = 200;
  must_go = 1;
  tradefreely = 1;
  automode = 0;
 
  xb_mode = FALSE;
  force_mode = FALSE;
  comp_color = 0;
  edit_color = 0;
  show_board = TRUE;
  is_pondering = FALSE;
  allow_pondering = TRUE;
  is_analyzing = FALSE;
  is_edit_mode = FALSE;
  have_partner = FALSE;
  must_sit = FALSE;
  go_fast = FALSE;
  fixed_time = FALSE;
  phase = Opening;
  root_to_move = WHITE;
  kibitzed = FALSE;

  move_number = 0;
  memset(game_history, 0, sizeof(game_history));
  memset(game_history_x, 0, sizeof(game_history_x));

  hash_history[move_number] = hash;
  
  setbuf (stdout, NULL);
  setbuf (stdin, NULL);
  start_up ();

  char testset[64] = {0}; 
  if (argc == 2)
  {
	printf("SPEC Workload\n");
	strcpy(testset, argv[1]);
    run_autotest(testset);    
  }

  return 0;

}

void output_gloabl_vars() {
	printf("-----output global vars-----\n");
	printf("\t node(%p): %d\n", &nodes, nodes);
}


void run_autotest(char *testset)
{
	printf("testset(%p): %s\n", testset, testset);
	//printf("Maybe migrate now? 10s countdown...\n");
	//sleep(10);

	int testsuite;
	//FILE *testsuite;
	char readbuff[STR_BUFF];
        int searchdepth;
	rtime_t start, end;
	
	move_s comp_move;

	testsuite = open(testset, O_RDWR);

	if (testsuite < 0) exit(EXIT_FAILURE);

	start = rtime();
	
    unsigned char c;
	int index = 0;
	while (read(testsuite, &c, 1) != 0) {

#ifndef NO_MIGRATION
	check_migrate(1, 0);
#endif

		if (c != '\n') {
			readbuff[index] = c;
			++index;
			continue;
		}
		readbuff[index] = '\0';
		index = 0;
		setup_epd_line(readbuff);
                root_to_move = ToMove;
					        
		clear_tt();
		initialize_hash();
		           
		printf("\n");
		display_board(stdout, 1);

		printf("EPD: %s\n", readbuff);

		//if (fgets(readbuff, STR_BUFF, testsuite) == NULL) exit(EXIT_FAILURE);
		while (read(testsuite, &c, 1) != 0 && c != '\n') {
			readbuff[index] = c;
			++index;
		}
		if (index > 0) {
			readbuff[index] = '\0';
			index = 0;
		} else {
			printf("read error!\n");
			exit(EXIT_FAILURE);
		}

		searchdepth = atoi(readbuff);
		
		printf("Searching to %d ply\n", searchdepth);
		maxdepth = searchdepth;

		fixed_time = INF;
		output_gloabl_vars();
		comp_move = think();

	}

//	while (fgets(readbuff, STR_BUFF, testsuite) != NULL)
// 	{
//		setup_epd_line(readbuff);
//                root_to_move = ToMove;
//					        
//		clear_tt();
//		initialize_hash();
//		           
//		printf("\n");
//		display_board(stdout, 1);
//
//		printf("EPD: %s\n", readbuff);
//
//		if (fgets(readbuff, STR_BUFF, testsuite) == NULL) exit(EXIT_FAILURE);
//		searchdepth = atoi(readbuff);
//		
//		printf("Searching to %d ply\n", searchdepth);
//		maxdepth = searchdepth;
//
//		fixed_time = INF;
//		comp_move = think();
//
//    printf("Maybe migrate now! You have 10 seconds!\n");
//    sleep(10);
//    check_migrate(0, 0);
//
//	}
		
	end = rtime();
/*        printf("Total elapsed: %i.%02i seconds\n", rdifftime(end, start)/100,
			                           rdifftime(end, start)%100);
*/
	close(testsuite);
	exit(EXIT_SUCCESS);
}
