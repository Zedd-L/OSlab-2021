#include "lib.h"
#include "types.h"

sem_t forks[5];

void getforks(int i);
void putdownforks(int i);

int uEntry(void) {
	/*For lab4.1, Test 'scanf'*/ 
	// int dec = 0;
	// int hex = 0;
	// char str[6];
	// char cha = 0;
	// int ret = 0;
	// while(1){
	// 	printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
	// 	ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
	// 	printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
	// 	if (ret == 4)
	// 		break;
	// }
	
	/*For lab4.2, Test 'Semaphore'*/
	int i = 4;
	sem_t sem;
	int ret = 0;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 2);
	if (ret == -1) {
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0) {
		while( i != 0) {
			i --;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1) {
		while( i != 0) {
			i --;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}

	/* For lab4.3, 
	TODO: You need to design and test the philosopher problem.
	Note that you can create your own functions.
	Requirements are demonstrated in the guide.*/
	int ret = 1;
	for (int i = 0; i < 5; i++)
		sem_init(forks + i, 1);
	for (int i = 0; i < 5; i++) {
		if (ret > 0) 
			ret = fork();
	}
	int start = getpid() - 2;
	if (start >= 0) {
		while (1) {
			printf("Philosopher %d: think\n", start + 1);
			sleep(128);
			getforks(start);
			printf("Philosopher %d: eat\n", start + 1);
			sleep(128);
			putdownforks(start);
		}
	}
	while (1);
	for (int i = 0; i < 5; i++) {
		sem_destroy(forks + i);
	}
	exit();
	return 0;
}

void getforks(int i) {
	if (i % 2 == 0) {
		sem_wait(forks + i);
		sem_wait(forks + ((i + 1) % 5));
	}
	else {
		sem_wait(forks + ((i + 1) % 5));
		sem_wait(forks + i);
	}
}

void putdownforks(int i) {
	sem_post(forks + i);
	sem_post(forks + ((i + 1) % 5));
}