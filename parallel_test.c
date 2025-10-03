#include "parallel.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    //If there are no parameters.
    if (argc < 2) {
	printf("How silly! You didn't put in any commands.\nHere's a template for you: ./parallel_test ls pwd\n");
	return 0;
    }
    //We're good? Let's get going!
    run_parallel_cmds(&argv[1]);
    return 0;
}
