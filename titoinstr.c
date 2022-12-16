#include "titoinstr.h"
#include "titostate.h"
#include <stdio.h>

typedef struct
{
    int32_t opcode : 8;
    int32_t rj : 3;
    int32_t mode : 2;
    int32_t ri : 3;
    int32_t addr : 16;
    int32_t sec_operand_value; // Second operand, ri+addr or memory value, depending on mode.
} TTK91Instruction;

TTK91Instruction ins;

void load_instruction(int32_t input_instr)
{
    ins.opcode = input_instr >> 24;
    ins.rj = (input_instr >> 21) & 0x7;
    ins.mode = (input_instr >> 19) & 0x3;
    ins.ri = (input_instr >> 16) & 0x7;
    ins.addr = input_instr & 0xffff;

    ins.sec_operand_value = ins.addr;

    if (ins.ri != 0)
        ins.sec_operand_value += mach_cpu[ins.ri];

    if (ins.mode == 0)
        ins.sec_operand_value = ins.sec_operand_value;

    else if (ins.mode == 1)
        ins.sec_operand_value = mach_readmem(ins.sec_operand_value);

    else if (ins.mode == 2)
        ins.sec_operand_value = mach_readmem(mach_readmem(ins.sec_operand_value));
}

void input(int32_t n)
{
    if (!waiting_for_input)
        return;
    mach_cpu[ins.rj] = n;
    waiting_for_input = 0;
    mach_cu[PC]++;
}

// ----- Nop
void instr_nop() {}

// ----- Data transfer instructions
void instr_load()
{
    mach_cpu[ins.rj] = ins.sec_operand_value;
}
void instr_store()
{
    // Addressing mode does something funny with store instruction.
    mach_writemem(ins.sec_operand_value, mach_cpu[ins.rj]);
}
void instr_in()
{
    waiting_for_input = 1;
    // printf("Please input a number: ");
    //
    // int result;
    // scanf("%d", &result);
    // mach_cpu[ins.rj] = result;
}
void instr_out()
{
    int value = mach_cpu[ins.rj];
    // printf("OUT: %d\n", value);

    for (int i = OUTPUT_BUFFER_SIZE - 1; i > 0; i--)
        output_buffer[i] = output_buffer[i - 1];

    output_buffer[0] = value;
    output_buffer_len++;
    
    if (output_buffer_len > OUTPUT_BUFFER_SIZE)
        output_buffer_len = OUTPUT_BUFFER_SIZE;
}

// ----- Artithmetic instructions
void instr_add()
{
    mach_cpu[ins.rj] += ins.sec_operand_value;
}
void instr_sub()
{
    mach_cpu[ins.rj] -= ins.sec_operand_value;
}
void instr_mul()
{
    mach_cpu[ins.rj] *= ins.sec_operand_value;
}
void instr_div()
{
    mach_cpu[ins.rj] /= ins.sec_operand_value;
}
void instr_mod()
{
    mach_cpu[ins.rj] %= ins.sec_operand_value;
}

// ----- Logic instructions
void instr_comp()
{
    // Clear GEL bits
    mach_cu[SR] &= ~(0b111 << 29);

    // Greater
    if (mach_cpu[ins.rj] > ins.sec_operand_value)
        mach_cu[SR] |= 1 << 31;

    // Equal
    else if (mach_cpu[ins.rj] == ins.sec_operand_value)
        mach_cu[SR] |= 1 << 30;

    // Less
    else
        mach_cu[SR] |= 1 << 29;
}

// ----- Branching instructions
void instr_jump()
{
    // -1 because pc is incremented right after.
    mach_cu[PC] = ins.sec_operand_value - 1;
}

void instr_jneg()
{
    if (mach_cpu[ins.rj] < 0)
    {
        mach_cu[PC] = ins.sec_operand_value - 1;
    }
}
void instr_jzer()
{
    if (mach_cpu[ins.rj] == 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jpos()
{
    if (mach_cpu[ins.rj] > 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jnneg()
{
    if (mach_cpu[ins.rj] >= 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jnzer()
{
    if (mach_cpu[ins.rj] != 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jnpos()
{
    if (mach_cpu[ins.rj] <= 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}

void instr_jles()
{
    if (((mach_cu[SR] >> 29) & 1) == 1)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jequ()
{
    if (((mach_cu[SR] >> 30) & 1) == 1)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jgre()
{
    if (((mach_cu[SR] >> 31) & 1) == 1)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jnles()
{
    if (((mach_cu[SR] >> 29) & 1) != 1)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jnequ()
{
    if (((mach_cu[SR] >> 30) & 1) == 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}
void instr_jngre()
{
    if (((mach_cu[SR] >> 31) & 1) == 0)
        mach_cu[PC] = ins.sec_operand_value - 1;
}

// ----- Stack instructions
void instr_push()
{
    mach_cpu[SP]++;
    mach_writemem(mach_cpu[SP], ins.sec_operand_value);
}
void instr_pop()
{
    mach_cpu[ins.ri] = mach_readmem(mach_cpu[SP]);
    mach_cpu[SP]--;
}
void instr_pushr()
{
    for (int i = 0; i < 7; i++)
    {
        mach_cpu[SP]++;
        mach_writemem(mach_cpu[SP], mach_cpu[i]);
    }
}
void instr_popr()
{
    int temp_sp = mach_cpu[SP];
    for (int i = 6; i >= 0; i--)
    {
        mach_cpu[i] = mach_readmem(temp_sp);
        temp_sp--;
    }
    mach_cpu[SP] -= 6;
}

// ----- Subroutine instructions
void instr_call()
{
    mach_cpu[SP]++;
    mach_writemem(mach_cpu[SP], mach_cu[PC]);
    mach_cpu[SP]++;
    mach_writemem(mach_cpu[SP], mach_cpu[FP]);
    mach_cu[PC] = ins.sec_operand_value - 1;
    mach_cpu[FP] = mach_cpu[SP];
}
void instr_exit()
{
    mach_cpu[SP] = mach_cpu[FP] - 2 - ins.sec_operand_value;
    mach_cu[PC] = mach_readmem(mach_cpu[FP] - 1);
    mach_cpu[FP] = mach_readmem(mach_cpu[FP]);
}
// ----- Syscalls
void instr_svc()
{
    switch (ins.addr)
    {
    case svc_halt:
        // Set PC to -1.
        // -2 is because PC is incremented right after.
        mach_cu[PC] = -2;
        return;
    default: // The rest of the syscalls aren't essential and are skiped for now.
        printf("Unknown syscall.");
        return;
    }
}

// ----- Functions
void exec_instr()
{
    load_instruction(mach_cu[IR]);

    switch (ins.opcode)
    {
    case NOP:
        instr_nop();
        break;
    case STORE:
        instr_store();
        break;
    case LOAD:
        instr_load();
        break;
    case IN:
        instr_in();
        break;
    case OUT:
        instr_out();
        break;
    case ADD:
        instr_add();
        break;
    case SUB:
        instr_sub();
        break;
    case MUL:
        instr_mul();
        break;
    case DIV:
        instr_div();
        break;
    case MOD:
        instr_mod();
        break;
    case AND:
        break;
    case OR:
        break;
    case XOR:
        break;
    case SHL:
        break;
    case SHR:
        break;
    case NOT:
        break;
    case SHRA:
        break;
    case COMP:
        instr_comp();
        break;
    case JUMP:
        instr_jump();
        break;
    case JNEG:
        instr_jneg();
        break;
    case JZER:
        instr_jzer();
        break;
    case JPOS:
        instr_jpos();
        break;
    case JNNEG:
        instr_jnneg();
        break;
    case JNZER:
        instr_jnzer();
        break;
    case JNPOS:
        instr_jnpos();
        break;
    case JLES:
        instr_jles();
        break;
    case JEQU:
        instr_jequ();
        break;
    case JGRE:
        instr_jgre();
        break;
    case JNLES:
        instr_jnles();
        break;
    case JNEQU:
        instr_jnequ();
        break;
    case JNGRE:
        instr_jngre();
        break;
    case CALL:
        instr_call();
        break;
    case EXIT:
        instr_exit();
        break;
    case PUSH:
        instr_push();
        break;
    case POP:
        instr_pop();
        break;
    case PUSHR:
        instr_push();
        break;
    case POPR:
        instr_popr();
        break;
    case SVC:
        instr_svc();
        break;
    }
    if (waiting_for_input)
        return;
    mach_cu[PC]++;
}

void print_instr()
{
    load_instruction(mach_cu[IR]);

    switch (ins.opcode)
    {
    case NOP:
        printf("NOP   ");
        printf("\n");
        return;
        break;
    case STORE:
        printf("STORE ");
        break;
    case LOAD:
        printf("LOAD  ");
        break;
    case IN:
        printf("IN    ");
        break;
    case OUT:
        printf("OUT   ");
        break;
    case ADD:
        printf("ADD   ");
        break;
    case SUB:
        printf("SUB   ");
        break;
    case MUL:
        printf("MUL   ");
        break;
    case DIV:
        printf("DIV   ");
        break;
    case MOD:
        printf("MOD   ");
        break;
    case AND:
        printf("AND   ");
        break;
    case OR:
        printf("OR    ");
        break;
    case XOR:
        printf("XOR   ");
        break;
    case SHL:
        printf("SHL   ");
        break;
    case SHR:
        printf("SHR   ");
        break;
    case NOT:
        printf("NOT   ");
        break;
    case SHRA:
        printf("SHRA  ");
        break;
    case COMP:
        printf("COMP  ");
        break;
    case JUMP:
        printf("JUMP  ");
        break;
    case JNEG:
        printf("JNEG  ");
        break;
    case JZER:
        printf("JZER  ");
        break;
    case JPOS:
        printf("JPOS  ");
        break;
    case JNNEG:
        printf("JNNEG ");
        break;
    case JNZER:
        printf("JNZER ");
        break;
    case JNPOS:
        printf("JNPOS ");
        break;
    case JLES:
        printf("JLES  ");
        break;
    case JEQU:
        printf("JEQU  ");
        break;
    case JGRE:
        printf("JGRE  ");
        break;
    case JNLES:
        printf("JNLES ");
        break;
    case JNEQU:
        printf("JNEQU ");
        break;
    case JNGRE:
        printf("JNGRE ");
        break;
    case CALL:
        printf("CALL  ");
        break;
    case EXIT:
        printf("EXIT  ");
        break;
    case PUSH:
        printf("PUSH  ");
        break;
    case POP:
        printf("POP   ");
        break;
    case PUSHR:
        printf("PUSHR ");
        break;
    case POPR:
        printf("POPR  ");
        break;
    case SVC:
        printf("SVC   ");
        break;
    }

    // Print Rj
    if (ins.rj < 6)
        printf("R%d, ", ins.rj);
    else if (ins.rj == 6)
        printf("SP, ");
    else
        printf("FP, ");

    // Print mode
    if (ins.mode == 0)
        printf("=");
    else if (ins.mode == 2)
        printf("@");

    if (ins.ri == 0)
    {
        // Print address
        printf("%d", ins.addr);
    }
    else if (ins.addr == 0)
    {
        // Print Rj
        if (ins.ri == 0)
            printf("");
        else if (ins.ri < 6)
            printf("R%d", ins.ri);
        else if (ins.ri == 6)
            printf("SP");
        else
            printf("FP");
    }
    else
    {
        // Print address
        printf("%d", ins.addr);
        // Print Rj
        if (ins.ri < 6)
            printf("(R%d)", ins.ri);
        else if (ins.ri == 6)
            printf("(SP)");
        else
            printf("FP)");
    }
    printf("\n");
}