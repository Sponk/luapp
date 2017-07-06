#include <iostream>

int parse();
int parse(FILE *fp, const char* file);

int main(int argc, char** argv)
{
	//std::cout << "l++ v0.1" << std::endl;
	
	if(argc < 2)
		return 0;
	
	FILE* file = fopen(argv[1], "r");
	if(!file)
	{
		perror("Could not open source file");
		return 1;
	}
	
	parse(file, argv[1]);	
	return 0;
}
