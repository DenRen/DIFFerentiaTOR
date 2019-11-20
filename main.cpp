#include <cstdio>
#include <MyLib2.h>
#include <cmath>

#define MAX_LEN_OPER 6

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
        int visitor_pref (int (*func) (nodeTree *, void *), void *aux_arg = nullptr);
    } node_t;

    // Визитор для строк, так как делает проверку на nullptr в data
    // Справа налево
    // Нет проверки на существование данных в узле !!!
    int nodeTree::visitor_pref (int (*func) (nodeTree *, void *), void *aux_arg) {
        int state = 0;

        if ((state = func (this, aux_arg)) != 0)
            return state;

        if (right != nullptr && (state = right->visitor_pref (func, aux_arg)) != 0)
            return state;

        if (left != nullptr && (state = left->visitor_pref (func, aux_arg)) != 0)
            return state;


        return 0;
    }

    // Дерево саморасширяемо, но при этом не уменьшает свои размеры обратно
    typedef struct Tree {
        node_t *node;       // Указатель на корень
        size_t count;       // Количесво узлов

        double exp = 1.5;   // При переполнении дерева, массив увеличится во столько раз
        unsigned step = 0;  // Если после конструктора step > 0, то вместо exp используется шаг step

        void *base_data = nullptr;

        Tree (size_t count, unsigned step);

        int verificator ();

        inline node_t *add_right (node_t *in_node, data_t value);

        inline node_t *add_left (node_t *in_node, data_t value);

        int read_expression (char *source);

        int save_to_file (char *name_file);

        tree_diff::node_t *copy (tree_diff::node_t *from);

        node_t *crtNode (int type, number_t value);

        node_t *get_Diff (node_t *to_node, const node_t *root_node);

        node_t *Copy (const node_t *curr_node);
    private:
        node_t *add (int right_or_left, node_t *curr_node, data_t value);

        node_t *empty_node; // Указатель на свободный элемент узла
        node_t *get_new_node ();

        size_t min_count ();

        node_t *Diff (const node_t *curr_node);
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
    Tree::Tree (size_t _count, unsigned _step = 0) {
        if (_count < min_count () && _step <= 0) _count = min_count ();

        if ((node = (node_t *) calloc (_count, sizeof (node_t))) == nullptr) {
            PRINT_ERROR ("Constructor error: failed to create node array")
            return;
        }
        count = _count;
        empty_node = node + 1;
        node->left = node->right = node->prev = nullptr;
        step = _step;
        assert (verificator ());
    }

    int Tree::verificator () {
        assert (exp >= 0);
        assert (exp || step);
        assert (exp || step > 0);
        assert (count > 0);
        assert (node != nullptr);
        assert (node->prev == nullptr);   // Предком корня может быть только nullptr

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
        assert (verificator ());
        if (empty_node == node + count) {   // Если дерево переполнилось, то расширяем его
            typeof (count) new_size = count;
            node_t *new_tree_root = nullptr;

            if (step > 0) {                 // При задании нового размера, проверяется корректность
                new_size += step;           // критически важных параметров дерева
            } else if (exp > 1) {
                new_size = (size_t) ((double) new_size * exp);
            } else {
                errno = EINVAL;
                PRINT_ERROR ("Tree: CRITICAL ERROR: exp < 1")
                return nullptr;
            }

            if ((new_tree_root = (node_t *) realloc (node, new_size * sizeof (node_t))) == nullptr) {
                PRINT_ERROR ("Tree: CRITICAL ERROR: failed to realloc tree")
                return nullptr;
            }

            node = new_tree_root;
            empty_node = new_tree_root + count;
            count = new_size;
        }

        return empty_node++;
    }

    inline size_t Tree::min_count () {
        return floor (1 / (exp - 1) + 1);
    }

    int save_tree_to_file_vis (tree_diff::node_t *curr_node, void *file) {/*
        fprintf ((FILE *) file, "{ \"%s\" ", curr_node->data);
        fflush ((FILE *) file);
        if (curr_node->right != nullptr) {
            if (curr_node->right->visitor_pref (save_tree_to_file_vis, file) < 0)
                return -1;
        }
        if (curr_node->left != nullptr) {
            if (curr_node->left->visitor_pref (save_tree_to_file_vis, file) < 0)
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
        if (node->visitor_pref (save_tree_to_file_vis, stdout) < 0) {
            PRINT_ERROR ("Error save to file")
            return 0;
        }
        return 0;
    }
}

void Dump_Tree (FILE *file_out, tree_diff::node_t *head);

void Dump_Tree_img (tree_diff::node_t *head);

enum _data_type {
    NUM = 1,
    VAR, OPER_or_FUNC
};

enum operators_number {
    ADD, SUB, MUL, DIV,
    POW, LOG, LN, LG,
    SIN, COS,
    TAN, CTAN, TG, CTG,
    SH, CH, TH, CTH,
    SINH, COSH, TANH, CTANH,
    EXP, NUM_OPER
};

enum separators {
    // При их изменении необходимо также изменить sscanf в tree_diff::Tree::read_expression
            LSEP = '(', RSEP = ')'
};

char _operators[NUM_OPER][MAX_LEN_OPER] = {
        "+", "-", "*", "/",
        "^", "log", "ln", "lg",
        "sin", "cos",
        "tan", "ctan", "tg", "ctg",
        "sh", "ch", "th", "cth",
        "sinh", "cosh", "tanh", "ctanh",
        "exp"
};

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
            } else {
                oper_on--;
            }
        } else if (*source == RSEP) {
            brk_count--;
            curr_node = curr_node->prev;
        } else {
            // Считываем либо число, либо оператор, либо функцию
            if (sscanf (source, "%n%[^() ]%n", &n_left, str, &n_right) == 0) {
                PRINT_ERROR ("ERROR READ")
                return tree_diff::EREAD;
            }
            printf ("THIS: %s\n", str);
            source += n_right - n_left - 1;
            if ((temp_num = get_ind_oper (str)) != (number_t) -1) {         // Это оператор или функция?
                curr_node->data.type = OPER_or_FUNC;
                curr_node->data.number = temp_num;
                oper_on++;
                if ((curr_node = add_right (curr_node, {0})) == nullptr)
                    return tree_diff::EADDLEFT;

            } else if (*str == 'x') {                                       // Это аргумент?
                curr_node->data.type = VAR;
                curr_node->data.number = 0;
            } else if (sscanf (str, "%lg", &temp_num)) {                    // Это число?
                curr_node->data.type = NUM;
                curr_node->data.number = temp_num;
            } else {                                                        // Что это вообще?
                PRINT_ERROR ("ERROR READ")
                printf ("\nstr: %s\n", str);
                return tree_diff::EUNIDENT;
            }
        }
        Dump_Tree (stdout, this->node);
    }
    return 0;
}

#define CreateNode node_t = get_new_no

#define CREATE_NODE_OPERATOR(left_expr, right_expr, _OPER) ({\
    node_t* __node = crtNode (OPER_or_FUNC, _OPER); \
    __node->left = (left_expr); \
    __node->right = (right_expr); \
    __node; })

#define _PLUS(left_expr, right_expr) CREATE_NODE_OPERATOR (left_expr, right_expr, ADD)
#define _SUB(left_expr, right_expr)  CREATE_NODE_OPERATOR (left_expr, right_expr, SUB)
#define _MUL(left_expr, right_expr)  CREATE_NODE_OPERATOR (left_expr, right_expr, MUL)
#define _DIV(left_expr, right_expr)  CREATE_NODE_OPERATOR (left_expr, right_expr, DIV)

#define L curr_node->left
#define R curr_node->right

#define dL Diff (curr_node->left)
#define dR Diff (curr_node->right)

tree_diff::node_t *tree_diff::Tree::Diff (const tree_diff::node_t *curr_node) {

    switch (curr_node->data.type) {
        case NUM:
            return crtNode (NUM, 0);
        case VAR:
            return crtNode (NUM, 1);
        case OPER_or_FUNC:
            switch ((int) curr_node->data.number) {
                case ADD:
                    return _PLUS (dL, dR);
                case SUB:
                    return _SUB (dL, dR);
                case MUL:
                    return _PLUS (_MUL (dL, R), _MUL (L, dR));
                default:
                    return nullptr;
            }
    }
    return nullptr;
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
        if ((new_node->left = Copy(curr_node->left)) == nullptr) {
            PRINT_ERROR ("Error copy left tree")
            return nullptr;
        }
    if (curr_node->right != nullptr)
        if ((new_node->right = Copy(curr_node->right)) == nullptr) {
            PRINT_ERROR ("Error copy right tree")
            return nullptr;
        }
    return new_node;
}

tree_diff::node_t *tree_diff::Tree::crtNode (int type, number_t value) {
    node_t *new_node = nullptr;
    if ((new_node = get_new_node ()) == nullptr) {
        PRINT_ERROR ("ERROR IN DIFF")
        return nullptr;
    }
    new_node->data.type = type;
    new_node->data.number = value;
    return new_node;
}

tree_diff::node_t *tree_diff::Tree::get_Diff (node_t *to_node, const tree_diff::node_t *root_node) {
    *to_node = *Diff (root_node);
    return to_node;
}

int main () {
    tree_diff::Tree tree (50), diff_tree (150);

    char string_in[] = "(x)+(((4) + (x))-(x))";

    if (tree.read_expression (string_in) || diff_tree.read_expression (string_in)) {
        PRINT_ERROR ("Failed to read tree from file")
        return 0;
    }

    Dump_Tree_img (tree.node);

    if (diff_tree.get_Diff (diff_tree.node, tree.node) == nullptr) {
        PRINT_ERROR ("Error copy")
        return 0;
    }
    int t = 0;
    printf ("%d", (t = 0, t+= 2, t));
    Dump_Tree_img (diff_tree.node);
    Dump_Tree_img (diff_tree.Copy(diff_tree.node));
    return 0;
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

        head->visitor_pref (ident_dot);         // Идентифицирую каждый узел и связываю его с его содержимым

        fprintf (file_dot, "\n");

        head->visitor_pref (build_tree_dot);    // Строю дерево

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
    if (head->visitor_pref (spec_printf) < 0) {
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






