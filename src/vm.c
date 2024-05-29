#include <cc.h>
#include <ast.h>
#include <func.h>

const char *opcodes = "LEA  IMM  JMP  JSR  BZ   BNZ  ENT  ADJ  LEV  LI   LC   SI   SC   PSH  OR   XOR  AND  EQ   NE   LT   GT   LE   GE   SHL  SHR  ADD  SUB  MUL  DIV  MOD  OPEN READ CLOS PRTF MALC MSET MCMP EXIT IMD";


void run_virtual_machine(int *pc, int* code, char *data, int argc, char *argv[]) {
    int a, cycle = 0;
    int i, *t;
    int *sp, *bp = NULL;

    sp = (int *)malloc(POOL_SIZE);
    if (!sp) {printf("Unable to malloc sp\n"); exit(-1);}
    memset(sp, 0, POOL_SIZE);

    bp = sp;

    /* Set up the stack */
    sp = (int *)((int)sp + POOL_SIZE);
    *--sp = EXIT;
    *--sp = PSH;
    t = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)t;

    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);

    while (1) {
        i = *pc++;
        ++cycle;
        
        if (0) {
            printf("%d> %.4s", cycle, &opcodes[i * 5]);
            if (i <= ADJ)
                printf(" %d\n", *pc);
            else
                printf("\n");
        }

        switch (i) {
            case LEA:  {
                    /* Load local address based on offset in *pc */
                    a = (int)(bp + *pc++);
                }
                break; /* load local addresss */
            case IMD:  a = (int)(data + *pc++); break; /* load relativ data address */
            case IMM:  a = *pc++; break;             /* load global address or immediate */
            case JMP:  pc = (int *)((int)code + *pc); break;       /* jump */
            case JSR:  {
                    /* "push" return address and jump to offset in *pc */
                    *--sp = (int)(pc + 1);
                    pc = (int *)((int)code + *pc);
                }
                break; /* jump to subroutine */
            case BZ:   pc = a ? pc + 1 : (int *)((int)code + *pc); break; /* branch if zero */
            case BNZ:  pc = a ? (int *)((int)code + *pc) : pc + 1; break; /* branch if not zero */
            case ENT: {
                    /* Setup bp and sp, allocate space for local variables */
                    *--sp = (int)bp;
                    bp = sp;
                    sp = sp - *pc++;
                }
                break; /* enter subroutine */
            case ADJ:  sp = sp + *pc++; break; /* stack adjust */
            case LEV:  {
                    /* Restore bp and sp */
                    sp = bp;
                    bp = (int *)*sp++;
                    pc = (int *)*sp++;                
                }
                break; /* leave subroutine */
            case LI:   a = *(int *)a; break; /* load int */
            case LC:   a = *(char *)a; break; /* load char */
            case SI:   *(int *)*sp++ = a; break; /* store int */
            case SC:   a = *(char *)*sp++ = a; break; /* store char */
            case PSH:  *--sp = a; break; /* push */

            case OR:   a = *sp++ |  a; break;
            case XOR:  a = *sp++ ^  a; break;
            case AND:  a = *sp++ &  a; break;
            case EQ:   a = *sp++ == a; break;
            case NE:   a = *sp++ != a; break;
            case LT:   a = *sp++ <  a; break;
            case GT:   a = *sp++ >  a; break;
            case LE:   a = *sp++ <= a; break;
            case GE:   a = *sp++ >= a; break;
            case SHL:  a = *sp++ << a; break;
            case SHR:  a = *sp++ >> a; break;
            case ADD:  a = *sp++ +  a; break;
            case SUB:  a = *sp++ -  a; break;
            case MUL:  a = *sp++ *  a; break;
            case DIV:  a = *sp++ /  a; break;
            case MOD:  a = *sp++ %  a; break;
            case OPEN: a = open((char *)sp[1], *sp); break;
            case READ: a = read(sp[2], (char *)sp[1], *sp); break;
            case CLOS: a = close(*sp); break;
            case PRTF: {
                    t = sp + pc[1];
                    a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]);
                } 
                break;
            case MALC: a = (int)malloc(*sp); break;
            case MSET: a = (int)memset((char *)sp[2], sp[1], *sp); break;
            case MCMP: a = memcmp((char *)sp[2], (char *)sp[1], *sp); break;
            case EXIT: {
                printf("exit(%d) cycle = %d\n", *sp, cycle);
                goto vm_exit;
            }
            default: printf("cc: unknown instruction = %d! cycle = %d\n", i, cycle); goto vm_exit;
        }
        //printf("a = 0x%x\n", a);
    }
vm_exit:
    ;
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);

    printf("Execution time = %ld ns\n", diffInNanos);
}