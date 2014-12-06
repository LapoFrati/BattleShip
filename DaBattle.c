#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "kbhit.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DIM_MARE 10 /* Dimensione del campo di gioco */
#define NNAVI 5	/* Numero di navi */

#define TRUE 1
#define FALSE 0

#define UP 0 /* Direzioni usate durante il posizionamento delle navi */
#define RIGHT 1 

#define MANCATO 1 /* Contrassegna una casella in cui è stato effettuato un tentativo ma non c'era nulla */
#define COLPITO -1 /* Contrassegna una casella in cui è stato effettuato un tentativo ed è stata colpita una nave */
#define START 0 /* Usato per non stampare informzioni inutili al primo turno */
#define AFFONDATO 2 /* Usato per stampare il messaggio corretto dopo aver affondato una nave */

struct sea{
	int mare[DIM_MARE][DIM_MARE]; 	/* matrice contenente le navi: 	valore > 0 nave viva, 
																	valore = 0 casella vuota
																	valore < 0 nave colpita 
																	|valore| = dimensione nave
									*/
	int tentativi[DIM_MARE][DIM_MARE]; /* matrice contenente i tentativi:	valore < 0 nave nemica colpita ( COLPITO = -1 )
																			valore = 0 casella non provata
																			valore > 0 casella provata vuota (MANCATO = 2)*/
	int pointer_x, pointer_y; /* x ed y del puntatore usato per sparare al nemico */
	int px, py; /* x ed y usate per stampare i messaggi */
	short unsigned int navi_affondate[NNAVI]; /* array di 0-1 per tenere traccia delle navi affondate */
	int action; /* variabile usata per stampare il messaggio corretto a seconda dell'esito dell'attacco */
} *player1, *player2;

short unsigned int DoomsdayClockPlayer1, DoomsdayClockPlayer2, Player1_surrender;
/* 	Ogni volta che un giocatore affonda una nave aumenta di 1 il contatre dell'altro giocatore.
	Se il contatore arriva a NNAVI o Player1_surrender == 1 il gioco termina*/

struct strategy{
	int prob[DIM_MARE][DIM_MARE]; /* matric contenente le probabilità di trovare una nave nemica in una determinata posizione */
	int max_prob; /* Massima probabilità tra tutte quelle presenti nella matrice prob */
} *AI;

/*
Se possibile la funzione inserisce la nave di dimensione dimnav nel tavolo 
in direzione orizzontale (se dir="->")/verticale se (se dir="^^") 
a partire dalla posizione piu' a sinistra/bassa (px,py).
La funzione restituisce dimnav se l'inserimento ha avuto successo.
La funzione restituisce -n se la nave andasse a collidere con la nave di dimensione n.
Se la nave non puo' essere posizionata per altri motivi la funzione restituisce 0.
*/
int inserisci(struct sea *board, int dimnav, int px, int py, char* dir){
	int i=0;
	int direzione = -1;
	
	/* Guardo in quale direzione deve essere inserita la nave */
	if(!strcmp(dir,"->"))
		direzione = RIGHT;
	else
	if(!strcmp(dir,"^^"))
		direzione = UP;
	else
		return 0; /* direzione non valida */

	/*Controllo se la nave può essere posizionata*/
	while(i < dimnav){
		if(direzione == RIGHT)
		{
			if(py+i >= DIM_MARE)
				return 0; /* uscito dai bordi */
			else 
			if(board->mare[px][py+i] != 0)
				return -board->mare[px][py+i]; /* collisione con un'altra nave */
		}
		else /* direzione == UP */
		{
			if(px-i < 0)
				return 0; /* uscito dai bordi */
			else 
			if(board->mare[px-i][py] != 0)
				return -board->mare[px-i][py]; /* collisione con un'altra nave */
		}
		i++;
	}

	/*Se arrivo qui la nave può essere posizionata*/
	for(i = 0; i<dimnav; i++){
			if(direzione == RIGHT)
				board->mare[px][py+i]=dimnav;
			else /* direzione == UP */
				board->mare[px-i][py]=dimnav;
		}

	return dimnav;
}

/* Stampa a schermo il tavolo da gioco del giocatore 1 ed i suoi tentativi */
void stampa(struct sea *board){
	int i,j;

	system("clear");
	printf("Use a,s,d,w to aim and ENTER to fire\nUse x to surrender\n\n");
	
	switch(player1->action){
		case MANCATO: 	printf("Hai mancato il nemico in (%d,%d)\n", player1->px, player1->py);
						break;
		case COLPITO:	printf("Hai colpito il nemico in (%d,%d)\n", player1->px, player1->py);
						break;
		case AFFONDATO:	printf("Hai colpito ed affondato il nemico in (%d,%d)\n", player1->px, player1->py);
						break;
		case START:		printf("\n");
						break;
	}

	switch(player2->action){
		case MANCATO: 	printf("Il nemico ti ha mancato in (%d,%d)\n", player2->px, player2->py);
						break;
		case COLPITO:	printf("Il nemico ti ha colpito in (%d,%d)\n", player2->px, player2->py);
						break;
		case AFFONDATO:	printf("Il nemico ti ha colpito ed affondato in (%d,%d)\n", player2->px, player2->py);
						break;
		case START:		printf("\n");
						break;
	}

	printf("\n   ");
	for(i=0; i<DIM_MARE; i++)
		printf(ANSI_COLOR_CYAN " %d " ANSI_COLOR_RESET,i);

	printf("\t   ");
	for(i=0; i< DIM_MARE; i++)
		printf(ANSI_COLOR_CYAN " %d " ANSI_COLOR_RESET,i);
	
	for(i=0; i<DIM_MARE; i++){
		for(j=0; j< DIM_MARE; j++){
			if (j == 0)
				printf(ANSI_COLOR_CYAN "\n %d " ANSI_COLOR_RESET, i);

			if(board->mare[i][j] == 0)
				printf(ANSI_COLOR_BLUE " _ " ANSI_COLOR_RESET); /* casella vuota */

			if(board->mare[i][j] < 0)
				printf(ANSI_COLOR_RED " + " ANSI_COLOR_RESET); /* casella colpita */

			if(board->mare[i][j] > 0 )
				printf(ANSI_COLOR_YELLOW " %d " ANSI_COLOR_RESET,board->mare[i][j]); /* nave */
		} 

	printf("\t");
		
		for(j=0; j< DIM_MARE; j++){
			if (j == 0)
				printf(ANSI_COLOR_CYAN " %d " ANSI_COLOR_RESET, i);
		
			if( (i == board->pointer_x) && (j == board->pointer_y))
				printf(ANSI_COLOR_RED " X " ANSI_COLOR_RESET); /* posizione puntatore */
			else
			if(board->tentativi[i][j] < 0 ){ /* nave nemica colpita */
				if(player2->navi_affondate[-(player2->mare[i][j])] == FALSE)
					printf(ANSI_COLOR_YELLOW " + " ANSI_COLOR_RESET);
				else
					printf(ANSI_COLOR_RED " + " ANSI_COLOR_RESET);
			}
			else
			if(board->tentativi[i][j] == MANCATO )
				printf(ANSI_COLOR_GREEN " O " ANSI_COLOR_RESET); /* casella già provata */
			else		
				printf(ANSI_COLOR_BLUE " _ " ANSI_COLOR_RESET); 
		}
	} 
	printf("\n\n");
}

/* Stampa a schermo la tabella delle probabilità usate dal player2 per decidere dove sparare */
void stampa_AI(){
	int i,j;
		printf("Max Prob = %d\n", AI->max_prob);
		printf("\n\n   ");
		for(i=0; i<DIM_MARE; i++)
			printf(" %d ",i);
		
		for(i=0; i<DIM_MARE; i++){
			for(j=0; j< DIM_MARE; j++){
				if (j == 0)
					printf("\n %d ", i);

				if(AI->prob[i][j] > 0 )
					printf(" %d ",AI->prob[i][j]); /* nave */
				else		
					printf(" _ ");
			} 

		} 
		printf("\n\n");
}

/*
La funzione inserisce NNAVI navi casualmente nel tavolo che le viene passato.
Le navi inserite hanno dimensione 1,2,...,NNAVI.
*/
void random_board(struct sea *board){
	int i, ris;

	for(i=1; i <= NNAVI; i++){
		do{
			if(rand()%2)
				ris = inserisci(board, i, rand()%DIM_MARE, rand()%DIM_MARE, "->");
			else
				ris = inserisci(board, i, rand()%DIM_MARE, rand()%DIM_MARE, "^^");
		}
		while(ris <= 0);
	}
}

/*
Se in (px,py) era presente una nave non colpita in quella posizione, la funzione restiuisce -n
Il tavolo in posizione (px,py) viene impostato a -n.
La funzione restituisce 0 se la nave sul tavolo era gia' colpita o se la zona di mare era vuota.
*/
int colpisci(struct sea *board, int px, int py){
		if(board->mare[px][py] > 0){
			board->mare[px][py] = -board->mare[px][py];
			return board->mare[px][py];
		}	
		return 0;
}

/* 	(px,py) è la posizione della casella colpita e contiene il valore -dimnav.
	Usando (x,y)={(-1,-1),(-1,0),(-1,1),(0,-1)...(1,0),(1,1)} verifico tutto intorno al punto (px,py)
	se in un raggio di dimnav posizioni vi è ancora una casella viva (cioè di valore dimnav) */
int affondata( struct sea *board, int px, int py){
	int i,x,y;
	for(i=0; i < -(board->mare[px][py]); i++){
		for(x=-1; x <= 1; x++ ){
			for(y=-1; y <= 1; y++ ){
				if(	px+(x*i) < DIM_MARE &&
					px+(x*i) >= 0 &&
					py+(y*i) < DIM_MARE &&
					py+(y*i) >= 0 &&
					board->mare[px+(x*i)][py+(y*i)]==-(board->mare[px][py]) )
						return FALSE; /* c'è ancora una casella viva di valore dimnav -> affondata = FALSE */
			}
		}
	}
	return TRUE; /* nessuna casella viva di valore dimnav nel range -> affondata = TRUE */
}

void move_cursor(struct sea *board, char dir){

	if(dir == 'a' && board->pointer_y > 0 ){ /* con 'a' muovo il cursore a sinistra */
		board->pointer_y = board->pointer_y - 1;
	}
	else
	if(dir == 'd' && board->pointer_y < (DIM_MARE-1)){ /* con 'd' muovo il cursore a destra */
		board->pointer_y = board->pointer_y + 1;
	}
	else
	if(dir == 'w' && board->pointer_x > 0 ){ /* con 'w' muovo il cursore in alto */
		board->pointer_x = board->pointer_x - 1;
	}
	else
	if(dir == 's' && board->pointer_x < (DIM_MARE-1)){ /* con 's' muovo il cursore in basso */
		board->pointer_x = board->pointer_x + 1;
	}

	player1->action = START;
	player2->action = START;

	stampa(board);
}

void turno_player1(){
	char ch = ' ';
	int ris = -1;

	while(ch != '\n') { /* muove il cursore nelle direzioni indicate tramite wasd e spara quando viene permuto ENTER */
	        if(kbhit()) {
	            ch = readch();
	            if(ch == 'a' || ch == 's' || ch == 'd'|| ch == 'w')
	            	move_cursor(player1, ch);
	            else
	            if(ch == 'x'){ /* con x player1 si arrende */
	            	Player1_surrender = TRUE;
	            	return;
	            }
	        }
	    }

	    player1->px = player1->pointer_x;
		player1->py = player1->pointer_y;

	    if(player1->tentativi[player1->pointer_x][player1->pointer_y] == 0){ /* tentativo non ancora effettuato */
	    	
	    	ris = colpisci(player2, player1->pointer_x, player1->pointer_y);

		    if(ris < 0) /* colpito qualcosa */
		    {	player1->tentativi[player1->pointer_x][player1->pointer_y] = COLPITO;
		    	player1->action = COLPITO;

		       	player2->mare[player1->pointer_x][player1->pointer_y] = ris;

		       
				if(affondata(player2, player1->pointer_x, player1->pointer_y)){ /* verifico se il colpo ha affondato la nave */
					player2->navi_affondate[-ris] = TRUE;
					player1->action = AFFONDATO;
					DoomsdayClockPlayer2++;
				}
		   	}
		    else{
		    	player1->tentativi[player1->pointer_x][player1->pointer_y] = MANCATO;
		    	player1->action = MANCATO;
		    }
		}
}
/* Funzione usata da player2 per scegliere dove sparare quando AI->max_prob == 0 */
void random_target(){
	do{
			player2->pointer_x = rand()%DIM_MARE;
			player2->pointer_y = rand()%DIM_MARE;
		}while(player2->tentativi[player2->pointer_x][player2->pointer_y] != 0); /* seleziona coordinate casuali che non siano già state provate */
}

/* 	Funzione usata da player2 per scegliere il bersaglio dove sparare quando AI->max_prob > 0  
	Scorre tutto il board alla ricerca di una locazione con probabilità AI->max_prob.
	Quando la trova salva in pointer_x e pointer_y le coordinate di tale locazione.
*/
void find_target(){
	int i, j;
	
	for(i=0; i<DIM_MARE; i++)
		for (j = 0; j < DIM_MARE; j++)
		{
			if(AI->prob[i][j] == AI->max_prob && player2->tentativi[i][j] == 0){
				player2->pointer_x = i;
				player2->pointer_y = j;
				break;
			}
		}
}

/* 	Quando viene colpita una nave in posizione (px,py) la probabilità di trovare una nave nelle caselle adiacenti aumenta
	La probabilità delle caselle immediatamente adiacenti viene aumentata di 2, 
	La probabilità delle caselle a distanza uno viene aumentata di 1;
Supponiamo di colpire una nave in posizione (3,2):

  0 1 2 3 4 		  0 1 2 3 4
0 _|_|_|_|_			0 _|_|_|_|_
1 _|_|_|_|_			1 _|_|1|_|_
2 _|_|_|_|_			2 _|_|2|_|_
3 _|_|x|_|_   --> 	3 1|2|x|2|1
4 _|_|_|_|_			4 _|_|2|_|_
5 _|_|_|_|_			5 _|_|1|_|_
6 _|_|_|_|_			6 _|_|_|_|_

Non posso sapere in quale direzione è orientata la nave quindi tutte le adiacenti caselle sono equiprobabili (prob = 2)

Supponiamo di effettuare il successivo tentativo in posizione (4,2):

  0 1 2 3 4			  0 1 2 3 4
0 _|_|_|_|_			0 _|_|_|_|_
1 _|_|1|_|_			1 _|_|1|_|_
2 _|_|2|_|_			2 _|_|3|_|_
3 1|2| |2|1   -->	3 1|2|_|2|1
4 _|_|x|_|_			4 1|2|x|2|1
5 _|_|1|_|_			5 _|_|3|_|_
6 _|_|_|_|_			6 _|_|1|_|_

Ora suppongo che la nave sia orientata per verticale quindi le caselle lungo questa direzione sono più probabili (prob = 3)
*/
void add_prob(int px, int py){
	int i;
	
	for(i=-2; i<=2; i++){
		if(i!=0){
			if(px + i < DIM_MARE && 0 <= px + i && player2->tentativi[px+i][py] == 0){
				AI->prob[px+i][py] += (3-abs(i));
			}
			if(py + i < DIM_MARE && 0 <= py + i && player2->tentativi[px][py+i] == 0){
				AI->prob[px][py+i] += (3-abs(i));
			}
		}
	}
}

/* Funzione per rimuovere le probabilità generate da una nave che è stata affondata */
void remove_prob( int val ){
	int i,j,k;
	
	#ifdef DEBUG
		printf("remove_prob\n");
	#endif

	for(i=0; i<DIM_MARE;i++)
		for(j=0; j<DIM_MARE;j++)
			if(player1->mare[i][j] == val){ /* cerco le posizioni corrispondenti alla nave affondata */
				for(k=-2; k<=2; k++){
					if(k!=0) /* questa posizione è già stata azzerata, devo occuparmi delle adiacenti */
					{	if( i + k < DIM_MARE && 0 <= i + k )
							AI->prob[i+k][j] -= (3-abs(k)); /* effettuo il procedimento inverso di add_prob */
						
						if(j + k < DIM_MARE && 0 <= j + k )
							AI->prob[i][j+k] -= (3-abs(k));
					}
				}
			}
}

/* 	Scorre tutta la tavola delle probabilità alla ricerca del massimo valore
	Salva il valore trovato in AI->max_prob */
void find_max_prob(){
	int i,j, prob = 0;
	
	for(i=0; i<DIM_MARE;i++)
		for(j=0; j<DIM_MARE;j++)
			if(AI->prob[i][j] > prob)	
				prob = AI->prob[i][j];

	AI->max_prob = prob;
}

void turno_player2(){
	int ris = -1;

	find_max_prob(); /* aggiorno le max prob */

	if( AI->max_prob == 0)
		random_target();
	else
		find_target();
	
		ris = colpisci(player1, player2->pointer_x, player2->pointer_y);

		AI->prob[player2->pointer_x][player2->pointer_y] = 0; /* quando sparo in una posizione la sua prob non mi serve più */

		player2->px = player2->pointer_x;
		player2->py = player2->pointer_y;

		if(ris < 0) /* ho colpito qualcosa */
	    {	player2->tentativi[player2->pointer_x][player2->pointer_y] = COLPITO;
	       	player1->mare[player2->pointer_x][player2->pointer_y] = ris;

	       	player2->action = COLPITO;

	       	add_prob(player2->pointer_x,player2->pointer_y); /* aggiorno la tavola con le probabilità */

			if(affondata(player1, player2->pointer_x, player2->pointer_y)){
				player1->navi_affondate[-ris] = TRUE;
				player2->action = AFFONDATO;
				remove_prob(ris); /* rimuovo le prob generate dalla nave che è appena stata affondata */
				DoomsdayClockPlayer1 ++;
			}
	   	}
	    else
	    {	player2->tentativi[player2->pointer_x][player2->pointer_y] = MANCATO;
	    	player2->action = MANCATO;
	    }
}

int main(int argc, char *argv[]){
	 
	player1 = (struct sea *)calloc(1,sizeof(struct sea));
	player2 = (struct sea *)calloc(1,sizeof(struct sea));
	AI = (struct strategy *)calloc(1,sizeof(struct strategy));

	srand(time(NULL)+getpid());

	random_board(player1);
	random_board(player2);

	stampa(player1);
	
	#ifdef DEBUG
		stampa(player2);
	#endif

    init_keyboard();

    while(DoomsdayClockPlayer1 < NNAVI && DoomsdayClockPlayer2 < NNAVI && Player1_surrender == FALSE){

    	turno_player1();
    	if(Player1_surrender == FALSE) 
    		turno_player2();

    	stampa(player1);

    	#ifdef DEBUG
    		stampa_AI();
    	#endif
	}

	if(DoomsdayClockPlayer1 == NNAVI)
		printf("Player2 wins\n");
	if(DoomsdayClockPlayer2 == NNAVI)
		printf("Player1 wins\n");
	if(Player1_surrender == TRUE)
		printf("Player1 surrenders\n");
    
    close_keyboard();
	return 0;
}