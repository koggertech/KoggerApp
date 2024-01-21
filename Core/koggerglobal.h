#ifndef KOGGERGLOBAL_H
#define KOGGERGLOBAL_H

#define early_return(cond, ...)       \
    do {                              \
        if (static_cast<bool>(cond)){ \
            return __VA_ARGS__;       \
    }                                 \
    } while (0)                       \

#endif // KOGGERGLOBAL_H
