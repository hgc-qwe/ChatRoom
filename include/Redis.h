#include <string>

class Redis{
public:
    bool connect();

    bool publish();

    bool subscribe();

    bool unsubscribe();
};