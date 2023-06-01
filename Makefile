all:
	cc ssh-import-id.c -L/usr/local/lib -lcurl -I/usr/local/include -Wall -Wextra -Werror -o ssh-import-id

debug:
	cc -g ssh-import-id.c -L/usr/local/lib -lcurl -I/usr/local/include -Wall -Wextra -Werror -o ssh-import-id

clean:
	rm ssh-import-id ssh-import-id-gh ssh-import-id-lp
