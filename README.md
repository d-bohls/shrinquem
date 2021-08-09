# shrinquem
An algorithm for logic minimization.

## 1. An Example

Consider the following truth table with three Boolean inputs:

n  | A  | B  | C  | *f*(*A*, *B*, *C*)
-- | -- | -- | -- | --
0  | 0  | 0  | 0  | 1
1  | 0  | 0  | 1  | 1
2  | 0  | 1  | 0  | 0
3  | 0  | 1  | 1  | 1
4  | 1  | 0  | 0  | 1
5  | 1  | 0  | 1  | 0
6  | 1  | 1  | 0  | 0
7  | 1  | 1  | 1  | 0

Shrinquem can be used to find a minimum sum-of-products representation for this Boolean function as follows:

```C
#include <stdlib.h>
#include <stdio.h>
#include "shrinquem.h"

int main(int argc, char* argv[])
{
    const numVars = 3;
    const triLogic truthTable[8] = { 1,1,0,1,1,0,0,0 };
    SumOfProducts sumOfProducts = { numVars };
    ReduceLogic(truthTable, &sumOfProducts);
    GenerateEquationString(&sumOfProducts, NULL);
    printf("f(A, B, C) = %s", sumOfProducts.equation);
    FinalizeSumOfProducts(&sumOfProducts);

    return 0;
}
```

This code prints the following to standard out.

```
f(A, B, C) = A'C + B'C'
```

## 2. An Example with a "Don't Care"

Consider the following truth table with two Boolean inputs, where X indicates that we don't care what a particular output is:

n  | A  | B  | *f*(*A*, *B*)
-- | -- | -- | --
0  | 0  | 0  | 0
1  | 0  | 1  | 1
2  | 1  | 0  | 0
3  | 1  | 1  | X

Shrinquem can be used to generate the equation as follows:

```C
#include <stdlib.h>
#include <stdio.h>
#include "shrinquem.h"

int main(int argc, char* argv[])
{
    const numVars = 2;
    const triLogic truthTable[4] =
    {
        LOGIC_FALSE,
        LOGIC_TRUE,
        LOGIC_FALSE,
        LOGIC_DONT_CARE
    };
    SumOfProducts sumOfProducts = { numVars };
    ReduceLogic(truthTable, &sumOfProducts);
    GenerateEquationString(&sumOfProducts, NULL);
    printf("f(A, B) = %s", sumOfProducts.equation);
    FinalizeSumOfProducts(&sumOfProducts);

    return 0;
}
```

This code outputs the following equation:

```
f(A, B) = B
```
