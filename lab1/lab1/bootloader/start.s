/* Real Mode Hello World */
/*
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0x7d00, %ax
	movw %ax, %sp # setting stack pointer to 0x7d00
	pushw $13
	pushw $message
	callw displayStr

loop:
	jmp loop

message:
	.string "Hello, World!\n\0"

displayStr:
	pushw %bp
    movw 4(%esp), %ax
    movw %ax, %bp
    movw 6(%esp), %cx
    movw $0x1301, %ax
    movw $0x000c, %bx # %bx 控制颜色
	movw $0x50, %dx # 修改， $0x0000 -> $0x50 ($80), 进行了一次换行， 和在Hello, World前面加\n效果相同
    int $0x10
 	popw %bp
    ret
*/


/* Protected Mode Hello World */
/*
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	cli
	pushw %ax
	inb $0x92, %al
	orb $0x2, %al
	outb %al, $0x92
	popw %ax
	data32 addr32 lgdt gdtDesc
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	data32 ljmp $0x08, $start32

.code32
start32:
	movw $0x10, %ax # setting data segment selector
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw $0x18, %ax
	movw %ax, %gs
	movl $0x8000, %eax
	movl %eax, %esp
	pushl $13
	pushl $message
	calll displayStr
loop32:
	jmp loop32

message:
	.string "Hello, World!\n\0"

displayStr:
	movl 4(%esp), %ebx
	movl 8(%esp), %ecx
	movl $((80*5+0)*2), %edi
	movb $0x0a, %ah #这里进行过修改 0x0c -> 0x0a，尝试对字体颜色的影响，高位是背景颜色，低位是字体颜色
nextChar:
	movb (%ebx), %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	incl %ebx
	loopnz nextChar # loopnz decrease ecx by 1
	ret

.p2align 2
gdt:
	.word 0, 0
	.byte 0, 0, 0, 0

	.word 0xffff, 0 #代码段描述符
	.byte 0, 0x9a, 0xcf, 0

	.word 0xffff, 0 #数据段描述符
	.byte 0, 0x92, 0xcf, 0

	.word 0xffff, 0x8000 #视频段描述符
	.byte 0x0b, 0x92, 0xcf, 0
gdtDesc:
	.word (gdtDesc - gdt -1)
	.long gdt
*/


/* Protected Mode Loading Hello World APP */
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	#TODO: Protected Mode Here
	cli
	pushw %ax
	inb $0x92, %al
	orb $0x2, %al
	outb %al, $0x92
	popw %ax
	data32 addr32 lgdt gdtDesc
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	data32 ljmp $0x08, $start32

.code32
start32:
	movw $0x10, %ax # setting data segment selector
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw $0x18, %ax # setting graphics data segment selector
	movw %ax, %gs

	movl $0x8000, %eax # setting esp
	movl %eax, %esp
	jmp bootMain # jump to bootMain in boot.c

.p2align 2
gdt: 
	#GDT definition here
	.word 0, 0
	.byte 0, 0, 0, 0
	
	.word 0xffff, 0 #代码段描述符
	.byte 0, 0x9a, 0xcf, 0

	.word 0xffff, 0 #数据段描述符
	.byte 0, 0x92, 0xcf, 0

	.word 0xffff, 0x8000 #视频段描述符
	.byte 0x0b, 0x92, 0xcf, 0

gdtDesc: 
	#gdtDesc definition here
	.word (gdtDesc - gdt -1)
	.long gdt
