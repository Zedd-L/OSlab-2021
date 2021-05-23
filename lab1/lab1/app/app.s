.code32

.global start
start:
	pushl $13
	pushl $message
	calll displayStr
loop:
	jmp loop

message:
	.string "Hello, World!\n\0"

displayStr:
	movl 4(%esp), %ebx
	movl 8(%esp), %ecx
	movl $((80*5+0)*2), %edi
	movb $0x0d, %ah #这里进行过修改 0x0c -> 0x0d，尝试对字体颜色的影响，高位是背景颜色，低位是字体颜色
nextChar:
	movb (%ebx), %al
	movw %ax, %gs:(%edi)
	addl $2, %edi
	incl %ebx
	loopnz nextChar # loopnz decrease ecx by 1
	ret
	