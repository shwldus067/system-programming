//function calls
void free_symtab();
void free_lists();
struct sym* find_symbol(char *symb);
void symbol_update(char *symb, int addr);
void modification_update(struct lst *list);
int pass_1(char *file);
int pass_2(void);
int make_listing(char *file);
int make_obj(char *file);
int assemble_f(char *file, struct sym **symbol_top);
void symbol_f(struct sym *symbol_top);
