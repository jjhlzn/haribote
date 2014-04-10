char msg[100];
int main(int argc, char **argv){
	int i=0;
	for(i = 0; i<argc; i++){
	       sprintf(msg,"argv[%d] = %s\n",i,argv[i]);
	       _api_putstr0(msg);	
	}
	return 0;
}       
