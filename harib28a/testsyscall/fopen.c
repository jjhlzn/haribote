
#include <stdlib.h>
#include <stdio.h>

int main(int ac, char **av)
{
	int exit_status = EXIT_SUCCESS;
	FILE  *input;
	
	while( *++av != NULL ){
		printf("about to open [%s]\n", *av);
		input = fopen( *av, "r");
		if( input == NULL ){
			perror( *av );
			exit_status = EXIT_FAILURE;
			continue;
		}


		char msg[1024];
		int count = 20;
		fread(msg, sizeof(char), count, input);	
		msg[count] = 0;
		printf("file contents = [%s]\n",msg);
		
		if(fclose( input ) != 0){
			perror( "fclose" );
			exit( EXIT_FAILURE );
		}
	}
	
	return exit_status;
}
