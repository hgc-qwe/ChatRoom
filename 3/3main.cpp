#include "3server.h"

int main()
{
    Server server(8888);
    server.Start();
    return 0;
}