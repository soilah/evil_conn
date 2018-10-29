target: src/target_funcs.c src/target.c
	gcc src/target_funcs.c src/target.c -o target -Wall -g3 -I include -lpthread

attacker: src/attacker.c
	gcc src/attacker.c -o attacker -Wall -g3 -I include

clean:
	rm attacker log* target
