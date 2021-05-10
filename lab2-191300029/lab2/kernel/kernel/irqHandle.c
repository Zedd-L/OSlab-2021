#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;


void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallGetChar(struct TrapFrame *tf);
void syscallGetStr(struct TrapFrame *tf);


void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%es"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%fs"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%gs"::"a"(KSEL(SEG_KDATA)));
	switch(tf->irq) {
		// TODO: 填好中断处理程序的调用
		case 0x21:
			KeyboardHandle(tf);
			break;
		case 0x80:	
			syscallHandle(tf);
			break;
		default:break;
	}
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void KeyboardHandle(struct TrapFrame *tf){
	uint32_t code = getKeyCode();
	if(code == 0xe){ // 退格符
		// TODO: 要求只能退格用户键盘输入的字符串，且最多退到当行行首
		int pos = 0;
		char key = getChar(0);
		uint16_t data = 0;
		data = key | (0x0c << 8);
		pos = (80 * displayRow + (displayCol - 1)) * 2;
		if(displayCol == 0){
			return;	
		}
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
		displayCol--;
	}
	else if(code == 0x1c){ // 回车符
		int pos = 0;
		displayRow++;
		displayCol = 0;
		pos = (80 * displayRow + displayCol) * 2;
		char key = getChar(0);
		uint16_t data = 0;
		data = key | (0x0c << 8);
		pos = (80 * displayRow + displayCol) * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
		if(displayRow == 25) {
			displayRow = 24;
			displayCol = 0;
			scrollScreen();
		}		
	}
	else if(code < 0x81) { // 正常字符
		// TODO: 注意输入的大小写的实现、不可打印字符的处理
		if (code == 0x01 || code == 0x0f || code == 0x45 || code == 0x46 || 
			code == 0x1d || code == 0x35 || code == 0x38 || code == 0x3a ||
			(code >= 0x3b&&code <= 0x44) || code == 0x57 || code == 0x58 || 
			code == 0x0) 
			return;
		char key = getChar(code);
		int pos = 0;
		uint16_t data = 0;
		data = key | (0x0c << 8);
		pos = (80 * displayRow + displayCol) * 2;
		if (key != '\0') {
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if(displayCol == 80){
				displayRow++;
				displayCol = 0;
					if(displayRow == 25) {
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}		
			}
		}
	}
	updateCursor(displayRow, displayCol);
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallRead(tf);
			break; // for SYS_READ
		default:break;
	}
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
	int sel = USEL(SEG_UDATA); //TODO: segment selector for user data, need further modification
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		// TODO: 完成光标的维护和打印到显存
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
				if(displayRow==25) {
					displayRow=24;
					displayCol=0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
}

void syscallRead(struct TrapFrame *tf){
	switch(tf->ecx){ //file descriptor
		case 0:
			syscallGetChar(tf);
			break; // for STD_IN
		case 1:
			syscallGetStr(tf);
			break; // for STD_STR
		default:break;
	}
}

/*edx ebx esi edi represent 3rd, 4th, 5th, 6th argument*/

void syscallGetChar(struct TrapFrame *tf){
	// TODO: 自由实现
	int len = tf->edx;
	int code = getKeyCode();
	while (code == 0) {
		code = getKeyCode();
	}
	char key = getChar(code);
	// displayCol--;
	if (code == 0xe) {
		char key = getChar(0);
		uint16_t data = key | (0x0c << 8);
		int pos = (80 * displayRow + (displayCol - 1)) * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
		displayCol--;
		tf->eax = '?';
		updateCursor(displayRow, displayCol);
		return;
	}
	if (code == 0xe + 0x80) {
		tf->eax = 0;
		return;
	}
	if (len == 0 && key != '\0' && key != '\n') {
		int pos = 0;
		uint16_t data = key | (0x0c << 8);
		pos = (80*displayRow+displayCol)*2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
		displayCol++;
		updateCursor(displayRow, displayCol);
	}
	tf->eax = key;
	return;
}

void syscallGetStr(struct TrapFrame *tf){
	// TODO: 自由实现
	int code = getKeyCode();
	while (code == 0) {
		code = getKeyCode();
	}
	char key = 0;
	int pos = 0;
	uint16_t data = 0;
	if (code == 0x1c) {
		key = getChar(code);
	}
	else if (code == 0xe) {
		// int now_pos = displayCol;
		char key = getChar(0);
		data = key | (0x0c << 8);
		pos = (80 * displayRow + (displayCol - 1)) * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
		displayCol--;
		tf->eax = '?';
		updateCursor(displayRow, displayCol);
		return;
	}
	else if (
		(code >= 0x02 && code <= 0x0d) || (code >= 0x10 && code <= 0x1b) ||
		(code >= 0x1e && code <= 0x29) || (code >= 0x2b && code <= 0x35) ||
		(code >= 0x47 && code <= 0x53))
		{
		key = getChar(code);
		data = key | (0x0c << 8);
		pos = (80 * displayRow + displayCol) * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
		displayCol++;
		if(displayCol == 80){
			displayRow++;
			displayCol = 0;
			if(displayRow == 25) {
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}		
		}
	}
	updateCursor(displayRow, displayCol);
	tf->eax = key;
	return;
}
