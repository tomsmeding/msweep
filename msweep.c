#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <termios.h>

#define DEF_NBOMBS (10)
#define DEF_WIDTH (9)
#define DEF_HEIGHT (9)

#define prflush(...) do { printf(__VA_ARGS__); fflush(stdout); } while(0)

typedef enum Direction {
	UP,
	RIGHT,
	DOWN,
	LEFT
} Direction;

struct termios tios_bak;

void initscreen(void) {
	struct termios tios;
	tcgetattr(0, &tios_bak);
	tios=tios_bak;
	tios.c_lflag&=~(
		ECHO|ECHOE|ECHOKE // no echo of normal characters, erasing and killing
		|ECHOCTL // don't visibly echo control characters (^V etc.)
		|ECHONL // don't even echo a newline
		|ICANON // disable canonical mode
#ifdef NOKERNINFO
		|NOKERNINFO // don't print a status line on ^T
#endif
		|IEXTEN //don't handle things like ^V specially
		//|ISIG // disable ^C ^\ and ^Z
		);
	tios.c_cc[VMIN]=1; // read one char at a time
	tios.c_cc[VTIME]=0; // no timeout on reading, make it a blocking read
	tcsetattr(0, TCSAFLUSH, &tios);

	prflush("\x1B[?1049h\x1B[2J\x1B[H");
}

void endscreen(void) {
	tcsetattr(0, TCSAFLUSH, &tios_bak);
	prflush("\x1B[?1049l");
}

void gotoxy(int x, int y) {
	prflush("\x1B[%d;%dH", y+1, x+1);
}

typedef enum Keytype {
	KARROW,
	KCHAR
} Keytype;

typedef struct Key {
	Keytype type;
	char ch;
	Direction dir;
} Key;

void getkey(Key *key) {
	char c = getchar();
	if(c != 27) {
		key->type = KARROW;
		switch (c) {
		case 'h': key->dir = LEFT; return;
		case 'j': key->dir = DOWN; return;
		case 'k': key->dir = UP; return;
		case 'l': key->dir = RIGHT; return;
		default:
			key->type = KCHAR;
			key->ch = c;
			return;
		}
	}
	c = getchar();
	if(c != '[') {
		ungetc(c, stdin);
		key->type = KCHAR;
		key->ch = c;
		return;
	}
	c=getchar();
	switch (c) {
	case 'A': key->type=KARROW; key->dir = UP; return;
	case 'B': key->type=KARROW; key->dir = DOWN; return;
	case 'C': key->type=KARROW; key->dir = RIGHT; return;
	case 'D': key->type=KARROW; key->dir = LEFT; return;
	default:
		// unknown escape code
		while(c < 64 || c > 126) {
			c = getchar();
		}
		getkey(key); // just try again
		return;
	}
}

typedef struct Data{
	bool open, bomb, flag;
	int count;
} Data;

void data_init(Data *data) {
	data->open = false;
	data->bomb = false;
	data->flag = false;
	data->count = 0;
}

typedef struct Board{
	int w,h;
	Data *data;
	int curx,cury;
	int nbombs,nflags,nopen;
} Board;

Board* board_make(int w, int h, int nbombs) {
	Board *bd = malloc(sizeof(Board));
	bd->w = w;
	bd->h = h;
	bd->data = malloc(w*h*sizeof(Data));
	bd->curx = 0;
	bd->cury = 0;
	bd->nbombs = nbombs;
	bd->nflags = 0;
	bd->nopen = 0;

	for (int i = 0; i < w*h; i++) {
		data_init(bd->data+i);
	}
	int n = bd->nbombs;
	while (n > 0) {
		int pos=rand()%(w*h);
		if(bd->data[pos].bomb) continue;
		bd->data[pos].bomb = true;
		bd->data[pos].count = 0;
		n--;
		for (int dy = pos<w?0:-1; dy<=(pos>=w*(h-1)?0:1); dy++)  {
			for (int dx = pos%w==0?0:-1; dx<=(pos%w==w-1?0:1); dx++) {
				if(dx == 0 && dy == 0) continue;
				if(!bd->data[pos+w*dy+dx].bomb) bd->data[pos+w*dy+dx].count++;
			}
		}
	}
	return bd;
}

void board_destroy(Board *bd) {
	free(bd->data);
	free(bd);
}

void board_goto(Board *bd, int x, int y) {
	(void)bd;
	gotoxy(2+2*x, 1+y);
}

void board_gotocursor(Board *bd) {
	board_goto(bd, bd->curx, bd->cury);
}

void board_shiftcursor(Board *bd, Direction dir) {
	switch(dir) {
	case UP: if(bd->cury>0)bd->cury--; else return; break;
	case RIGHT: if(bd->curx<bd->w-1)bd->curx++; else return; break;
	case DOWN: if(bd->cury<bd->h-1)bd->cury++; else return; break;
	case LEFT: if(bd->curx>0)bd->curx--; else return; break;
	}
	board_gotocursor(bd);
}

// doesn't gotoxy
void board_drawcell(Board *bd, int x, int y) {
	const Data *data=bd->data+(bd->w*y+x);
	if(data->flag)putchar('#');
	// else if(data->bomb)putchar(','); // DEBUG
	else if(!data->open)putchar('.');
	else if(data->count==0)putchar(' ');
	else putchar('0'+data->count);
}

void board_draw(Board *bd) {
	gotoxy(0, 0);
	putchar('+');
	for(int x=0;x<bd->w;x++)printf("--");
	printf("-+\n");
	for(int y=0;y<bd->h;y++) {
		putchar('|');
		for(int x=0;x<bd->w;x++) {
			putchar(' ');
			board_drawcell(bd, x, y);
		}
		printf(" |");
		switch(y) {
		case 1: printf("   %dx%d minesweeper", bd->w, bd->h); break;
		case 2: printf("   %d bombs", bd->nbombs); break;
		case 3: printf("   %d flag%s placed ", bd->nflags, bd->nflags==1?"":"s"); break;
		case 5: printf("   'f' to flag, <space> to open"); break;
		case 6: printf("   arrow keys to move, 'r' to restart"); break;
		case 7: printf("   'q' to quit"); break;
		}
		putchar('\n');
	}
	putchar('+');
	for(int x=0;x<bd->w;x++)printf("--");
	printf("-+\n");
	board_gotocursor(bd);
}

void board_flag(Board *bd) {
	Data *data=bd->data+(bd->w*bd->cury+bd->curx);
	data->flag=!data->flag;
	bd->nflags+=2*data->flag-1;
}

void board_flood(Board *bd, int x, int y) {
	bd->data[bd->w*y+x].open=true;
	bd->nopen++;
	if(bd->data[bd->w*y+x].count!=0)return;
	if(x>0) {Data *d=bd->data+(bd->w*y+x-1);if(!d->open)board_flood(bd, x-1, y);}
	if(y>0) {Data *d=bd->data+(bd->w*(y-1)+x);if(!d->open)board_flood(bd, x, y-1);}
	if(x<bd->w-1) {Data *d=bd->data+(bd->w*y+x+1);if(!d->open)board_flood(bd, x+1, y);}
	if(y<bd->h-1) {Data *d=bd->data+(bd->w*(y+1)+x);if(!d->open)board_flood(bd, x, y+1);}
}

bool board_open(Board *bd) {
	Data *data=bd->data+(bd->w*bd->cury+bd->curx);
	if(data->flag||data->open) {
		prflush("\x07"); // bel
		return false;
	}
	if(data->bomb)return true;
	board_flood(bd, bd->curx, bd->cury);
	return false;
}

void board_revealbombs(Board *bd) {
	printf("\x1B[7m");
	for(int y=0;y<bd->h;y++)
	for(int x=0;x<bd->w;x++) {
		Data *data=bd->data+(bd->w*y+x);
		if(!data->bomb)continue;
		board_goto(bd, x, y);
		board_drawcell(bd, x, y);
	}
	printf("\x1B[0m");
	board_gotocursor(bd);
}

bool board_win(Board *bd) {
	return bd->nflags==bd->nbombs&&bd->nopen+bd->nflags==bd->w*bd->h;
}


void prompt_quit(int height) {
	gotoxy(0, height);
	prflush("Really quit? [y/N] ");
	Key key;
	getkey(&key);
	if(key.type!=KCHAR||key.ch!='y') {
		prflush("\x1B[2K");
	} else {
		exit(0);
	}
}

bool prompt_playagain(const char *msg, int height) {
	gotoxy(0, height);
	prflush("\x1B[7m%s\x1B[0m\nPlay again? [y/N] ", msg);
	Key key;
	getkey(&key);
	prflush("\x1B[2K\x1B[A\x1B[2K");
	if(key.type!=KCHAR||key.ch!='y') {
		return false;
	} else {
		return true;
	}
}

void signalend(int sig) {
	(void)sig;
	endscreen();
	exit(1);
}

int main(int argc, char **argv) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec*1000000ULL+tv.tv_usec);

	initscreen();
	atexit(endscreen);
	signal(SIGINT, signalend);

	int width = DEF_WIDTH;
	int height = DEF_HEIGHT;
	int nbombs = DEF_NBOMBS;
	for (int i = 1; i < argc; i++) {
		int val = atoi(argv[i]);
		switch (i) {
		case 1: width = val;
		case 2: height = val;
		case 3: nbombs = val;
		}
	}

	Board *bd=board_make(width, height, nbombs);
	Key key;
	bool quit = false;
	while (!quit) {
		board_draw(bd);
		if (board_win(bd)) {
			if (!prompt_playagain("You win!", height + 2))  break;
			board_destroy(bd);
			bd=board_make(width, height, nbombs);
			continue;
		}
		getkey(&key);
		switch (key.type) {
		case KARROW:
			board_shiftcursor(bd, key.dir);
			break;
		case KCHAR:
			switch (key.ch) {
			case 'q':
				prompt_quit(height + 2);
				break;
			case 'f':
				board_flag(bd);
				break;
			case 'r':
				board_destroy(bd);
				bd=board_make(width, height, nbombs);
				break;
			case ' ':
				if (!board_open(bd)) break;
				board_revealbombs(bd);
				if (!prompt_playagain("BOOM!", height + 2))  {
					quit = true;
					break;
				}
				board_destroy(bd);
				bd=board_make(width, height, nbombs);
				break;
			}
			break;
		default:
			prflush("\x07");
			break;
		}
	}

	board_destroy(bd);
}
