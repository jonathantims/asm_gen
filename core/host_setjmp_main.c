/* Generate host_setjmp.asm. This process is linked before that object
 * exists, and host_fail names host_longjmp, so the two definitions below
 * only satisfy the link. They are not the setjmp the rest of the build runs. */

#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "host_setjmp.h"

int host_setjmp(void* buf) {
    (void)buf;
    return 0;
}

__declspec(noreturn) void host_longjmp(void* buf, int code) {
    (void)buf;
    (void)code;
    host_die(host_fail_msg);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    write_host_setjmp();
    return write_asm_file("host_setjmp.asm");
}
