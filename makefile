
.PHONY: all clean run

all: soft-render

clean:
	rm ./soft-render

run: soft-render
	./soft-render

soft-render: main.c
	$(CC) -o soft-render main.c -lSDL2 -lSDL2_ttf -lm

