all:
	cc ssh-import-id-gh.c -L/usr/local/lib -lcurl -I/usr/local/include -Wall -Wextra -Werror -o ssh-import-id-gh
	# gcc ssh-import-id-gh.c -o ssh-import-id-gh.o -lcurl

debug:
	cc -g ssh-import-id-gh.c -L/usr/local/lib -lcurl -I/usr/local/include -Wall -Wextra -Werror -o ssh-import-id-gh

clean:
	rm ssh-import-id-gh
