#include <stdio.h>
#include "manager.h"

int main(int argc, char *argv[])
{
    manager_init(argc, argv);
    manager_run();
    return 0;
}
