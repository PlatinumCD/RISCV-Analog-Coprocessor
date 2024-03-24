
int main() {
    __asm__ __volatile__ ("mvm.set x0, x0, x0");
    __asm__ __volatile__ ("mvm.l   x0, x0, x0");
    __asm__ __volatile__ ("mvm     x0, x0, x0");
    __asm__ __volatile__ ("mvm.s   x0, x0, x0");
    __asm__ __volatile__ ("mvm.mv  x0, x0, x0");
}
