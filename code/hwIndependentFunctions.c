#include <hwIndependentFunctions.h>

uint32_t checkHexFormat(const char *str, uint32_t length)
{
    uint32_t err_count = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        if (isxdigit(str[i]) == 0)
        {
            err_count++;
        }
    }
    return err_count;
}

uint32_t checkRange(uint32_t iNumber, uint32_t iRange)
{
    uint32_t err_count = 0;
    if (iNumber <= iRange)
    {
        err_count = 0;
    }
    else
    {
        err_count++;
    }
    return err_count;
}

uint32_t checkParity(uint32_t iNumber)
{
    uint32_t err_count = 0;
    if (iNumber % 2 == 0)
    {
        err_count = 0;
    }
    else
    {
        err_count++;
    }
    return err_count;
}
