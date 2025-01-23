
#ifndef _NAMESER_H_
#define	_NAMESER_H_

/*
 * Define constants based on rfc883
 */
#define PACKETSZ	512		/* maximum packet size */
#define MAXDNAME	256		/* maximum domain name */
#define MAXCDNAME	255		/* maximum compressed domain name */
#define MAXLABEL	63		/* maximum length of domain label */
	/* Number of bytes of fixed size data in query structure */
#define QFIXEDSZ	4
	/* number of bytes of fixed size data in resource record */
#define RRFIXEDSZ	10

/*
 * Internet nameserver port number
 */
#define NAMESERVER_PORT	53

/*
 * Currently defined opcodes
 */
#define QUERY		0x0		/* standard query */
#define IQUERY		0x1		/* inverse query */
#define STATUS		0x2		/* nameserver status query */
/*#define xxx		0x3		/* 0x3 reserved */
	/* non standard - supports ALLOW_UPDATES stuff from Mike Schwartz */
#define UPDATEA		0x9		/* add resource record */
#define UPDATED		0xa		/* delete a specific resource record */
#define UPDATEDA	0xb		/* delete all named resource record */
#define UPDATEM		0xc		/* modify a specific resource record */
#define UPDATEMA	0xd		/* modify all named resource record */

#define ZONEINIT	0xe		/* initial zone transfer */
#define ZONEREF		0xf		/* incremental zone referesh */

/*
 * Currently defined response codes
 */
#define NOERROR		0		/* no error */
#define FORMERR		1		/* format error */
#define SERVFAIL	2		/* server failure */
#define NXDOMAIN	3		/* non existent domain */
#define NOTIMP		4		/* not implemented */
#define REFUSED		5		/* query refused */
	/* non standard */
#define NOCHANGE	0xf		/* update failed to change db */

/*
 * Type values for resources and queries
 */
#define T_A		1		/* host address */
#define T_NS		2		/* authoritative server */
#define T_MD		3		/* mail destination */
#define T_MF		4		/* mail forwarder */
#define T_CNAME		5		/* connonical name */
#define T_SOA		6		/* start of authority zone */
#define T_MB		7		/* mailbox domain name */
#define T_MG		8		/* mail group member */
#define T_MR		9		/* mail rename name */
#define T_NULL		10		/* null resource record */
#define T_WKS		11		/* well known service */
#define T_PTR		12		/* domain name pointer */
#define T_HINFO		13		/* host information */
#define T_MINFO		14		/* mailbox information */
#define T_MX		15		/* mail routing information */
#define T_TXT		16		/* text strings */
#define	T_RP		17		/* responsible person */
#define	T_AFSDB		18		/* AFS cell database */
#define	T_NSAP		22		/* NSAP address */
#define	T_NSAP_PTR	23		/* reverse lookup for NSAP */
	/* non standard */
#define T_UINFO		100		/* user (finger) information */
#define T_UID		101		/* user ID */
#define T_GID		102		/* group ID */
#define T_UNSPEC	103		/* Unspecified format (binary data) */
	/* Query type values which do not appear in resource records */
#define T_AXFR		252		/* transfer zone of authority */
#define T_MAILB		253		/* transfer mailbox records */
#define T_MAILA		254		/* transfer mail agent records */
#define T_ANY		255		/* wildcard match */

/*
 * Values for class field
 */

#define C_IN		1		/* the arpa internet */
#define C_CHAOS		3		/* for chaos net (MIT) */
#define C_HS		4		/* for Hesiod name server (MIT) (XXX) */
	/* Query class values which do not appear in resource records */
#define C_ANY		255		/* wildcard match */

/*
 * Status return codes for T_UNSPEC conversion routines
 */
#define CONV_SUCCESS 0
#define CONV_OVERFLOW -1
#define CONV_BADFMT -2
#define CONV_BADCKSUM -3
#define CONV_BADBUFLEN -4

/*
 * Structure for query header.  The order of the fields is machine- and
 * compiler-dependent, depending on the byte/bit order and the layout
 * of bit fields.  We use bit fields only in int variables, as this
 * is all ANSI requires.  This requires a somewhat confusing rearrangement.
 */

typedef struct {
#if HARDWARE
	ushort 		id;		/* query identification number */
			/* fields in third byte */
	unsigned	qr:1 __attribute__ ((packed));		/* response flag */
	unsigned	opcode:4 __attribute__ ((packed));	/* purpose of message */
	unsigned	aa:1 __attribute__ ((packed));		/* authoritive answer */
	unsigned	tc:1 __attribute__ ((packed));		/* truncated message */
	unsigned	rd:1 __attribute__ ((packed));		/* recursion desired */
			/* fields in fourth byte */
	unsigned	ra:1 __attribute__ ((packed));		/* recursion available */
	unsigned	pr:1 __attribute__ ((packed));		/* primary server required (non standard) */
	unsigned	unused:2 __attribute__ ((packed));	/* unused bits */
	unsigned  	rcode:4 __attribute__ ((packed));	/* response code */
				/* remaining bytes */
	ushort qdcount __attribute__ ((packed));	/* number of question entries */
	ushort ancount __attribute__ ((packed));	/* number of answer entries */
	ushort nscount __attribute__ ((packed));	/* number of authority entries */
	ushort arcount __attribute__ ((packed));	/* number of resource entries */
#else
#ifdef FOR_MAC
#pragma options align = mac68k
#endif
	unsigned    id:16;		/* query identification number */
			/* fields in third byte */
	unsigned	qr:1;		/* response flag */
	unsigned	opcode:4;	/* purpose of message */
	unsigned	aa:1;		/* authoritive answer */
	unsigned	tc:1;		/* truncated message */
	unsigned	rd:1;		/* recursion desired */
			/* fields in fourth byte */
	unsigned	ra:1;		/* recursion available */
	unsigned	pr:1;		/* primary server required (non standard) */
	unsigned	unused:2;	/* unused bits */
	unsigned  	rcode:4;	/* response code */
	unsigned	qdcount:16;  /* number of question entries */
	unsigned    ancount:16;	/* number of answer entries */
	unsigned    nscount:16;	/* number of authority entries */
	unsigned    arcount:16;	/* number of resource entries */
#ifdef FOR_MAC
#pragma options align = reset
#endif
#endif

} HEADER 
#ifndef FOR_MAC
__attribute__ ((aligned (2)))
#endif
;

/*
 * Defines for handling compressed domain names
 */
#define INDIR_MASK	0xc0

/*
 * Structure for passing resource records around.
 */
struct rrec {
	short	r_zone;			/* zone number */
	short	r_class;		/* class number */
	short	r_type;			/* type number */
	unsigned int	r_ttl;			/* time to live */
	int	r_size;			/* size of data area */
	char	*r_data;		/* pointer to data */
};

extern	unsigned short	_getshort(unsigned char *msgp);
extern	unsigned int	_getlong(unsigned char *msgp);

/*
 * Inline versions of get/put short/long.  Pointer is advanced.
 * We also assume that a "unsigned short" holds 2 "chars"
 * and that a "unsigned int" holds 4 "chars".
 *
 * These macros demonstrate the property of C whereby it can be
 * portable or it can be elegant but never both.
 */
#define GETSHORT(s, cp) { \
	uchar *t_cp = (uchar *)(cp); \
	(s) = ((ushort)t_cp[0] << 8) | (ushort)t_cp[1]; \
	(cp) += 2; \
}

#define GETLONG(l, cp) { \
	uchar *t_cp = (uchar *)(cp); \
	(l) = (((unsigned int)t_cp[0]) << 24) \
	    | (((unsigned int)t_cp[1]) << 16) \
	    | (((unsigned int)t_cp[2]) << 8) \
	    | (((unsigned int)t_cp[3])); \
	(cp) += 4; \
}

#define PUTSHORT(s, cp) { \
	ushort t_s = (ushort)(s); \
	uchar *t_cp = (uchar *)(cp); \
	*t_cp++ = t_s >> 8; \
	*t_cp   = t_s; \
	(cp) += 2; \
}

/*
 * Warning: PUTLONG --no-longer-- destroys its first argument.  if you
 * were depending on this "feature", you will lose.
 */
#define PUTLONG(l, cp) { \
	unsigned int t_l = (unsigned int)(l); \
	uchar *t_cp = (uchar *)(cp); \
	*t_cp++ = t_l >> 24; \
	*t_cp++ = t_l >> 16; \
	*t_cp++ = t_l >> 8; \
	*t_cp   = t_l; \
	(cp) += 4; \
}

#endif /* !_NAMESER_H_ */
