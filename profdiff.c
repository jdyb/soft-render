
#include <stdio.h>
#include <string.h>

#include "prof.h"

struct profiledata {
	char             name[128];
	struct profblock blocks[PROFNAME_LAST];
};

int main(int argc, char *argv[])
{
	unsigned data_count = 0;
	struct profiledata data[128];

	if (argc < 3) {
		fputs("./profdiff START NEXT ...\n", stderr);
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		FILE *f = NULL;

		f = fopen(argv[i], "r");
		if (!f) {
			return 1;
		}

		strncpy(data[i - 1].name, argv[i], sizeof(data[0].name));

		/* FIXME Use proper file format for profile data. */
		/* FIXME Check return of fread. */
		fread(&(data[i - 1].blocks), sizeof(data[0].blocks[0]), PROFNAME_LAST, f);

		fclose(f);
	}

	data_count = argc - 1;

	for (unsigned i = 0; i < data_count; i++) {
		printf("name: %s\n", data[i].name);
		for (unsigned k = 0; k < PROFNAME_LAST; k++) {
			struct profblock *pb = &(data[i].blocks[k]);
			printf("%3x %8s %12u %12u %9.1f HZ (%0.5f)\n",
					k,
					profstr[k],
					pb->count_last_frame,
					pb->count_total,
					1 / pb->clock_last_frame,
					pb->clock_last_frame);
		}
	}

	for (unsigned i = 0; i < (data_count - 1); i ++) {
		unsigned j = i + 1;
		printf("diff: %s %s\n", data[i].name, data[j].name);
		for (unsigned k = 0; k < PROFNAME_LAST; k++) {
			struct profblock *a = &(data[i].blocks[k]);
			struct profblock *b = &(data[j].blocks[k]);

			unsigned a_count = a->count_total;
			unsigned b_count = b->count_total;

			float a_clock = a->clock_total;
			float b_clock = b->clock_total;

			float a_clock_avg = a_clock / a_count;
			float b_clock_avg = b_clock / b_count;

			float a_avg_rate = 1 / a_clock_avg;
			float b_avg_rate = 1 / b_clock_avg;

			float diff_avg_rate = b_avg_rate - a_avg_rate;

			/* FIXME Pixel statistics are currently unreliable. */
			if (k == PROFNAME_PIXEL)
				continue;

			printf("%s %12u %12u %9.1f %9.1f %9.1f\n",
					profstr[k],
					a_count,
					b_count,
					a_avg_rate,
					b_avg_rate,
					diff_avg_rate);
		}
	}

	return 0;
}

