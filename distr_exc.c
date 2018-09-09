/**
 * Distributed Systems - Itai-Rodeh Algorithm for Anonymous Rings
 * Written by Kosmas Sidiropoulos
 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stddef.h>
#include <time.h>

typedef struct token {
    int level;
    int id;
    int hops;
    int un;	//bit 1:true, 0:false

    int ready;	//Used for the send <ready> message
} token;

struct stats	//The level, id, and state of each process
{
	int level;
	int id;
	int state;	//0:sleep, 1:cand, 2:leader, 3:lost
}

print_stats(int rank, int state, struct token tok)
{
	printf("Rank %d, state %d, level %d, id %d, hops %d, un %d, ready %d\n", 
	       rank, state, tok.level, tok.id, tok.hops, tok.un, tok.ready);
}

int main(int argc, char **argv) {

    const int token_tag = 66;	//Tag used for the communication in the ring.
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr,"Requires at least two processes.\n");
        exit(-1);
    }

	/* Create a MPI_Datatype so we can send a struct to the next process*/
    const int nitems = 5;
    int blocklengths[5] = {1,1,1,1,1};
    MPI_Datatype types[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype MPI_TOKEN;
    MPI_Aint     offsets[5];
    offsets[0] = offsetof(token, level);
    offsets[1] = offsetof(token, id);
    offsets[2] = offsetof(token, hops);
    offsets[3] = offsetof(token, un);
    offsets[4] = offsetof(token, ready);
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_TOKEN);
    MPI_Type_commit(&MPI_TOKEN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Status status;
    srand(time(NULL) + rank);	/* Random generator with rank as a seed for the id */

    /*Init p*/
    struct stats pstats;
    pstats.level = 1;
    pstats.id = 1 + rand() % size;
    pstats.state = 1;

    int p = rank;
    int p_next = (rank == size - 1)? 0 : p+1;	//Calculate the next process in the ring.
    int src = (p > 0) ? p - 1 : size - 1;	//Calculate the previous process in the ring.
	token snd_tok;	//Send token
	token rcv_tok;	//Receive token
	
	snd_tok.level = pstats.level;
	snd_tok.id = pstats.id;
	snd_tok.hops = 1;
	snd_tok.un = 1;
	snd_tok.ready = 0;
    //Sends init values to the next process in the ring.
    MPI_Send(&snd_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);

	int stop = 0;
    while(stop == 0)
    {
	    //Receive the token from previous process
        MPI_Recv(&rcv_tok, 1, MPI_TOKEN, src, token_tag, MPI_COMM_WORLD, &status);	
		//print_stats(rank, pstats.state, rcv_tok);

        if(rcv_tok.ready == 0)
        {
            if(rcv_tok.hops == size && pstats.state == 1)
            {
				/*send <ready> to p_next */
				if(rcv_tok.un == 1)
				{
					pstats.state = 2;
					stop = 1;
					snd_tok.ready = 1;
					MPI_Send(&snd_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);
				}
				/* send <tok, level_p id_p, 1, true> to p_next*/
				else
				{
					pstats.id = 1 + rand() % size;
					pstats.level++;
					
					snd_tok.level = pstats.level;
					snd_tok.id = pstats.id;
					snd_tok.hops = 1;
					snd_tok.un = 1;
					snd_tok.ready = 0;
					MPI_Send(&snd_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);
				}
			}
			else
			{
				/* <tok, level, id, hops+1, false> */
				if(rcv_tok.level == pstats.level && rcv_tok.id == pstats.id)
				{
					snd_tok.level = rcv_tok.level;
					snd_tok.id = rcv_tok.id;
					snd_tok.hops = rcv_tok.hops + 1;
					snd_tok.un = 0;
					snd_tok.ready = 0;
					MPI_Send(&snd_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);
				}
				/*send <tok, level, id, hops+1, bit> to Next_p */
				else if((rcv_tok.level > pstats.level && rcv_tok.id > pstats.id) || 
						(rcv_tok.level == pstats.level && rcv_tok.id > pstats.id) || 
						(rcv_tok.level > pstats.level && rcv_tok.id == pstats.id))
				{
					pstats.state = 3;
					snd_tok.level = rcv_tok.level;
					snd_tok.id = rcv_tok.id;
					snd_tok.hops = rcv_tok.hops + 1;
					snd_tok.un = rcv_tok.un;
					snd_tok.ready = 0;
					MPI_Send(&snd_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);
				}
				else
				{
					/* Purge the send token by sending the receive token to the next process in the ring */
					MPI_Send(&rcv_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);
				}
			}
        }
        else
        {
			stop = 1;
			snd_tok.ready = 1;
			/*send <ready> to p_next */
			MPI_Send(&snd_tok, 1, MPI_TOKEN, p_next, token_tag, MPI_COMM_WORLD);
		}
    }
    
    printf("rank %d state %d : %s\n", rank, pstats.state, (pstats.state == 3) ? "lost" : "leader");

    MPI_Type_free(&MPI_TOKEN);	//Free the MPI_Datatype we created.
    MPI_Finalize();

    return 0;
}
