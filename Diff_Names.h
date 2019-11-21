//
// Created by tester on 22.11.2019.
//

#ifndef DIFFERENTIATOR_DIFF_NAMES_H
#define DIFFERENTIATOR_DIFF_NAMES_H

#define MAX_LEN_OPER 8
#define MAX_LEN_ARG 31
#define MAX_QUANT_ARG 32

enum _data_type {
    NUM = 1,
    VAR, OPER_or_FUNC
};

enum operators_number {
    ADD, SUB, MUL, DIV,
    POW, LOG, LN, LG,                       // LOG != LN
    SIN, COS,
    TAN, CTAN, TG, CTG,

    SH, CH, TH, CTH,
    SINH, COSH, TANH, CTANH,

    ARCSIN, ARCCOS,
    ARCTAN, ARCCTAN, ARCTG, ARCCTG,

    ARSH, ARCH, ARTH, ARCTH,
    ARSINH, ARCOSH, ARTANH, ARCTANH,

    EXP, NUM_OPER
};

enum separators {
    // При их изменении необходимо также изменить sscanf в Tree::read_expression
            LSEP = '(', RSEP = ')'
};

char _operators[NUM_OPER][MAX_LEN_OPER] = {
        "+", "-", "*", "/",
        "^", "log", "ln", "lg",             // log != ln
        "sin", "cos",
        "tan", "ctan", "tg", "ctg",

        "sh", "ch", "th", "cth",
        "sinh", "cosh", "tanh", "ctanh",

        "arcsin", "arccos",
        "arctan", "arcctan", "arctg", "arcctg",

        "arsh", "arch", "arth", "arcth",
        "arsinh", "arcosh", "artanh", "arctanh",

        "exp"
};

#define CREATE_NODE_OPERATOR(left_expr, right_expr, _OPER) \
        crtNode (OPER_or_FUNC, _OPER, left_expr, right_expr, curr_node->prev)

#define _NUM(value) crtNode (NUM, value, nullptr, nullptr, curr_node->prev)

#define _ADD(left_expr, right_expr)   CREATE_NODE_OPERATOR (left_expr, right_expr, ADD)
#define _SUB(left_expr, right_expr)   CREATE_NODE_OPERATOR (left_expr, right_expr, SUB)
#define _MUL(left_expr, right_expr)   CREATE_NODE_OPERATOR (left_expr, right_expr, MUL)
#define _DIV(left_expr, right_expr)   CREATE_NODE_OPERATOR (left_expr, right_expr, DIV)
#define _POW(left_expr, right_expr)   CREATE_NODE_OPERATOR (left_expr, right_expr, POW)

#define _HFN(right_expr)               _MUL (dR, right_expr)                                //Сложная функция

#define _SIN(right_expr)               CREATE_NODE_OPERATOR (nullptr, right_expr, SIN)
#define _COS(right_expr)               CREATE_NODE_OPERATOR (nullptr, right_expr, COS)
#define _TAN(right_expr)               CREATE_NODE_OPERATOR (nullptr, right_expr, TAN)
#define _CTAN(right_expr)              CREATE_NODE_OPERATOR (nullptr, right_expr, CTAN)
#define _SINH(right_expr)              CREATE_NODE_OPERATOR (nullptr, right_expr, SH)       // Можно добавить выбор
#define _COSH(right_expr)              CREATE_NODE_OPERATOR (nullptr, right_expr, CH)       // SN или SINH как локаль

#define _EXP(right_expr)               CREATE_NODE_OPERATOR (nullptr, right_expr, EXP)
#define _LN(right_expr)                CREATE_NODE_OPERATOR (nullptr, right_expr, LN)

#define L curr_node->left
#define R curr_node->right
#define C curr_node
#define _TYPE(curr_node) curr_node->data.type
#define _NUMBER(curr_node) curr_node->data.number

#define cL Copy (curr_node->left)
#define cR Copy (curr_node->right)
#define cN Copy (curr_node)

#define dL Diff (curr_node->left)
#define dR Diff (curr_node->right)

#endif //DIFFERENTIATOR_DIFF_NAMES_H
