#include <iostream>
#include <getopt.h>

#include <AST.h>

#define VERSION_STRING "0.1"

int parse();
int parse(FILE *fp, const AST::CompilationFlags& flags);

int main(int argc, char** argv)
{
	AST::CompilationFlags flags;
	flags.output = "a.out";
	//std::cout << "l++ v0.1" << std::endl;
	
	if(argc < 2)
		return 0;
	
	int opt;
	while((opt = getopt(argc, argv, "mvhs:o:I:")) != -1)
	{
		switch (opt)
			{
		case 's':
				flags.input = optarg;
		break;
				
		case 'o':
				flags.output = optarg;
		break;

		case 'h':
				//usage(argv[0]);
				exit(EXIT_SUCCESS);
				break;

		case 'm':
			flags.isModule = true;
			break;
				
		case 'v':
				std::cout << "l++ v" << VERSION_STRING << std::endl;
				exit(EXIT_SUCCESS);
				break;

		case 'I':
				flags.includePath = optarg;
		break;
		default:
				//usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	
	FILE* file = fopen(flags.input.c_str(), "r");
	if(!file)
	{
		perror("Could not open source file");
		return 1;
	}
	
	parse(file, flags);
	return 0;
}

