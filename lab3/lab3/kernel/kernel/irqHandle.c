#include "x86.h"
#include "device.h"


extern int displayRow;
extern int displayCol;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;
extern TSS tss;

void GProtectFaultHandle(struct StackFrame *sf);
void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);
void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);


void irqHandle(struct StackFrame *sf) { // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpstackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch(sf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(sf);
			break;
		case 0x20:
			timerHandle(sf);
			break;
		case 0x80:
			syscallHandle(sf);
			break;
		default:assert(0);
	}
	pcb[current].stackTop = tmpstackTop;
}

void timerHandle(struct StackFrame *sf) {
	uint32_t tmpStackTop;
	for (int i = 1; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_BLOCKED) {
			if (pcb[i].sleepTime > 0) 
				pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0)
				pcb[i].state = STATE_RUNNABLE;
		}
	}
	pcb[current].timeCount++;
	int pid;
	if (pcb[current].timeCount >= MAX_TIME_COUNT) {
		pid = current + 1;
		if (pid >= MAX_PCB_NUM)
			pid -= MAX_PCB_NUM;
		while (pid != current) {
			if (pcb[pid].state == STATE_RUNNABLE || pcb[pid].state == STATE_RUNNING)
				break;
			pid += 1;
			if (pid >= MAX_PCB_NUM)
				pid -= MAX_PCB_NUM;
		}
		if (pid == current) {
			if (pcb[current].state == STATE_RUNNABLE || pcb[current].state == STATE_RUNNING)
				pcb[current].timeCount = 0;
			else
				current = 0;
		}
		else 
			current = pid;
		pcb[current].state = STATE_RUNNING;
	}
	tmpStackTop = pcb[current].stackTop;
	pcb[current].stackTop = pcb[current].prevStackTop;
	tss.esp0 = (uint32_t)&(pcb[current].stackTop);
	asm volatile("movl %0, %%esp" ::"m"(tmpStackTop));
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");
	return;
	

}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}


void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case 0:
			syscallWrite(sf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(sf);
			break;
		case 3:
			syscallSleep(sf);
			break;
		case 4:
			syscallExit(sf);
			break;
		default:break;
	}
}

void syscallSleep(struct StackFrame *sf) {
	uint32_t time = (uint32_t)sf->ecx;
	pcb[current].state = STATE_BLOCKED;
	if (time  < 0)
		return;
	pcb[current].sleepTime = time;
	pcb[current].timeCount = MAX_TIME_COUNT;
	asm volatile("int $0x20");
	return;
}

void syscallFork(struct StackFrame *sf) {
	int i;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD)
			break;
	}
	if (i != MAX_PCB_NUM) {
		enableInterrupt();
		for (int j = 0; j < 0x100000; j++)
			*(uint8_t *)(j + (i+1)*0x100000) = *(uint8_t *)(j + (current+1)*0x100000);
		disableInterrupt();
		pcb[i].pid = i;
		pcb[i].timeCount = 0;
		pcb[i].sleepTime = 0;
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop);
		pcb[i].stackTop = (uint32_t)&(pcb[i].regs);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].regs.cs = USEL(2 * i + 1);
		pcb[i].regs.ds = USEL(2 * i + 2);
		pcb[i].regs.es = USEL(2 * i + 2);
		pcb[i].regs.fs = USEL(2 * i + 2);
		pcb[i].regs.ss = USEL(2 * i + 2);
		pcb[i].regs.gs = USEL(2 * i + 2);
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else
		pcb[current].regs.eax = -1;
	return;
}

void syscallExit(struct StackFrame *sf) {
	pcb[current].state = STATE_DEAD;
	pcb[current].timeCount = MAX_TIME_COUNT;
	asm volatile("int $0x20");
	return;
}

void syscallWrite(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case 0:
			syscallPrint(sf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct StackFrame *sf) {
	int sel = sf->ds; //TODO segment selector for user data, need further modification
	char *str = (char*)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		if(character == '\n') {
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				displayRow=24;
				displayCol=0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==80){
				displayRow++;
				displayCol=0;
				if(displayRow==25){
					displayRow=24;
					displayCol=0;
					scrollScreen();
				}
			}
		}
		//asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		//asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
	return;
}

