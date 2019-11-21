#include "TreeDiff.h"

int main () {
    Tree tree, diff_tree, diff_tree1;

    char name_TeX_file[] = "MyLaTeX.tex";
    char string_in[] = "(x)^(x)";

    if (tree.read_expression (string_in)) {
        perror ("Failed to read tree from file");
        return 0;
    }

    Dump_Tree_img (tree.node);

    if (!Tree::Optimizer (tree.node)) {
        perror ("Error optimizer");
        return 0;
    }

    Dump_Tree_img (tree.node);

    if (Tree::get_Diff (diff_tree.node, tree.node, 4) == nullptr) {
        //PRINT_ERROR ("Error copy")
        perror ("Error copy");
        return 0;
    }
    
    Dump_Tree_img (diff_tree.node);

    Dump_Tree_Tex (diff_tree.node, name_TeX_file);
    return 0;
}



