#include <cstdio>
#include <MyLib2.h>
#include <cmath>
#include <iostream>

#define MAX_LEN_OPER 8
#define MAX_LEN_ARG 31
#define MAX_QUANT_ARG 32

typedef double number_t;

typedef struct data_tree {
    unsigned char type;     // Число, переменная, выражение (sin(lg(x*y^z)))
    number_t number;          // Число
} data_tree_t;

namespace tree_diff {
    typedef data_tree_t data_t;

    enum consts {
        RIGHT,
        LEFT,
    };

    enum errors {
        ALL_RIGHT,
        EFILLROOT,
        EFILLNODE,
        EREAD,
        EADDRIGHT,
        EADDLEFT,
        SYNERROR,
        EUNIDENT
    };

    typedef struct nodeTree {
    public:
        data_t data;
        nodeTree *prev;
        nodeTree *right;
        nodeTree *left;

        // Аргумент aux_arg может исопльзоваться для передачи, например указателя на дерево этого узла
        int visitor_pref_r (int (*func) (nodeTree *, void *), void *aux_arg = nullptr);

        int visitor_pref_l (int (*func) (nodeTree *, void *), void *aux_arg = nullptr);
    } node_t;

    // Визитор для строк, так как делает проверку на nullptr в data
    // Справа налево
    // Нет проверки на существование данных в узле !!!
    int nodeTree::visitor_pref_r (int (*func) (nodeTree *, void *), void *aux_arg) {
        int state = 0;

        if ((state = func (this, aux_arg)) != 0)
            return state;

        if (right != nullptr && (state = right->visitor_pref_r (func, aux_arg)) != 0)
            return state;

        if (left != nullptr && (state = left->visitor_pref_r (func, aux_arg)) != 0)
            return state;


        return 0;
    }

    int nodeTree::visitor_pref_l (int (*func) (nodeTree *, void *), void *aux_arg) {
        int state = 0;

        if ((state = func (this, aux_arg)) != 0)
            return state;

        if (left != nullptr && (state = left->visitor_pref_r (func, aux_arg)) != 0)
            return state;

        if (right != nullptr && (state = right->visitor_pref_r (func, aux_arg)) != 0)
            return state;

        return 0;
    }

    // Дерево саморасширяемо, но при этом не уменьшает свои размеры обратно
    typedef struct Tree {
        node_t *node;       // Указатель на корень

        Tree ();

        int verificator ();

        inline node_t *add_right (node_t *in_node, data_t value);

        inline node_t *add_left (node_t *in_node, data_t value);

        int read_expression (char *source);

        int save_to_file (char *name_file);

        static node_t *crtNode (int type, number_t value, node_t *left, node_t *right, node_t *prev = nullptr);

        static node_t *get_Diff (node_t *to_node, node_t *curr_node, int how_many = 1);

        static node_t *Copy (const node_t *curr_node);

        static int Optimizer (node_t *head_node);

        static int del_Branch_Left (node_t *curr_node);     // Рекурентно удалить у переданного узла левую ветку

        static int del_Branch_Right (node_t *curr_node);    // Рекурентно удалить у переданного узла правую ветку

        static int del_Node_Left (node_t *curr_node);   // Вырезает узел с его левой частью из дерева, устраняя разрыв
        // Рекурентно удалить у переданного узла левую ветку, сам узел
        // и склеить правую ветку и родителя удалённого узла.
        static int del_Node_Right (node_t *curr_node);  // Вырезает узел с его правой частью из дерева, устраняя разрыв
        // Рекурентно удалить у переданного узла правую ветку, сам узел
        // и склеить левую ветку и родителя удалённого узла.

    private:
        node_t *add (int right_or_left, node_t *curr_node, data_t value);

        static node_t *get_new_node ();

        static node_t *Diff (const node_t *curr_node);

        static int Opt (node_t *curr_node);

        static int set_Prev (node_t *curr_node);

        static int del_Branch (node_t *del_node);
    } tree_t;

/*  TODO Другая версия конструктора (кажется хуже, чем действующая)
    Tree::Tree (size_t _count, unsigned _step = 0) : count (_count), step (_step) {
        if (count < min_count () && step <= 0) count = min_count ();

        if ((node = (node_t *) calloc (count, sizeof (node_t))) == nullptr) {
            PRINT_ERROR ("Constructor error: failed to create node array")
            return;
        }
        empty_node = node + 1;
        node->left = node->right = node->prev = nullptr;
        assert (verificator ());
    }
*/
    Tree::Tree () {
        node = nullptr;
        if ((node = get_new_node ()) == nullptr) {
            PRINT_ERROR ("Error construct tree")
            return;
        }
        node->left = node->right = nullptr;
        node->prev = node;
        assert (verificator ());
    }

    int Tree::verificator () {
        assert (node != nullptr);
        assert (node->prev != nullptr);   // Предком корня может быть только он сам

        return 1;
    }

    inline node_t *Tree::add_right (node_t *in_node, data_t value) {
        return add (RIGHT, in_node, value);
    }

    inline node_t *Tree::add_left (node_t *in_node, data_t value) {
        return add (LEFT, in_node, value);
    }

    node_t *Tree::add (int right_or_left, node_t *curr_node, data_t value) {
        // Не используется удобное добавление непосредственно и использованием БД
        assert (verificator ());
        node_t *new_node = nullptr;                         // Указатель новый узел
        node_t **temp_ptr = nullptr;                        // Указатель для определения выбора режима вставки

        if (right_or_left == RIGHT)
            temp_ptr = &(curr_node->right);
        else
            temp_ptr = &(curr_node->left);

        if ((new_node = get_new_node ()) == nullptr)
            return nullptr;                                 // Критическая ошибка

        new_node->data = value;
        new_node->prev = curr_node;

        // Вставить между узлами новый узел
        // А его left и right указывают на следующий элемент (right == left)
        // сделать проверку на этот случай
        if ((new_node->left = new_node->right = *temp_ptr) != nullptr) {

            if (curr_node->left == curr_node->right)            // Для случая, когда в исходном
                new_node->left = new_node->right = nullptr;     // дереве указатели на left и right были равны
            else
                (*temp_ptr)->prev = new_node;
        }

        *temp_ptr = new_node;
        return new_node;
    }

    node_t *Tree::get_new_node () {
        node_t *new_node = nullptr;
        if ((new_node = new node_t{0}) == nullptr) {
            PRINT_ERROR ("Failed to create a new node")
            return nullptr;
        }

        return new_node;
    }

    int save_tree_to_file_vis (tree_diff::node_t *curr_node, void *file) {/*
        fprintf ((FILE *) file, "{ \"%s\" ", curr_node->data);
        fflush ((FILE *) file);
        if (curr_node->right != nullptr) {
            if (curr_node->right->visitor_pref (save_tree_to_file_vis, file) < 0)
                return -1;
        }
        if (curr_node->left != nullptr) {
            if (curr_node->left->visitor_pref_r (save_tree_to_file_vis, file) < 0)
                return -1;
        }

        fprintf ((FILE *) file, "} ");*/
        return 1;
    }

    int Tree::save_to_file (char *name_file) {
        FILE *file = nullptr;
        if ((file = fopen (name_file, "w")) == nullptr) {
            PRINT_ERROR ("Error open file")
            return 0;
        }
        if (node->visitor_pref_r (save_tree_to_file_vis, stdout) < 0) {
            PRINT_ERROR ("Error save to file")
            return 0;
        }
        return 0;
    }
}

void Dump_Tree (FILE *file_out, tree_diff::node_t *head);

void Dump_Tree_img (tree_diff::node_t *head);

void Dump_Tree_Tex (tree_diff::node_t *head, char *name_file = nullptr);

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
    // При их изменении необходимо также изменить sscanf в tree_diff::Tree::read_expression
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

unsigned char _arguments[MAX_QUANT_ARG][MAX_LEN_ARG + 1] = {0};     // Не существует нулевых аргументов

int check_arg (const char *argument) {
    for (int i = 0; i < MAX_QUANT_ARG; i++)
        if (!_arguments[i][0]) {
            if (strcpy ((char *) _arguments[i], argument) == nullptr) {
                if (strlen (argument) > MAX_LEN_ARG) {
                    PRINT_ERROR ("Error to add a new argument: argument name too long")
                    return -1;
                }
                PRINT_ERROR ("Error to add a new argument");
                return -1;
            } else
                return 0;
        } else if (strcmp ((char *) _arguments[i], argument) == 0)
            return i;

    PRINT_ERROR ("Too many arguments!")
    return -1;
}

number_t get_ind_oper (char *oper) {
    for (int i = 0; i < NUM_OPER; i++)
        if (strcmp (oper, _operators[i]) == 0)
            return i;
    return -1;
}

int tree_diff::Tree::read_expression (char *source) {
    /* Использую bigarr
     * Уже использую %n)))
     * Не использую %n в sscanf (), потому что размер русского символа два байта, а не один в англ. кодировке.
     * Проблема в том, что внутри строки могут быть числа, которые придётся выделять и т.п.
     * Лучше просто использовать strlen и радоваться жизни
    */

    char str[512] = "";
    int brk_count = 0;   // Счётчик скобок (исп. для окончания цикла и поиска синт. ошибок)

    tree_diff::node_t *curr_node = this->node;  // Указатель на текущий узел
    number_t temp_num = 0;                      // Используется для проверки на число или оператор
    int n_left = 0, n_right = 0;                // Флаги для %n в sscanf, чтобы вычислять длину строки
    int oper_on = 0;

    source--;
    while (true) {
        if (*(++source) == '\0')        // Проверка на окончание файла
            break;
        else if (isspace (*source))     // Пропуск пробелов
            continue;
        else if (*source == LSEP) {
            brk_count++;
            if (!oper_on) {
                if ((curr_node = add_left (curr_node, {0})) == nullptr)
                    return tree_diff::EADDLEFT;
            } else
                oper_on--;

        } else if (*source == RSEP) {
            brk_count--;
            curr_node = curr_node->prev;
        } else {
            // Считываем либо число, либо оператор, либо функцию
            if (sscanf (source, "%n%[^() ]%n", &n_left, str, &n_right) == 0) {
                PRINT_ERROR ("ERROR READ")
                return tree_diff::EREAD;
            }
            //printf ("THIS: %s\n", str);
            source += n_right - n_left - 1;
            if ((temp_num = get_ind_oper (str)) != (number_t) -1) {         // Это оператор или функция?
                curr_node->data.type = OPER_or_FUNC;
                curr_node->data.number = temp_num;
                oper_on++;
                if ((curr_node = add_right (curr_node, {0})) == nullptr)
                    return tree_diff::EADDLEFT;

            } else if (sscanf (str, "%lg", &temp_num)) {                    // Это число?
                curr_node->data.type = NUM;
                curr_node->data.number = temp_num;
            } else {                                                        // Всё остальное считается аргументом
                curr_node->data.type = VAR;

            }
        }
        //Dump_Tree (stdout, this->node);
    }
    return 0;
}

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

#define _FUNC(ARG, EQUAL)                                                                           \
                if (_TYPE (R) == NUM && _NUMBER (R) == ARG) {                                       \
                    _TYPE (C) = NUM;                                                                \
                    _NUMBER (C) = EQUAL;                                                            \
                    del_Branch_Right (C);                                                           \
                    return 0;                                                                       \
                }

tree_diff::node_t *tree_diff::Tree::Diff (const tree_diff::node_t *curr_node) {
    switch (curr_node->data.type) {
        case NUM:
            return _NUM (0);
        case VAR:
            return _NUM (1);
        case OPER_or_FUNC:
            switch ((int) curr_node->data.number) {
                case ADD:                                                               // Простая арифметика
                    return _ADD (dL, dR);
                case SUB:
                    return _SUB (dL, dR);
                case MUL:
                    return _ADD (_MUL (dL, cR), _MUL (cL, dR));
                case DIV:
                    return _DIV ((_SUB (_MUL (dL, cR), _MUL (cL, dR))), (_MUL (cR, cR)));
                case POW:
                    if (_TYPE (R) == NUM)
                        return _MUL (dL, (_MUL (cR, _POW (cL, _SUB (cR, _NUM (1))))));

                    return _MUL (Diff (_MUL (cR, _LN (cL))), cN);
                case SIN:                                                               // Обычная тригонометрия
                    return _HFN (_COS (cR));
                case COS:
                    return _HFN (_MUL (_NUM (-1), _SIN (cR)));
                case TAN:
                case TG:
                    return _HFN (_DIV (_NUM (1), _POW (_COS (cR), _NUM (2))));
                case CTAN:
                case CTG:
                    return _HFN (_DIV (_NUM (-1), _POW (_SIN (cR), _NUM (2))));

                case SH:                                                                // Гиперболическая тригонометрия
                case SINH:
                    return _HFN (_COSH (cR));
                case CH:
                case COSH:
                    return _HFN (_SINH (cR));
                case TH:
                case TANH:
                    return _HFN (_DIV (_NUM (1), _POW (_COSH (cR), _NUM (2))));
                case CTH:
                case CTANH:
                    return _HFN (_DIV (_NUM (-1), _POW (_SINH (cR), _NUM (2))));

                case ARCSIN:                                                            // Обратные функции триг.
                    return _HFN (_DIV (_NUM (1), _POW (_SUB (_NUM (1), _POW (cR, _NUM (2))), _NUM (0.5))));
                case ARCCOS:
                    return _HFN (_DIV (_NUM (-1), _POW (_SUB (_NUM (1), _POW (cR, _NUM (2))), _NUM (0.5))));
                case ARCTAN:
                case ARCTG:
                    return _HFN (_DIV (_NUM (1), _ADD (_NUM (1), _POW (cR, _NUM (2)))));
                case ARCCTAN:
                case ARCCTG:
                    return _HFN (_DIV (_NUM (-1), _ADD (_NUM (1), _POW (cR, _NUM (2)))));

                case ARSH:                                                              // Обратные функции гип. триг.
                case ARSINH:
                    return _HFN (_DIV (_NUM (1), _POW (_ADD (_NUM (1), _POW (cR, _NUM (2))), _NUM (0.5))));
                case ARCH:
                case ARCOSH:
                    return _HFN (_DIV (_NUM (1), _POW (_SUB (_POW (cR, _NUM (2)), _NUM (1)), _NUM (0.5))));
                case ARTH:
                case ARTANH:
                case ARCTH:
                case ARCTANH:
                    return _HFN (_DIV (_NUM (1), _SUB (_NUM (1), _POW (cR, _NUM (2)))));
                case EXP:
                    return _EXP (cR);
                case LN:
                    return _DIV (dR, cR);

                case LOG:                   // Не оч работает
                    return Diff (_DIV (_LN (cL), _LN (cR)));

                case LG:
                    return _DIV (Diff (_LN (cL)), _LN (_NUM (10)));
                default:
                    printf ("Undef");
                    return nullptr;
            }
    }
    return nullptr;
}

tree_diff::node_t *tree_diff::Tree::crtNode (int type, number_t value, node_t *left, node_t *right, node_t *prev) {
    node_t *new_node = nullptr;

    if ((new_node = get_new_node ()) == nullptr) {
        PRINT_ERROR ("ERROR IN DIFF")
        return nullptr;
    }
    new_node->data.type = type;
    new_node->data.number = value;

    new_node->prev = prev;
    if (left != nullptr)
        new_node->left = left;
    if (right != nullptr)
        new_node->right = right;
    return new_node;
}

tree_diff::node_t *tree_diff::Tree::Copy (const tree_diff::node_t *curr_node) {
    node_t *new_node = nullptr;

    if ((new_node = get_new_node ()) == nullptr) {
        PRINT_ERROR ("Error copy tree")
        return nullptr;
    }

    new_node->data = curr_node->data;
    new_node->prev = curr_node->prev;

    if (curr_node->left != nullptr)
        if ((new_node->left = Copy (curr_node->left)) == nullptr) {
            PRINT_ERROR ("Error copy left tree")
            return nullptr;
        }
    if (curr_node->right != nullptr)
        if ((new_node->right = Copy (curr_node->right)) == nullptr) {
            PRINT_ERROR ("Error copy right tree")
            return nullptr;
        }
    return new_node;
}

tree_diff::node_t *tree_diff::Tree::get_Diff (node_t *to_node, tree_diff::node_t *curr_node, int how_many) {
    tree_t temp_tree_1, temp_tree_2;
    node_t *save_node = temp_tree_2.node;
    temp_tree_2.node = curr_node;

    for (int i = 0; i < how_many; i++) {
        if (i + 1 == how_many)
            temp_tree_1.node = to_node;
        bool Curr_is_Root = (temp_tree_2.node == temp_tree_2.node->prev);

        *(temp_tree_1.node) = *Diff (temp_tree_2.node);

        // Нужно переопределять родителя корня, т.к. в при дифф. родителей не переопределяют, а у корня
        // родителем обязательно должен быть он сам:
        if (Curr_is_Root)
            temp_tree_1.node->prev = temp_tree_1.node;

        set_Prev (temp_tree_1.node); // По той же причине (см. выше) определяю родителей у всех узлов полученного дерева

        if (!Optimizer (temp_tree_1.node)) {
            PRINT_ERROR ("Error optimizer")
            return nullptr;
        }

        if (i == 0) {
            temp_tree_2.node = save_node;
            temp_tree_2.node->prev = save_node;
            temp_tree_2.node->left = temp_tree_2.node->right = nullptr;
        } else if (i + 1 == how_many)
            break;

        del_Branch_Right (temp_tree_2.node);
        del_Branch_Left (temp_tree_2.node);
        temp_tree_2.node->data = {0};

        node_t *revers = temp_tree_2.node;
        temp_tree_2.node = temp_tree_1.node;
        temp_tree_1.node = revers;
    }

    return to_node;
}

int tree_diff::Tree::Optimizer (tree_diff::node_t *head_node) {
    if (head_node == nullptr)
        return 0;
    // Оптимизируем, пока не перестанет изменяться
    while (true) {
        if (!Opt (head_node))
            continue;
        else if (Opt (head_node))
            break;
        else
            continue;
    }

    return 1;
}

number_t exec_oper (int Oper, number_t Left, number_t Right = 1) {
    switch (Oper) {
        case ADD:
            return Left + Right;
        case SUB:
            return Left - Right;
        case MUL:
            return Left * Right;
        case DIV:
            return Left / Right;
        case EXP:
            return exp (Right);

        case SIN:
            return sin (Right);
        case COS:
            return cos (Right);
        case TAN:
        case TG:
            return tan (Right);
        case CTAN:
        case CTG:
            return 1 / tan (Right);

        case SINH:
        case SH:
            return sinh (Right);
        case COSH:
        case CH:
            return cosh (Right);
        case TANH:
        case TH:
            return tanh (Right);
        case CTANH:
        case CTH:
            return 1 / tanh (Right);

        case ARCSIN:
            return asin (Right);
        case ARCCOS:
            return acos (Right);
        case ARCTAN:
        case ARCTG:
            return atan (Right);
        case ARCCTAN:
        case ARCCTG:
            return atan (1 / Right);

        case ARSINH:
        case ARSH:
            return asin (Right);
        case ARCOSH:
        case ARCH:
            return acosh (Right);
        case ARTANH:
        case ARTH:
            return atanh (Right);
        case ARCTANH:
        case ARCTH:
            return atanh (1 / Right);

        case LN:
            return log (Right);
            /*case LOG:
                return log (R)  // Кажется, не будет  использоваться
            */
        case LG:
            return log (Right) / log (10);
        default:
        PRINT_ERROR ("Undefined operator")
    }

    return 0;
}

int tree_diff::Tree::Opt (tree_diff::node_t *curr_node) {
    // Считается, что дерево построенно корректно
    // (Т.е., например, в узле умжножения сущ., не равные нулю указатели на r и l, которые тоже корректны)

    if (_TYPE (C) == OPER_or_FUNC) {
        // Если можно вычислить число
        if ((L == nullptr && _TYPE (R) == NUM) || (L != nullptr && (_TYPE (L) == NUM && _TYPE (R) == NUM))) {
            _TYPE (C) = NUM;
            if (L != nullptr)
                _NUMBER (C) = exec_oper (_NUMBER (C), _NUMBER (L), _NUMBER (R));
            else
                _NUMBER (C) = exec_oper (_NUMBER (C), NAN, _NUMBER (R));

            del_Branch_Left (C);
            del_Branch_Right (C);
            return 0;
        }

        // Если бин. операции с 0
        switch ((int) _NUMBER (C)) {
            case SUB:   // f - 0
                if (!(_TYPE (R) == NUM && _NUMBER (R) == 0))
                    break;
            case ADD:   // f + 0 || 0 + f
                if (_TYPE (R) == NUM && _NUMBER (R) == 0) {
                    node_t *save_L = L;
                    del_Node_Right (C);
                    return Optimizer (save_L);
                }
                if (_TYPE (L) == NUM && _NUMBER (L) == 0) {
                    node_t *save_R = R;
                    del_Node_Left (C);
                    return Optimizer (save_R);
                }
                break;
            case MUL:   // f * 0 или 0 * f
                if ((_TYPE (L) == NUM && _NUMBER (L) == 0) || (_TYPE (R) == NUM && _NUMBER (R) == 0)) {
                    _TYPE (C) = NUM;
                    _NUMBER (C) = 0;
                    del_Branch_Right (C);
                    del_Branch_Left (C);
                    return 0;
                }
                break;
            case POW:   // 0 ^ f
                if (_TYPE (R) == NUM && _NUMBER (R) == 0) {
                    _TYPE (C) = NUM;
                    _NUMBER (C) = 1;
                    del_Branch_Right (C);
                    del_Branch_Left (C);
                    return 0;
                }
                break;
            case DIV:   // 0 / f
                if (_TYPE (L) == NUM && _NUMBER (L) == 0) {
                    _TYPE (C) = NUM;
                    _NUMBER (C) = 0;
                    del_Branch_Right (C);
                    del_Branch_Left (C);
                    return 0;
                }
                break;
        }

        // Если бин. операции с 1
        switch ((int) _NUMBER (C)) {
            case MUL:       // f * 1 || 1 * f
                if ((_TYPE (R) == NUM && _NUMBER (R) == 1)) {
                    node_t *save_L = L;
                    del_Node_Right (C);
                    return Optimizer (save_L);
                }
                if ((_TYPE (L) == NUM && _NUMBER (L) == 1)) {
                    node_t *save_R = R;
                    del_Node_Left (C);
                    return Optimizer (save_R);
                }
                break;
            case POW:       // Для любого x в д.ч. x^1 = x
                if ((_TYPE (L) == NUM && _NUMBER (L) == 1)) {
                    _TYPE (C) = NUM;
                    _NUMBER (C) = 1;
                    del_Branch_Right (C);
                    del_Branch_Left (C);
                    return 0;
                }
            case DIV:       // f / 1 = f
                if ((_TYPE (R) == NUM && _NUMBER (R) == 1)) {
                    node_t *save_L = L;
                    del_Node_Right (C);
                    return Optimizer (save_L);
                }
                break;
        }

        // Если
        if ((int) _NUMBER (C) == MUL) {
            if (L != nullptr && R != nullptr) {
                if (_TYPE (L) == NUM && (_TYPE (R) == OPER_or_FUNC && _NUMBER (R) == MUL)) {
                    if (_TYPE(R->left) == NUM) {
                        _NUMBER (L) *= _NUMBER (R->left);
                        del_Node_Left (R);
                    } else if (_TYPE (R->right) == NUM) {
                        _NUMBER (L) *= _NUMBER (R->right);
                        del_Node_Right (R);
                    }
                } else if (_TYPE (R) == NUM && (_TYPE (L) == OPER_or_FUNC && _NUMBER (L) == MUL)) {
                    if (_TYPE(L->left) == NUM) {
                        _NUMBER (R) *= _NUMBER (L->left);
                        del_Node_Left (L);
                    } else if (_TYPE (L->right) == NUM) {
                        _NUMBER (R) *= _NUMBER (L->right);
                        del_Node_Right (L);
                    }
                }
            }
        }
    }

    Optimizer (L);
    Optimizer (R);

    return 1;
}

int tree_diff::Tree::del_Branch_Left (tree_diff::node_t *curr_node) {
    if (curr_node->left != nullptr) {
        del_Branch (curr_node->left);
        curr_node->left = nullptr;
    }
    return 0;
}

int tree_diff::Tree::del_Branch_Right (tree_diff::node_t *curr_node) {
    if (curr_node->right != nullptr) {
        del_Branch (curr_node->right);
        curr_node->right = nullptr;
    }
    return 0;
}

int tree_diff::Tree::del_Branch (tree_diff::node_t *del_node) {
    if (del_node->right != nullptr) {
        del_Branch (del_node->right);
        del_node->right = nullptr;
    }

    if (del_node->left != nullptr) {
        del_Branch (del_node->left);
        del_node->left = nullptr;
    }

    free (del_node);
    return 0;
}

int tree_diff::Tree::del_Node_Left (tree_diff::node_t *curr_node) {
    del_Branch_Left (curr_node);

    node_t **ptrPrev_to_curr = nullptr;         // Сюда будет записан указатель на ячейку родителя, которая указывает на
    // текущее дерево

    if (curr_node->prev->left == curr_node)     // Получаем этот указатель
        ptrPrev_to_curr = &(curr_node->prev->left);
    else
        ptrPrev_to_curr = &(curr_node->prev->right);

    if (curr_node->prev != curr_node) {         // Если это не корень
        if (R != nullptr) {
            R->prev = curr_node->prev;
            *ptrPrev_to_curr = R;
            free (curr_node);
        } else {
            free (curr_node);
            *ptrPrev_to_curr = nullptr;
        }
    } else {                                    // Если это корень
        node_t *save_R_node = R;
        *curr_node = *R;
        if (L != nullptr)
            L->prev = curr_node;
        if (R != nullptr)
            R->prev = curr_node;
        free (save_R_node);
    }
    return 0;
}

int tree_diff::Tree::del_Node_Right (tree_diff::node_t *curr_node) {
    del_Branch_Right (curr_node);

    node_t **ptrPrev_to_curr = nullptr;         // Сюда будет записан указатель на ячейку родителя, которая указывает на
    // текущее дерево

    if (curr_node->prev->left == curr_node)     // Получаем этот указатель
        ptrPrev_to_curr = &(curr_node->prev->left);
    else
        ptrPrev_to_curr = &(curr_node->prev->right);

    if (curr_node->prev != curr_node) {         // Если это не корень
        if (L != nullptr) {
            L->prev = curr_node->prev;
            *ptrPrev_to_curr = L;
            free (curr_node);
        } else {
            free (curr_node);
            *ptrPrev_to_curr = nullptr;
        }
    } else {                                    // Если это корень
        node_t *save_L_node = L;
        *curr_node = *L;
        if (R != nullptr)
            R->prev = curr_node;
        if (L != nullptr)
            L->prev = curr_node;
        free (save_L_node);
    }
    return 0;
}

int tree_diff::Tree::set_Prev (tree_diff::node_t *curr_node) {
    if (R != nullptr) {
        R->prev = curr_node;
        set_Prev (R);
    }

    if (L != nullptr) {
        L->prev = curr_node;
        set_Prev (L);
    }

    return 0;
}

int main () {
    tree_diff::Tree tree, diff_tree, diff_tree1;

    char name_TeX_file[] = "MyLaTeX.tex";
    char string_in[] = "(x)^(x)";

    if (tree.read_expression (string_in)) {
        PRINT_ERROR ("Failed to read tree from file")
        return 0;
    }

    Dump_Tree_img (tree.node);

    if (!tree.Optimizer (tree.node)) {
        PRINT_ERROR ("Error optimizer")
        return 0;
    }

    Dump_Tree_img (tree.node);

    if (diff_tree.get_Diff (diff_tree.node, tree.node, 4) == nullptr) {
        PRINT_ERROR ("Error copy")
        return 0;
    }
    /*
    Dump_Tree_img (diff_tree.node);

    if (!diff_tree.Optimizer (diff_tree.node)) {
        PRINT_ERROR ("Error optimizer")
        return 0;
    }
    */
    Dump_Tree_img (diff_tree.node);

    Dump_Tree_Tex (diff_tree.node, name_TeX_file);
    return 0;
}

int tex_printf (tree_diff::node_t *node, FILE *file) {

    fprintf (file, "(");

    if (node->left != nullptr) {
        if (tex_printf (node->left, file) < 0) {
            PRINT_ERROR ("Failed to visitor")
            return -1;
        }
    }

    switch (node->data.type) {
        case OPER_or_FUNC:
            fprintf (file, "%s", &(_operators[(int) node->data.number][0]));
            break;
        case VAR:
            fprintf (file, "%c", 'x');
            break;
        case NUM:
            fprintf (file, "%lg", node->data.number);
            break;
        default:
        PRINT_ERROR ("Undefined type's node")
    }

    if (node->right != nullptr) {
        if (tex_printf (node->right, file) < 0) {
            PRINT_ERROR ("Failed to visitor")
            return -1;
        }
    }

    fprintf (file, ")");

    return 0;
}

void Dump_Tree_Tex (tree_diff::node_t *head, char *name_file) {
    FILE *file = nullptr;

    if (name_file == nullptr)
        file = stdout;
    else if ((file = fopen (name_file, "w")) == nullptr) {
        PRINT_ERROR ("Error create TEX file")
        return;
    }

    fprintf (file, "%s",  "\\documentclass[12pt,a4paper]{article}\n"
                          "\\usepackage[latin1]{inputenc}\n"
                          "\\usepackage{amsmath}\n"
                          "\\usepackage{amsfonts}\n"
                          "\\usepackage{amssymb}\n"
                          "\\usepackage{graphicx}\n"
                          "\\begin{document}\n");

    if (tex_printf (head, file) < 0) {
        PRINT_ERROR ("Failed to visitor")
    }

    fprintf (file, "%s",  "\n\\end{document}");
}

namespace Dtdot {

    int ident_dot (tree_diff::node_t *node, void *) {
        printf ("\t%zu [label = \"", (size_t) node);
        if (node->data.type == VAR) {
            printf ("%c\"];\n", 'x');
        } else if (node->data.type == OPER_or_FUNC) {
            printf ("%s\"];\n", _operators[(int) node->data.number]);
        } else if (node->data.type == NUM)
            printf ("%lg\"];\n", node->data.number);
        else {
            printf (" \"]");
        }
        return 0;
    }

    int build_tree_dot (tree_diff::node_t *node, void *) {
        if (node->left != nullptr)
            printf ("\t%zu -> %zu [label = \"left\"]\n", (size_t) node, (size_t) node->left);

        if (node->right != nullptr)
            printf ("\t%zu -> %zu [label = \"right\"]\n", (size_t) node, (size_t) node->right);

        return 0;
    }

    inline void recreate_dot_dir () {
        system ("rm -rf TreeSnapshot/ 2> /dev/null");
        system ("mkdir TreeSnapshot");
        system ("touch TreeSnapshot/README.txt; echo This folder constatly deleted! > TreeSnapshot/README.txt");
    }

    bool fill_dot_file (tree_diff::node_t *head) {
        FILE *file_dot = fopen ("temp_file.dot", "w");
        if (file_dot == nullptr) {
            PRINT_ERROR ("DONT CREATE FILE_DOT")
            return false;
        }

        fprintf (file_dot, "digraph G {\n"
                           "    rankdir = TR;\n"
                           "    node[shape=ellipse, fontsize=50, color = red];\n"
                           "    edge[fontsize=50, color = blue, fillcolor = blue];\n");

        FILE *save_stdout = stdout;
        stdout = file_dot;                      // Крайне удобный приём для перенаправления потока

        head->visitor_pref_r (ident_dot);         // Идентифицирую каждый узел и связываю его с его содержимым

        fprintf (file_dot, "\n");

        head->visitor_pref_r (build_tree_dot);    // Строю дерево

        fprintf (file_dot, "}");

        fclose (file_dot);
        stdout = save_stdout;
        return true;
    }
}

int spec_printf (tree_diff::node_t *node, void *) {
    if (node->data.type == VAR) {
        printf ("%c:\n", 'x');
    } else if (node->data.type == OPER_or_FUNC) {
        printf ("%s:\n", _operators[(int) node->data.number]);
    } else if (node->data.type == NUM)
        printf ("%lg:\n", node->data.number);

    if (node->right != nullptr) {
        if (node->right->data.type == VAR) {
            printf ("\tright: %c\n", 'x');
        } else if (node->right->data.type == OPER_or_FUNC) {
            printf ("\tright: %s\n", _operators[(int) node->right->data.number]);
        } else if (node->right->data.type == NUM)
            printf ("\tright: %lg\n", node->right->data.number);
    }

    if (node->left != nullptr) {
        if (node->left->data.type == VAR) {
            printf ("\tleft: %c\n", 'x');
        } else if (node->left->data.type == OPER_or_FUNC) {
            printf ("\tleft: %s\n", _operators[(int) node->left->data.number]);
        } else if (node->left->data.type == NUM)
            printf ("\tleft: %lg\n", node->left->data.number);
    }
    return 0;
}

void Dump_Tree (FILE *file_out, tree_diff::node_t *head) {
    FILE *temp_stdout = stdout;
    stdout = file_out;
    printf ("\n\n");
    if (head->visitor_pref_r (spec_printf) < 0) {
        PRINT_ERROR ("Failed to visitor")
    }
    printf (R"(//////END\\\\\\)");
    stdout = temp_stdout;
}

void Dump_Tree_img (tree_diff::node_t *head) {
    static size_t number_calls = 0;
    static bool first = false;

    // Удаляю папку со старыми данными и создаю пустую новую для хранения результата (фото, видео, GIF)
    if (!first) {
        Dtdot::recreate_dot_dir ();
        first = true;
    }

    if (!Dtdot::fill_dot_file (head))
        return;

    char comand[256] = "";
    sprintf (comand, "dot -Tpng -Gsize=10,16\\! -Gdpi=150 temp_file.dot -o TreeSnapshot/%zu.png", number_calls);
    system (comand);

    number_calls++;
}






