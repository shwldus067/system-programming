//function calls
void print_bp();
void clear_bp();
int find_bp(int bpaddr);
void set_bp(int bpaddr);
void operate(int opcode, int a1, int a2, int ni);
void init_register();
void run_f(int *loaded);
