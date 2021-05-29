#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "main.h"
#include "disassemble.h"

bool g_verbose = false;

void printUsage(const char *prog) {
	if (!prog)
		prog = "picard";

	printf("Usage: %s [options] files\n"
		"Options:\n"
		"  -D, --dvle <index>    Specifies the dvle to print info from\n"
		"  -V, --verbose         Enables printing of verbose messages\n"
		"  -v, --version         Displays version information\n"
		"  -h, --help            Displays this message\n"
		, prog);
}

void printVersion(void) {
	puts("Compiled on " __DATE__ " " __TIME__);
	puts(PROJECT_NAME " v" PROJECT_VERSION);
}

int main(int argc, char *argv[]) {
	int dvleIndex = -1;
	int errTotal = 0;

	if (argc <= 1) {
		printUsage(argv[0]);
		return 1;
	}

	const struct option long_options[] = {
		{ "dvle",    required_argument, NULL, 'D', },
		{ "verbose", no_argument,       NULL, 'V', },
		{ "version", no_argument,       NULL, 'v', },
		{ "help",    no_argument,       NULL, 'h', },
		{ NULL,      no_argument,       NULL,   0, },
	};

	int option;
	while ((option = getopt_long(argc, argv, ":D:Vvh", long_options, NULL)) != -1) {
		int this_optind = optind ? optind-1 : 1;
		switch (option) {
			case 'D':
				dvleIndex = atoi(optarg);
				break;
			case 'V':
				g_verbose = true;
				break;
			case 'v':
				printVersion();
				return 0;
			case 'h':
				printUsage(argv[0]);
				return 0;
			case ':':
				printf("option needs a value\n");
				break;
			case '?': //used for some unknown options
				printf("unknown option: %s\n", argv[this_optind]);
				break;
		}
	}

	while (optind < argc) {
		char *bin = NULL;
		const char *filename = argv[optind++];
		FILE *fp = fopen(filename, "rb");

		if (!fp) {
			printf("Error: could not open file %s\n", filename);
			continue;
		} else
			verbose("Processing %s\n", filename);

		char magic[5] = {0};
		fread(magic, sizeof(char), 4, fp);
		if (memcmp("DVLB", magic, 4)==0) {
			fseek(fp, 0, SEEK_END);
			uint32_t len = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			bin = malloc(len+1);
			fread(bin, 1, len, fp);
		} else if (memcmp("CGFX", magic, 4)==0) {
			// Lazy hack TODO::Posibly fix this
			fseek(fp, 0, SEEK_END);
			uint32_t fullen = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			for (uint32_t i = 0; i + 4 < fullen; i+=4) {
				fread(magic, sizeof(char), 4, fp);
				if (memcmp("DVLB", magic, 4)==0) {
					verbose("%s found. offset: 0x%x\n", magic, i);
					fseek(fp, i, SEEK_SET);
					bin = malloc((fullen-i)+1);
					fread(bin, 1, fullen-i, fp);
					break;
				}
			}
		} else {
			printf("Invalid magic %s\n", magic);
			continue;
		}

		fclose(fp);

		dmp_pica_info pinfo = picaParseHeader(bin);
		int err = picaDisass(&pinfo, dvleIndex);
		errTotal += err > 0 ? err : 0;
		picaFinish(&pinfo);
		if (bin) free(bin);
	}

	if (errTotal)
		error("%d errors occured. See above for details.\n", errTotal);

	return 0;
}
