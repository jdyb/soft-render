
.PHONY: all clean run prof

all: soft-render profdiff

clean:
	rm ./soft-render

run: soft-render
	./soft-render

prof: soft-render
	./soft-render
	mv prof profs/prof-$(shell date +%F-%H-%M-%S)

soft-render: main.c prof.h
	$(CC) -o soft-render main.c -lSDL2 -lSDL2_ttf -lm

profdiff: profdiff.c prof.h
