PORTNAME=	ssh-import-id
DISTVERSION=	5.11
CATEGORIES=	security python
MASTER_SITES=	CHEESESHOP
PKGNAMEPREFIX=	${PYTHON_PKGNAMEPREFIX}

MAINTAINER=	p.mousavizadeh@protonmail.com
COMMENT=	Authorize SSH public keys from trusted online identities
WWW=		https://git.launchpad.net/ssh-import-id

LICENSE=	GPLv3

RUN_DEPENDS=	${PYTHON_PKGNAMEPREFIX}distro>0:sysutils/py-distro@${PY_FLAVOR}

USES=		python:2.6+
USE_PYTHON=	autoplist distutils

.include <bsd.port.mk>
