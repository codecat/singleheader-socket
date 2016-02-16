#include <cstdio>

#define EZS_CPP
#include "EzSock.h"

int main()
{
	EzSock s;
	s.create();
	s.bind(8810);
	s.listen();
	printf("Listening on 8810\n");

	while (true) {
		EzSock client;
		s.accept(&client);
		printf("Client connected\n");

		while (true) {
			unsigned char c = 0;
			int ret = client.Receive(&c, 1);
			printf("Ret: %d, char: %c\n", ret, c);
			if (c == 0) {
				break;
			}
		}
		printf("Client disconnected\n");
	}
	return 0;
}

