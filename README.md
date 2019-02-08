# Itai-Rodeh Algorithm for Anonymous Rings

The Itai-Rodeh is a leader selection algorithm used in a 
distrubuted system with anonymous rings. It has a time complexity
of O(Nlog(N)) and sends O(N) messages.

## The algorithm


	init:
	p_state = cand; level_p = 1; id_p = rand(1, ..., N);
 	send <tok, level, id, 1, true> to next_p;
 
	while not stop do {
		receive a message;
		if it is a token <tok, level, id, hops, un> then {
			if hops=N and state_p = cand then {
				if un then {
					state_p = leader; stop_p = true; send <ready> to p_next
				}
				else{
					level_p:=level_p + 1; id_p = rand({1, ..., N}); send <tok, level_p id_p, 1, true> to p_next
				}
			}
			else if((level > level_p && id > id_p) || (level == level_p && id > id_p) || (level > level_p && id == id_p)){
				state_p = lost;	send <tok, level, id, hops+1, bit> to p_next
			}
			else if(level == level_p && id == id_p){
				send <tok, level, id, hops+1, false> to p_next
			}
			else{
				send <tok, level, id, hops, bit> to p_next
			}
		}
		else{
			send <ready> to p_next; stop_p = true;
		}
    }

## How to compile and run
	
	mpicc distr_exc.c -o <output>
	mpirun -np <number_of_p> ./<output>
