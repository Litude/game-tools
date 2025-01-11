
SHPTOOLOBJS=	shptool.obj

shptool : $(SHPTOOLOBJS)
	$(CC) $(LDFLAGS) -o shptool $(SHPTOOLOBJS) $(LIBS)

shptool.obj:	shptool.c
	$(CC) $(CFLAGS) -c shptool.c -o shptool.obj

