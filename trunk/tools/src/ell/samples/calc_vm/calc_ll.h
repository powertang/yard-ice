#ifndef __CALC_LL_H__
#define __CALC_LL_H__

#include <stdint.h>

/* Last symbol num:120  Num rules:75 */

#define T_EOF                    0
#define T_DOT                    1
#define T_COMMA                  2
#define T_SEMICOLON              3
#define T_COLON                  4
#define T_LBRACKET               5
#define T_RBRACKET               6
#define T_LPAREN                 7
#define T_RPAREN                 8
#define T_LBRACE                 9
#define T_RBRACE                10
#define T_ASR                   11
#define T_SHL                   12
#define T_LTE                   13
#define T_LT                    14
#define T_GTE                   15
#define T_GT                    16
#define T_EQU                   17
#define T_NEQ                   18
#define T_PLUS                  19
#define T_MINUS                 20
#define T_MUL                   21
#define T_DIV                   22
#define T_MOD                   23
#define T_OR                    24
#define T_LOR                   25
#define T_AND                   26
#define T_LAND                  27
#define T_XOR                   28
#define T_NOT                   29
#define T_INV                   30
#define T_QUEST                 31
#define T_ASSIGN                32
#define T_ELSE                  33
#define T_FALSE                 34
#define T_FOR                   35
#define T_IF                    36
#define T_PRINT                 37
#define T_TRUE                  38
#define T_VAR                   39
#define T_WHILE                 40
#define T_ID                    41
#define T_INT                   42
#define T_ERR                   43

#define NONTERM_BASE            44
#define N_PROGRAM               44
#define N_STAT                  45
#define N_COMPOUND_STAT         46
#define N_STAT_LIST             47
#define N_EXP_LST_OPT           48
#define N_EXP                   49
#define N_CONDITION             50
#define N_ELSE_OPT              51
#define N_VAR_LIST              52
#define N_ASSIGN_OR_CALL        53
#define N_VAR                   54
#define N_VAR_LIST1             55
#define N_VAR_ASSIGN_OPT        56
#define N_ASSIGN_OR_CALL1       57
#define N_ARG_LIST_OPT          58
#define N_ARG_LIST              59
#define N_ARG_LIST1             60
#define N_EXP_LST               61
#define N_EXP_LST1              62
#define N_AND_EXP               63
#define N_OR_EXP                64
#define N_RELATIONAL_EXP        65
#define N_AND_EXP1              66
#define N_SHIFT_EXP             67
#define N_RELATIONAL_EXP1       68
#define N_ADDITIVE_EXP          69
#define N_SHIFT_EXP1            70
#define N_MULT_EXP              71
#define N_ADDITIVE_EXP1         72
#define N_UNARY_EXP             73
#define N_MULT_EXP1             74
#define N_PRIMARY_EXP           75
#define N_METH_OR_ATTR          76
#define N_METH_OR_ATTR1         77

#define ACTION_BASE             78
#define A_OP_FOR_INIT           78
#define A_OP_FOR_COND           79
#define A_OP_FOR_AFTER          80
#define A_OP_FOR_END            81
#define A_OP_WHILE_BEGIN        82
#define A_OP_WHILE_COND         83
#define A_OP_WHILE_END          84
#define A_OP_IF_COND            85
#define A_OP_IF_END             86
#define A_OP_IF_ELSE            87
#define A_OP_VAR_DECL           88
#define A_OP_PUSH_SYM           89
#define A_OP_POP_SYM            90
#define A_OP_ASSIGN             91
#define A_OP_METHOD             92
#define A_OP_METH_OR_ATTR       93
#define A_OP_RET_DISCARD        94
#define A_OP_ARG                95
#define A_OP_OR                 96
#define A_OP_LOGIC_OR           97
#define A_OP_XOR                98
#define A_OP_AND                99
#define A_OP_LOGIC_AND         100
#define A_OP_EQU               101
#define A_OP_NEQ               102
#define A_OP_LT                103
#define A_OP_GT                104
#define A_OP_GTE               105
#define A_OP_LTE               106
#define A_OP_SHL               107
#define A_OP_ASR               108
#define A_OP_ADD               109
#define A_OP_SUB               110
#define A_OP_MUL               111
#define A_OP_DIV               112
#define A_OP_MOD               113
#define A_OP_INV               114
#define A_OP_MINUS             115
#define A_OP_NOT               116
#define A_OP_PUSH_INT          117
#define A_OP_PUSH_TRUE         118
#define A_OP_PUSH_FALSE        119

#define IS_A_TERMINAL(_X) ((_X) < NONTERM_BASE)
#ifdef ACTION_BASE
#define IS_A_NONTERMINAL(_X) ((_X) >= NONTERM_BASE && (_X) < ACTION_BASE)
#define IS_AN_ACTION(_X) ((_X) >= ACTION_BASE)
#define ACTION(_X) ((_X) - ACTION_BASE)
#else
#define IS_A_NONTERMINAL(_X) ((_X) >= NONTERM_BASE)
#define IS_AN_ACTION(_X) (0)
#define ACTION(_X) (0)
#endif

extern const const char * const ll_sym_tab[];

#ifdef __cplusplus
extern "C" {
#endif

int ll_rule_push(uint8_t * sp, unsigned int sym, unsigned int tok);
int ll_start(uint8_t * sp);

#ifdef __cplusplus
}
#endif

#endif /* __CALC_LL_H__ */


