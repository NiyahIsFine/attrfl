#include "BaseTypeRegister.h"
#include "Generate.h"

int main(int argc, char **argv)
{
    if (HasFlag(argc, argv, "--generate"))
    {
        if (HasFlag(argc, argv, "--debug"))
            return Generate::GenerateDebug(argc, argv);
        return Generate::Generate(argc, argv);
    }

    if (HasFlag(argc, argv, "--baseType_register"))
    {
        return BaseTypeRegister::BaseTypeRegister(argc, argv);
    }
}
