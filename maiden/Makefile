target = maiden

cflags = -g

src = main.c

inc =

lib = -lmonome -llua

$(target): $(src)
	gcc $(src) $(inc) $(cflags) -o $(target) $(lib)

clean:
	rm $(target)
