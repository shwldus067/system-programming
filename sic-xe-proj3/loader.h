//function calls
struct est* find_estab(char *symb);
void estab_update(struct est *new_est);
void print_estab();
int loader_f(char (*file)[40], int cnt, int addr);
int pass1(char *file, int *csaddr);
int pass2(char *file, int *csaddr);
