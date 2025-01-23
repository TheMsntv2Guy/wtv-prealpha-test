CC = /tools/gnu/bin/gcc 
CFLAGS = -B/usr/bin/ -I/usr/include -m32
OBJ = ../../objects/tools

all: ${OBJ}
	@echo ""
	@echo "    -> Tools"
	@gmake -s tools

tools: ${OBJ}/snarfsyms ${OBJ}/buildrom ${OBJ}/lus ${OBJ}/romify
 

${OBJ} :
	@mkdir -p ${OBJ}


#---- snarf ---------------------------
${OBJ}/snarfsyms:	${OBJ}/SnarfSyms.o ${OBJ}/cplus-dem.o
	echo "       Linking ${notdir $@}"
	$(CC) $(CFLAGS) -o ${OBJ}/snarfsyms ${OBJ}/SnarfSyms.o ${OBJ}/cplus-dem.o

${OBJ}/SnarfSyms.o:	SnarfSyms.c Symbols.h
	echo "       Compiling $(<F)"
	$(CC) -c $(CFLAGS)  SnarfSyms.c -o ${OBJ}/SnarfSyms.o

${OBJ}/cplus-dem.o:	cplus-dem.c demangle.h
	echo "       Compiling $(<F)"
	$(CC) -c $(CFLAGS) cplus-dem.c -o ${OBJ}/cplus-dem.o


#---- buildrom ---------------------------
${OBJ}/buildrom:	${OBJ}/BuildROM.o
	echo "       Linking ${notdir $@}"
	$(CC) $(CFLAGS) -o ${OBJ}/buildrom ${OBJ}/BuildROM.o

${OBJ}/BuildROM.o:	BuildROM.c 
	echo "       Compiling $(<F)"
	$(CC) -c $(CFLAGS)  BuildROM.c -o ${OBJ}/BuildROM.o


#---- LookupSymbol ---------------------------
${OBJ}/lus:	${OBJ}/LookupSymbol.o
	echo "       Linking ${notdir $@}"
	$(CC) $(CFLAGS) -o ${OBJ}/lus ${OBJ}/LookupSymbol.o

${OBJ}/LookupSymbol.o:	LookupSymbol.c Symbols.h
	echo "       Compiling $(<F)"
	$(CC) -c $(CFLAGS)  LookupSymbol.c -o ${OBJ}/LookupSymbol.o



#---- romify ---------------------------
${OBJ}/romify:	${OBJ}/uromify.o
	echo "       Linking ${notdir $@}"
	$(CC) $(CFLAGS) -o ${OBJ}/romify ${OBJ}/uromify.o

${OBJ}/uromify.o:	uromify.c 
	echo "       Compiling $(<F)"
	$(CC) -c $(CFLAGS) uromify.c -o ${OBJ}/uromify.o
