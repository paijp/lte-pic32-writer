	.file	1 "bootloader.s"
	.section	.mdebug.abi32
	.previous
	.gnu_attribute	4, 3
#	.section	.text,code
	.section	.reset,code,keep
	.align	2
	.set	nomips16
	.set	nomicromips
	.set	noreorder
	.set	nomacro
	.ent	_reset
	.type	_reset, @function
_reset:
	.frame	$sp,0,$31
	.mask	0x00000000,0
	.fmask	0x00000000,0

zero	=	$0
at	=	$1
v0	=	$2
v1	=	$3
a0	=	$4
a1	=	$5
a2	=	$6
a3	=	$7
t0	=	$8
t1	=	$9
t2	=	$10
t3	=	$11
t4	=	$12
t5	=	$13
t6	=	$14
t7	=	$15
s0	=	$16
s1	=	$17
s2	=	$18
s3	=	$19
s4	=	$20
s5	=	$21
s6	=	$22
s7	=	$23
t8	=	$24
t9	=	$25
k0	=	$26
k1	=	$27
gp	=	$28
sp	=	$29
fp	=	$30
ra	=	$31


	b	_startup2
	nop
_startup:
	b	_startup + 0x400
	nop
_startup2:
	lui	t0, 0xbf88
	li	t1, 1000
	li	t2, 0x0400
	li	t3, 0x0800
	or	t4, t2, t3
	
	sw	t4, 0x6158(t0)		# CNPUBSET
.waitclr:
	lw	t8, 0x6120(t0)		# PORTB
	and	t9, t8, t3
	bnez	t9, .exit
	nop
	
	addiu	t1, t1, -1
	bnez	t1, .waitclr
	nop
	
	sw	t4, 0x6134(t0)		# LATBCLR
	
	sw	t2, 0x6114(t0)		# TRISBCLR
.resetwait0:
	lw	t8, 0x6120(t0)		# PORTB
	and	t9, t8, t3
	beqz	t9, .resetwait0
	nop
	
	sw	t2, 0x6118(t0)		# TRISBSET
	
.resetwait1:
	lw	t8, 0x6120(t0)		# PORTB
	and	t9, t8, t2
	beqz	t9, .resetwait1
	nop
	
	jal	recvw
	nop
	lui	t8, 0x85a2		# magic
	ori	t9, t8, 0xdfa9
	bne	t9, v0, .halt
	
	jal	nvmunlock
	li	a0, 0x4000		# nop
	jal	nvmunlock
	li	a0, 0x4000		# nop
	jal	nvmunlock
	li	a0, 0x4005		# erase PFM
	
	lui	s0, 0xbf81
	
	lui	t8, 0x1fc0		# boot flash
	ori	t8, t8, 0x0400
	sw	t8, 0xfffff420(s0)		# NVMADDR
	jal	nvmunlock
	li	a0, 0x4004		# erase page
	
.main:
	jal	recvw
	nop
	move	s1, v0			# addr
	
	lui	s2, 0xa000		# SRAM
	lui	s3, 0xa000
	ori	s3, 0x80
.recvrows:
	jal	recvw
	nop
	sb	v0, 3(s2)
	srl	v0, v0, 8
	sb	v0, 2(s2)
	srl	v0, v0, 8
	sb	v0, 1(s2)
	srl	v0, v0, 8
	sb	v0, 0(s2)
	addiu	s2, s2, 4
	bne	s2, s3, .recvrows
	nop
	
	lui	s2, 0xffff
	ori	t8, s2, 0xff80
	and	s1, s1, t8
	
	ori	t8, s2, 0xfc00
	and	t8, t8, s1
	lui	t9, 0x1fc0
	beq	t8, t9, .writeboot
	nop
	
	lui	t8, 0xfff0
	and	t8, t8, s1
	lui	t9, 0x1d00
	beq	t8, t9, .write
	nop
	
	b	.main
	nop
	
.writeboot:
	addiu	s1, s1, 0x400		# boot -> boot+0x400
	
.write:
	sw	s1, 0xfffff420(s0)		# NVMADDR
	sw	zero, 0xfffff440(s0)	# NVMSRCADDR
	jal	nvmunlock
	li	a0, 0x4003		# write row
	
	b	.main
	nop
	
.halt:
	b	.halt
	nop
	
.exit:
	b	_reset + 0x400
	nop


nvmunlock:
	lui	t0,0xbf81
	sw	a0, 0xfffff400(t0)
	
.waitlvd:
	lw	t8, 0xfffff400(t0)
	andi	t8, t8, 0x800
	bnez	t8, .waitlvd
#	nop
	
	lui	t8, 0xaa99
	ori	t8, t8, 0x6655
	sw	t8, 0xfffff410(t0)
	
	lui	t8, 0x5566
	ori	t8, t8, 0x99aa
	sw	t8, 0xfffff410(t0)
	
	li	t8, 0x8000
	sw	t8, 0xfffff408(t0)
	
.waitwr:
	lw	t8, 0xfffff400(t0)
	andi	t8, t8, 0x8000
	bnez	t8, .waitwr
#	nop
	
	li	t8, 0x4000
	sw	t8, 0xfffff404(t0)
	jr	ra
	nop


recvw:
	lui	t0, 0xbf88
	li	t1, 32
	li	t2, 0x0400
	li	t3, 0x0800
	move	v0, zero
	
.bitloop:
	beqz	t1, .ret
	nop
	addiu	t1, t1, -1
	sll	v0, v0, 1
	
.waitbit:
	lw	t8, 0x6120(t0)
	and	t9, t8, t2
	beqz	t9, .bit0
	nop
	
	and	t9, t8, t3
	beqz	t9, .bit1
	nop
	
	b	.waitbit
	nop
	
.bit0:
	sw	t3, 0x6114(t0)
.bit0wait0:
	lw	t8, 0x6120(t0)
	and	t9, t8, t2
	beqz	t9, .bit0wait0
	nop
	
	sw	t3, 0x6118(t0)
	
.bit0wait1:
	lw	t8, 0x6120(t0)
	and	t9, t8, t3
	beqz	t9, .bit0wait1
	nop
	
	b	.bitloop
	nop
	
.bit1:
	ori	v0, v0, 1
	
	sw	t2, 0x6114(t0)
.bit1wait0:
	lw	t8, 0x6120(t0)
	and	t9, t8, t3
	beqz	t9, .bit1wait0
	nop
	
	sw	t2, 0x6118(t0)
	
.bit1wait1:
	lw	t8, 0x6120(t0)
	and	t9, t8, t2
	beqz	t9, .bit1wait1
	nop
	
	b	.bitloop
	nop
	
.ret:
	jr	ra
	nop


	.align 2
	.end _reset
	.globl _reset
	.size _reset, .-_reset


	.section	.bev_excpt,code,keep,address(0xbfc00380)
	.align	2
	.ent	_bev_exception
	.type	_bev_exception, @function
_bev_exception:
	j	0xbfc00780
	nop


	.align 2
	.end _bev_exception
	.globl _bev_exception
	.size _bev_exception, .-_bev_exception


# Configuration word @ 0xbfc00bfc
	.section	.config_BFC00BFC, code, keep, address(0xBFC00BFC)
	.type	__config_BFC00BFC, @object
	.size	__config_BFC00BFC, 4
__config_BFC00BFC:
	.word	0x7FFFFFF2
# Configuration word @ 0xbfc00bf8
	.section	.config_BFC00BF8, code, keep, address(0xBFC00BF8)
	.type	__config_BFC00BF8, @object
	.size	__config_BFC00BF8, 4
__config_BFC00BF8:
	.word	0xFD6E4D59
# Configuration word @ 0xbfc00bf4
	.section	.config_BFC00BF4, code, keep, address(0xBFC00BF4)
	.type	__config_BFC00BF4, @object
	.size	__config_BFC00BF4, 4
__config_BFC00BF4:
	.word	0xFFF9F8D8
# Configuration word @ 0xbfc00bf0
	.section	.config_BFC00BF0, code, keep, address(0xBFC00BF0)
	.type	__config_BFC00BF0, @object
	.size	__config_BFC00BF0, 4
__config_BFC00BF0:
	.word	0x0FFFFFFF
