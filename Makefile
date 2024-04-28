CFLAGS= -Wall -Werror -fsanitize=address

all: SimpleServer PersistentServer PipelinedServer

SimpleServer: SimpleServer.c Helper.c
	gcc $(CFLAGS) -o $@ $^

PersistentServer: PersistentServer.c Helper.c 
	gcc $(CFLAGS) -o $@ $^

PipelinedServer: PipelineServer.c Helper.c 
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -f SimpleServer PersistentServer PipelinedServer
