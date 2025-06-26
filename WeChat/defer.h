#ifndef DEFER_H
#define DEFER_H

#include <functional>

class Defer{
private:
    std::function<void()> _func;
public:
    Defer(std::function<void()> func);
    ~Defer();
};



#endif // DEFER_H



/// 把头像 icon 统一改为 QString










