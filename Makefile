PORTNAME=		ssh-import-id
DISTVERSION=	5.11
CATEGORIES=		security python
MASTER_SITES=	CHEESESHOP
PKGNAMEPREFIX=	${PYTHON_PKGNAMEPREFIX}

MAINTAINER= 	p.mousavizadeh@protonmail.com
COMMENT=		Authorize SSH public keys from trusted online identities

LICENSE=		GPLv3

USES=			python:3.9
USE_PYTHON=		autoplist distutils

PLIST_FILES=	bin/ssh-import-id-gh \
				bin/ssh-import-id-lp

.include <bsd.port.mk>
