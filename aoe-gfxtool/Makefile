
GFXTOOLOBJS=	gfxtool.obj \
				palette.obj \
				qdbmp.obj

gfxtool : $(GFXTOOLOBJS)
	$(CC) $(LDFLAGS) -o gfxtool $(GFXTOOLOBJS) $(LIBS)

gfxtool.obj:	gfxtool.c
	$(CC) $(CFLAGS) -c gfxtool.c -o gfxtool.obj

palette.obj:	palette.c
	$(CC) $(CFLAGS) -c palette.c -o palette.obj

qdbmp.obj:	qdbmp.c qdbmp.h
	$(CC) $(CFLAGS) -c qdbmp.c -o qdbmp.obj
