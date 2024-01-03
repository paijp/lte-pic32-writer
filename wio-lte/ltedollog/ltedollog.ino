
#ifndef	UARTBAUD
#define	UARTBAUD	115200
#endif

#ifndef	BUFFERSIZE
#define	BUFFERSIZE	65536
#endif

#ifndef	SENDINTERVAL
#define	SENDINTERVAL	15000		/* 15s */
#endif

#ifndef	SIMAPN
#define	SIMAPN	"soracom.io"
#endif

#ifndef	SIMUSER
#define	SIMUSER	"sora"
#endif

#ifndef	SIMPASS
#define	SIMPASS	"sora"
#endif

#ifndef	SENDDOMAIN
#define	SENDDOMAIN	"beam.soracom.io"
#endif

#ifndef	SENDPORT
#define	SENDPORT	23080
#endif

#ifndef	DOWNLOADURL
#define	DOWNLOADURL	"http://beam.soracom.io/yourpath/?p="
#endif


/*jp.pa-i/html
<p><a href="https://github.com/paijp/diagram-in-comment">How to generate this?</a></p>
*/
/* generated HTML: https://paijp.github.io/lte-pic32-writer/ltedollog.html */

/*jp.pa-i.cir/map32mx2-28
PGD	RPB10	PGED2	UTX2
PGC	PGEC2
*/

/*jp.pa-i/html
<hr />
*/

/*jp.pa-i.cir/cir dip28 PIC32MX270F256B
22(442{r10k}2[[3V3])2[MCLR]2{c104}2[[G]
2
2
2
2
2
2
22[[G]
2
2
2
2
22[[3V3]2{c106}2[[G]
2
8
8
8
8
888[[G]
88{c106}8[[G]
88[PGD]
88[PGC]
88[[3V3]
8
8
8
88[[G]
88[[3V3]8{c104}8[[G]
*/

/*jp.pa-i.cir/cir sip5 ICSP
22[MCLR]
22[[3V3]
22[[G]
22[PGD]
22[PGC]
*/

/*jp.pa-i.cir/cir sip4 WIO_LTE_UART
22[[LTE_G]2[G]
22[[LTE_3V3]
22[LTE_TX]2[MCLR]
22[LTE_RX]2[PGD]
*/

/*jp.pa-i.cir/cir sip4 WIO_LTE_D20
22[[LTE_G]2[G]
22[[LTE_3V3]
22[LTE_D19]2[PGD]
22[LTE_D20]2[PGC]
*/


#include	<WioLTEforArduino.h>

WioCellular	Wio;

typedef	int	W;
typedef	short	H;
typedef	char	B;
typedef	unsigned int	UW;
typedef	unsigned short	UH;
typedef	unsigned char	UB;


#define	CK0	D20
#define	DT0	D19

static	int	socket0 = -1;

#define	BLOCKSIZE	0x400
#define	ADDRHMASK	0xfffffc00
#define	WRITEBUFSIZE	65
struct	writebuf_struct {
	UB	d[BLOCKSIZE];
	UW	addr;
};
static	union {
	struct	writebuf_struct	writebuf[WRITEBUFSIZE];
	byte	logbuf[BUFFERSIZE];
} d;

static	int	logbuf_rpos = 0;
static	int	logbuf_wpos = 0;

static	W	exitflag = 0;


static	void	idletask()
{
	int	c, wpos;
	
	if (!Serial.available())
		return;
	
	c = Serial.read();
	wpos = logbuf_wpos + 1;
	if (wpos >= BUFFERSIZE)
		wpos = 0;
	if (wpos == logbuf_rpos)
		return;	/* overflow */
	d.logbuf[logbuf_wpos] = c;
	logbuf_wpos = wpos;
}


static	void	idleproc()
{
}


static	W	send2wire(UB c, W bits)
{
	W	bitpos;
	
	bitpos = 0;
	for (;;) {
		if ((exitflag))
			return -1;
		idleproc();
		
		if (digitalRead(CK0) == LOW)
			continue;
		if (digitalRead(DT0) == LOW)
			continue;
		
		if (bitpos >= bits)
			return 0;
		
		if ((c & (0x80 >> bitpos))) {
			pinMode(CK0, OUTPUT);
			digitalWrite(CK0, LOW);
			while (digitalRead(DT0) == HIGH) {
				if ((exitflag))
					return -1;
				idleproc();
			}
			pinMode(CK0, INPUT_PULLUP);
		} else {
			pinMode(DT0, OUTPUT);
			digitalWrite(DT0, LOW);
			while (digitalRead(CK0) == HIGH) {
				if ((exitflag))
					return -1;
				idleproc();
			}
			pinMode(DT0, INPUT_PULLUP);
		}
		bitpos++;
	}
}


void	setup()
{
	delay(200);
	Wio.Init();
	Serial.begin(UARTBAUD);
	
	Wio.PowerSupplyGrove(true);
	Wio.PowerSupplyLTE(true);
	
	pinMode(CK0, INPUT_PULLUP);
	pinMode(DT0, INPUT_PULLUP);
	
	delay(500);
	Wio.LedSetRGB(10, 10, 10);
	delay(500);
	Wio.LedSetRGB(10, 0, 0);
#if 0
	{
		static	W	l = 10;
		
		Wio.LedSetRGB(10, 0, *((char*)(&l)));
		for (;;)
			delay(2000);
		
	}
#endif
	if (!Wio.TurnOnOrReset()) {
		Wio.LedSetRGB(0, 0, 10);
		return;
	}
	Wio.LedSetRGB(10, 10, 0);
	if (!Wio.Activate(SIMAPN, SIMUSER, SIMPASS))
		return;
	Wio.LedSetRGB(0, 10, 0);
	for (;;) {
		static	char	url[256] = DOWNLOADURL;
		static	int	page = 0;
		int	len, blocks;
		W	i, j, k;
		
		{
			char	c, *p;
			
			p = url;
			while ((c = *(p++)))
				if (c == '=') {
					if (page >= 100)
						*(p++) = '0' + ((page / 100) % 10);
					if (page >= 10)
						*(p++) = '0' + ((page / 10) % 10);
					*(p++) = '0' + (page % 10);
					*p = 0;
					break;
				}
		}
		if ((len = Wio.HttpGet(url, (char*)d.writebuf, sizeof(d.writebuf))) < 1024) {
			if (page > 0)
				break;
			delay(2000);
			continue;
		}
		blocks = len / sizeof(struct writebuf_struct);
		if (page == 0) {
			Wio.LedSetRGB(0, 10, 10);
		
			pinMode(CK0, OUTPUT);
			digitalWrite(CK0, LOW);
		
			Serial.write((char *){0}, 1);
			delay(100);
		
			while (digitalRead(DT0) == HIGH) {
				if ((exitflag))
					break;
				idleproc();
			}
			pinMode(CK0, INPUT_PULLUP);
		
			Wio.LedSetRGB(10, 0, 10);
		
			send2wire(0x85, 8);		/* magic */
			send2wire(0xa2, 8);
			send2wire(0xdf, 8);
			send2wire(0xa9, 8);
		
#if 0
			Wio.LedSetRGB(0, 0, 0);
#endif
		}
		for (j=0; j<blocks; j++) {
			struct	writebuf_struct	*p;
			
			p = d.writebuf + j;
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
		Wio.LedSetRGB(0, 10, 0);
		page++;
	}
	send2wire(0, 8);		/* wait write finish */
	
	Serial.begin(UARTBAUD);
	Serial.write((char *){0}, 1);
	while (Serial.available())
		Serial.read();
	Wio.SetDoWorkInWaitForAvailableFunction(idletask);
	logbuf_rpos = logbuf_wpos;
	
	Wio.LedSetRGB(10, 10, 10);
	
	for (;;) {
		if ((socket0 = Wio.SocketOpen(SENDDOMAIN, SENDPORT, Wio.SOCKET_UDP)) >= 0)
			break;
		delay(2000);
	}
}


void	loop()
{
	static	byte	sendbuf[1460];
	int	sendbuf_wpos, sendbuf_lfpos;
	int	lfpos;
	
	idleproc();
	idletask();
	if (logbuf_rpos == logbuf_wpos)
		return;
	
	{
		static	int	remainms = SENDINTERVAL;
		static	int	lastms = -1;
		int	ms;
		
		if (lastms == (ms = millis()))
			return;
		lastms = ms;
		if (--remainms > 0)
			return;
		remainms = SENDINTERVAL;
	}
	
	sendbuf_wpos = sendbuf_lfpos = 0;
	lfpos = 0;
	while (sendbuf_wpos < sizeof(sendbuf)) {
		int	c;
		
		if (logbuf_rpos == logbuf_wpos)
			break;
		sendbuf[sendbuf_wpos++] = c = d.logbuf[logbuf_rpos++];
		if (logbuf_rpos >= BUFFERSIZE)
			logbuf_rpos = 0;
		if (c == 0xa) {
			sendbuf_lfpos = sendbuf_wpos;
			lfpos = logbuf_rpos;
		}
	}
	if (sendbuf_lfpos > 0) {
		sendbuf_wpos = sendbuf_lfpos;
		logbuf_rpos = lfpos;
	}
	Wio.LedSetRGB(10, 10, 10);
	Wio.SocketSend(socket0, sendbuf, sendbuf_wpos);
	Wio.LedSetRGB(0, 10, 0);
}

