#include <stdio.h>
#include <pthread.h>
#include <curses.h>


/*
 *
 *  Memory map definition
 *
 */

#define R0 0x0
#define R1 0x2
#define R2 0x4
#define R4 0x6
// Stack pointer register 
#define SP 0x8
// Stack base register 
#define SB 0xa 
// Flag register 
#define FL 0xc 
// Instruction pointer register
#define IP 0xe 
// Interrupt vector table
#define INT0 0x10
#define INT1 0x12 
#define INT2 0x14 
#define INT3 0x16

// Keyboard base address
#define KB 0x18

// Console buffer base address
#define VB 0x820

// Flags 
#define F_INT   0x8000
#define F_CARRY 0x4000

/*************************************/


// Macros 
#define V_CHAR(vm, x) (*((char *)(&vm->mem[(x)])))
#define V_SHORT(vm, x) (*((short *)(&vm->mem[(x)])))
#define V_PTR(vm, x) ((&vm->mem[(x)]))
#define LOCK(vm) (pthread_mutex_lock(&(vm->lock)))
#define UNLOCK(vm) (pthread_mutex_unlock(&(vm->lock)))

// Display mutex
pthread_mutex_t disp_lock = PTHREAD_MUTEX_INITIALIZER;

// Structs
typedef struct {
    void *mem;
    pthread_mutex_t lock;
    int errorCode;
} ndVM;

// Create VM
ndVM *create_vm(){
    ndVM *vm = malloc(sizeof(ndVM));
    vm->mem = malloc(0x10000);
    pthread_mutex_init(&(vm->lock), NULL);
    vm->errorCode = 0;
}

void delete_vm(ndVM *vm){
    free(vm->mem);
    free(vm);
}

// Run VM
int run_vm(ndVM *vm) {

    // Fetch and decode
    while(1){
        LOCK(vm);
        short ip = V_SHORT(vm, IP);
        // Fetch an instruction.
        char instr = V_CHAR(vm, ip);
        short op1, op2, op3;

        ip++;
        switch(instr){
            case '?':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                if(op1 == op2)
                    V_SHORT(vm, IP) = op3;
                else 
                    V_SHORT(vm, IP) = ip;
                break;
            case '=':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = op2; 
                break;
            case 'l':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2); 
                break;
            case '+':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) + V_SHORT(vm, op3);
                break;
            case '-':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) - V_SHORT(vm, op3);
                break;
            case '*':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) * V_SHORT(vm, op3);
                break;
            case '/':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) / V_SHORT(vm, op3);
                break;
            case 'm':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) % V_SHORT(vm, op3);
                break;
            case 'o':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) | V_SHORT(vm, op3);
                break;
            case 'a':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) & V_SHORT(vm, op3);
                break;
            case 'x':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                op3 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = V_SHORT(vm, op2) ^ V_SHORT(vm, op3);
                break;
            case 'n':
                op1 = V_SHORT(vm, ip); ip+=2;
                op2 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, op1) = !V_SHORT(vm, op2);
                break;
            case 'i':
                ip+=1;
                interrupt(vm, V_CHAR(vm, ip-1));
                break;
            case 'p':
                op1 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, V_SHORT(vm, SB) - V_SHORT(vm, SP)) = op1;
                V_SHORT(vm, SP)+=2;
                break;
            case 'P':
                op1 = V_SHORT(vm, ip); ip+=2;
                V_SHORT(vm, IP) = ip;
                V_SHORT(vm, SP)-=2;
                V_SHORT(vm, op1) = V_SHORT(vm, V_SHORT(vm, SB) - V_SHORT(vm, SP));
                break;
            case 'h':
                V_SHORT(vm, IP) = ip;
                usleep(10);
                break;
            case 'H':
                V_SHORT(vm, IP) = ip;
                return 0;
                break;
            default:
                return -1;
        }
        UNLOCK(vm);
    }

}

void interrupt(ndVM* vm, char n){
    short addr;
    switch(n){
        case 0:
            addr = V_SHORT(vm, INT0); 
            break;
        case 1:
            addr = V_SHORT(vm, INT1);
            break;
        case 2:
            addr = V_SHORT(vm, INT2);
            break;
        case 3:
            addr = V_SHORT(vm, INT3);
            break;
    }
    

    V_SHORT(vm, V_SHORT(vm, SB) - V_SHORT(vm, SP)) = V_SHORT(vm, IP);
    V_SHORT(vm, SP)+=2;
    V_SHORT(vm, IP) = addr;
}

// VM thread 
void vm_thread(ndVM *vm){
    // Initialize and load rom bios.
    FILE *fp = fopen("./rom.bin", "r");
    int i = 0;
    fread(vm->mem, 1, 0xffff, fp);
    fclose(fp);
   
    vm->errorCode = run_vm(vm);
}



// Console thread
void console_thread(ndVM *vm){
    int i, j;
    char c=0;
    while(1){
        pthread_mutex_lock(&disp_lock);
        for(i = 0;i < 25; i++){ 
            for(j = 0; j < 80; j++){
                char c = V_CHAR(vm, VB + ((i*80)+j));
                move(i,j);
                if(c!=0)  addch(c);
            }
        }
        usleep(100);
        refresh();
        pthread_mutex_unlock(&disp_lock);
    }

}

// Keyboard thread
void kbd_thread(ndVM *vm){
    char v;
    while(1){
        if ((V_SHORT(vm, FL) & F_INT != 0)){
            pthread_mutex_lock(&disp_lock);
            if( (v = getch()) != -1){
                V_CHAR(vm, KB) = v;
                LOCK(vm);
                interrupt(vm, 1);
                UNLOCK(vm);
                usleep(100);
            } 
            pthread_mutex_unlock(&disp_lock);
        }
    }
}

// Timer thread
void timer_thread(ndVM *vm){
    int i = 0;
    while(1){
        if ((V_SHORT(vm, FL) & F_INT != 0)){
            LOCK(vm);
            interrupt(vm, 0);
            UNLOCK(vm);
            usleep(50);
        }
    }
}


// Entry point.
int main(int argc, char **argv, char **env){
    ndVM *vm;
    pthread_t th[4];
    WINDOW* win;

    if( (win = initscr()) == NULL ){
        fprintf(stderr, "Error initializing ncurses.\n");
        exit(1);
    }

    // Non-blocking getch
    timeout(0);

    vm = create_vm();

    pthread_create(&th[0], NULL, vm_thread, (void *)vm);
    pthread_create(&th[1], NULL, console_thread, (void *)vm);
    pthread_create(&th[2], NULL, timer_thread, (void *)vm);
    pthread_create(&th[3], NULL, kbd_thread, (void *)vm);

    pthread_join(th[0], NULL);

    delwin(win);
    endwin();
    refresh();

    return 0;
}
