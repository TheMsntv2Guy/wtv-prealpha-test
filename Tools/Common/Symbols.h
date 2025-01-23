#define	kMaxSymbols	10000

#define	kSymbolNameLen	64

typedef struct {
	unsigned long 	address;
	char			symbol[kSymbolNameLen];
} Symbol;

typedef struct {
	unsigned long cookie;
	long count;
} SymHeader;


#define	kSymbolSignature 0x74696d6e	

#define START 	"SYMBOL TABLE:"
#define END 	"XXXXXX"
