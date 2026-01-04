OS=$$(uname -o)

.PHONY: depends

default: build

depends:
	@echo "Install Dependencies"
	@if [ -e /etc/debian_version ]; then\
		dpkg -s libcurl4-openssl-dev &>/dev/null || DEBIAN_FRONTEND=noninteractive apt install -y libcurl4-openssl-dev;\
	fi

build: depends
	cc ssh-import-id.c -L/usr/local/lib -lcurl -I/usr/local/include -Wall -Wextra -Werror -o ssh-import-id

install: build
	cp ssh-import-id /usr/local/bin/

debug:
	cc -g ssh-import-id.c -L/usr/local/lib -lcurl -I/usr/local/include -Wall -Wextra -Werror -o ssh-import-id

clean:
	rm -f ssh-import-id ssh-import-id-gh ssh-import-id-lp

uninstall: clean
	rm /usr/local/bin/ssh-import-id
