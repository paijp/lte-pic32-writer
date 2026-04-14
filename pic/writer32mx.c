
#include	"header.h"

#pragma config  PMDL1WAY=OFF, IOL1WAY=OFF, FUSBIDIO=OFF, FVBUSONIO=OFF
#pragma config  FPLLIDIV=DIV_1, FPLLMUL=MUL_20, UPLLIDIV=DIV_1, UPLLEN=OFF
#pragma	config	FPLLODIV=DIV_2, FNOSC=FRCPLL, FSOSCEN=OFF, IESO=OFF
#pragma	config	POSCMOD=XT, OSCIOFNC=OFF, FPBDIV=DIV_1, FCKSM=CSECMD
#pragma	config	WDTPS=PS16384, WINDIS=OFF, FWDTEN=OFF, FWDTWINSZ=WINSZ_50
#pragma	config	DEBUG=OFF, JTAGEN=OFF, ICESEL=ICS_PGx2, PWP=OFF
#pragma	config	BWP=OFF, CP=OFF


/*jp.pa-i.cir/map32mx2-28
LSS	RPB3
LRS_T2P	RPB4
P2L_P2T	RPB5	SDO1

LCK_TCK	RPB14	SCK1

PGD	RPB10	PGED2
PGC	PGEC2
fix	CLKI	CLKO	VCAP
VBUS	RB6
VUSB3V3	RB12

P1__U2P	RPB8	URX2
P2__PGC0	RPA0	UTX1
P4__P2U	RPB0	UTX2
P5__MCLR0	RPB1
P6__PGD0	RPB2	URX1

*/

/*jp.pa-i.cir/pcbgrid20
** 10,0qr

** 4,13t10,0,0
B8
A0
A1
B0
B1
B2
15
13
SC
B9

** 3,13t10,0,0
(DA)
G
G
5V
5V
3V3
A0
A1
SD
(BT)


** 2,23r4,0,0
5V	88888888<0>>8889
G	88888888<0>>88
TX	88888888<1k>>8
RX	88888888<1k>>>>>>4


** 7,23t4,0,0
G	74<0>>>
C	78<0>>>>>>>77
D	98888888<0>>>>>>4444
M	88888888<0>>>>444


*/


#define	PORT_PGC0	PORTAbits.RA0
#define	TRIS_PGC0	TRISAbits.TRISA0
#define	LAT_MCLR0	LATBbits.LATB1
#define	PORT_PGD0	PORTBbits.RB2
#define	TRIS_PGD0	TRISBbits.TRISB2


#define	BUFFERSIZE	256
static	UH	p2cbuf[BUFFERSIZE];
static	W	p2cwpos = 0;
static	W	p2crpos = 0;

static	UB	p2ubuf[BUFFERSIZE];
static	W	p2uwpos = 0;
static	W	p2urpos = 0;

static	W	recverror = 0x8000;

#define	BLOCKSIZE	0x400
#define	ADDRHMASK	0xfffffc00
#define	WRITEBUFSIZE	32
static	struct	writebuf_struct {
	UB	d[BLOCKSIZE];
	UW	addr;
	W	size;
} writebuf[WRITEBUFSIZE];
static	W	writebufrsize = 0;
static	W	writebufwsize = 0;
static	struct	writebuf_struct	*wp = NULL;

static	W	writing = 0;


static	const	UB	*bin2hex = "0123456789abcdef";


static	void	wait025us(void)
{
	long	l;

	for (l=1; l>0; l--)
		asm("nop");
}


static	void	wait05us(void)
{
	long	l;

	for (l=3; l>0; l--)
		asm("nop");
}


static	void	wait1us(void)
{
	long	l;

	for (l=15; l>0; l--)
		asm("nop");
}


static	void	wait1ms(void)
{
	long	l;

	for (l=15000; l>0; l--)
		asm("nop");
}


static	void	wait200ms(void)
{
	long	l;

	for (l=3000000; l>0; l--)
		asm("nop");
}


static	void	p2cdata(UB c)
{
	p2cbuf[p2cwpos++] = c;
	if (p2cwpos >= BUFFERSIZE)
		p2cwpos = 0;
}


static	void	p2udata(UB c)
{
	p2ubuf[p2uwpos++] = c;
	if (p2uwpos >= BUFFERSIZE)
		p2uwpos = 0;
}


static	void	p2ustr(const UB *s)
{
	UB	c;
	
	while ((c = *(s++)))
		p2udata(c);
}


static	void	recvrs(UW c)
{
	static	W	linewpos = -1;
	static	W	upper = -1;
	static	UB	linebuf[4];
	static	UB	linesum = 0;
	static	UW	addrh = 0;
	static	UW	addr = 0;
	UW	l;
	W	i;
	
	if (c == ':') {
		linewpos = 0;
		upper = -1;
		linesum = 0;
		return;
	}
	if (linewpos < 0) {
		if (linewpos == -1) {
			if (c < 0x20)
				return;
			linewpos = -2;
		}
		p2cdata(c);
		return;
	}
	if ((c >= '0')&&(c <= '9'))
		c -= '0';
	else if ((c >= 'A')&&(c <= 'F'))
		c = c - 'A' + 0xa;
	else if ((c >= 'a')&&(c <= 'f'))
		c = c - 'a' + 0xa;
	else {
		linewpos = -1;
		p2cdata(c);
		return;
	}
	if (upper < 0) {
		upper = c << 4;
		return;
	}
	c |= upper;
	upper = -1;
	linesum += c;
	if (linewpos < 4) {
		linebuf[linewpos++] = c;
		return;
	}
	switch (linebuf[3]) {
		default:
			linewpos = -1;
			return;
		case	0:
			break;
		case	1:
			for (i=0; i<writebufwsize; i++) {
				wp = writebuf + i;
				while (wp->size < BLOCKSIZE)
					wp->d[wp->size++] = 0xff;
			}
			writebufrsize = writebufwsize;
			writebufwsize = 0;
			wp = NULL;
			linewpos = -1;
			return;
		case	4:		/* address-high */
			if (linewpos == 4) {
				addrh = c << 24;
				linewpos++;
			} else if (linewpos == 5) {
				addrh |= (c << 16);
				linewpos = -1;
			}
			writebufrsize = 0;
			return;
	}
	if (linewpos == 4)
		addr = addrh | (((UW)linebuf[1]) << 8) | linebuf[2];
	if (linewpos >= linebuf[0] + 4) {
		if ((linesum))
			recverror |= 4;
		linewpos = -1;
		return;
	}
	
	l = addr & ADDRHMASK;
	if ((wp == NULL)||(wp->addr != l)) {
		wp = NULL;
		for (i=0; i<writebufwsize; i++)
			if (writebuf[i].addr == l) {
				wp = writebuf + i;
				break;
			}
		if (wp == NULL) {
			if (writebufwsize >= WRITEBUFSIZE - 1) {
				recverror |= 0x100;
				linewpos = -1;
				return;
			}
			wp = writebuf + writebufwsize++;
			wp->addr = l;
			wp->size = 0;
		}
	}
	
	i = addr - wp->addr;
	while (wp->size < i)
		wp->d[wp->size++] = 0xff;
	wp->d[wp->size++] = c;
	linewpos++;
	addr++;
}


static	void	idletask(void)
{
	while ((p2cwpos != p2crpos)&&(U1STAbits.UTXBF == 0)) {
		U1TXREG = p2cbuf[p2crpos++];
		if (p2crpos >= BUFFERSIZE)
			p2crpos = 0;
	}
	while ((p2uwpos != p2urpos)&&(U2STAbits.UTXBF == 0)) {
		U2TXREG = p2ubuf[p2urpos++];
		if (p2urpos >= BUFFERSIZE)
			p2urpos = 0;
	}
	if ((U2STAbits.OERR))
		U2STA = 0x1400;
	while ((U2STAbits.URXDA))
		recvrs(U2RXREG);
	
	if ((writing))
		return;
	
	if ((U1STAbits.OERR))
		U1STA = 0x1400;
	while ((U1STAbits.URXDA)) {
		W	c;
		
		c = U1RXREG;
		p2udata(c);
	}
}


static	W	send2wire(UB c, W bits)
{
	W	bitpos;
	
	bitpos = 0;
	for (;;) {
		idletask();
		if (writebufrsize <= 0)
			return -1;
		
		if (PORT_PGC0 == 0)
			continue;
		if (PORT_PGD0 == 0)
			continue;
		
		if (bitpos >= bits)
			return 0;
		
		if ((c & (0x80 >> bitpos))) {
			PORT_PGC0 = 0;
			TRIS_PGC0 = 0;		/* out */
			while ((PORT_PGD0)) {
				idletask();
				if (writebufrsize <= 0)
					return -1;
			}
			TRIS_PGC0 = 1;		/* in */
		} else {
			PORT_PGD0 = 0;
			TRIS_PGD0 = 0;		/* out */
			while ((PORT_PGC0)) {
				idletask();
				if (writebufrsize <= 0)
					return -1;
			}
			TRIS_PGD0 = 1;		/* in */
		}
		bitpos++;
	}
}


void	main(void)
{
	CNPUA = 0xffff;
	CNPUB = 0xffff;
	
	CNPDA = 0;
	CNPDB = 0;
	
	TRISA = 0x0010;		/* -------- ---I--OO */
	TRISB = 0x2384;		/* OOI---II I-OOOIOO */
	
	ANSELA = 0;
	ANSELB = 0;
	
	PORTA = 0xffff;
	PORTB = 0xffff;
	
	SYSKEY = 0;
	SYSKEY = 0xaa996655;
	SYSKEY = 0x556699aa;
	SYSKEY = 0;
	
	SYSKEY = 0;
	SYSKEY = 0xaa996655;
	SYSKEY = 0x556699aa;
	SYSKEY = 0;
	
	U1MODE = 0;
	U1BRG = 86;		/* 115.4kbps */
	U1MODE = 0x8008;	/* enable N81 4(u2brg + 1) */
	U1STA = 0x1400;		/* rx-enable */
	
	U2RXR = 4;		/* RB8 */
	RPB0R = 2;		/* UTX2 */
	
	U2MODE = 0;
	U2BRG = 86;		/* 115.4kbps */
	U2MODE = 0x8008;	/* enable N81 4(u2brg + 1) */
	U2STA = 0x1400;		/* rx-enable */
	
	p2ustr(".\r\n");
	idletask();
	wait200ms();
	
	p2ustr("..\r\n");
	idletask();
	wait200ms();
	
	p2ustr("...\r\n");
	idletask();
	wait200ms();
	
	p2ustr("....\r\n");
	idletask();
	wait200ms();
	
	p2ustr(".....\r\n");
	idletask();
	wait200ms();
	
	for (;;) {
		W	i;
		
		p2ustr("mclr\r\n");
		
		RPA0R = 1;		/* UTX1 */
		U1RXR = 4;		/* RB2 */
		
		TRIS_PGC0 = 1;		/* in */
		TRIS_PGD0 = 1;		/* in */
		
		LAT_MCLR0 = 0;
		
		for (i=0; i<15000; i++)
			idletask();
		
		LAT_MCLR0 = 1;
		
		writing = 0;
		
		p2ustr("run\r\n");
		
		while (writebufrsize <= 0)
			idletask();
		
		p2ustr("writing\r\n");
		
		RPA0R = 0;		/* i/o */
		U1RXR = 0;		/* dummy:RA2 */
		writing = 1;
		
		LAT_MCLR0 = 0;
		
		for (i=0; i<15000; i++)
			idletask();
		
		PORT_PGC0 = 0;
		TRIS_PGC0 = 0;		/* out */
		
		for (i=0; i<15000; i++)
			idletask();
		
		LAT_MCLR0 = 1;
		
		for (i=0; i<15000; i++)
			idletask();
		
		while ((PORT_PGD0)) {
			idletask();
			if (writebufrsize <= 0)
				break;
		}
		break;
		TRIS_PGC0 = 1;		/* in */
		{
			struct	writebuf_struct	*p;
			W	j, k;
			
			send2wire(0x85, 8);	/* magic */
			send2wire(0xa2, 8);
			send2wire(0xdf, 8);
			send2wire(0xa9, 8);
			
			for (j=0; j<writebufrsize; j++) {
				p = writebuf + j;
				k = 0;
				while (k < BLOCKSIZE) {
					UW	addr;
					
					addr = p->addr + k;
					send2wire((addr >> 24) & 0xff, 8);
					send2wire((addr >> 16) & 0xff, 8);
					send2wire((addr >> 8) & 0xff, 8);
					send2wire(addr & 0xff, 8);
					for (i=0; i<128; i++)
						send2wire(p->d[k++], 8);
				}
			}
		}
		send2wire(0, 8);		/* wait write finish */
		writebufrsize = 0;
	}
}


