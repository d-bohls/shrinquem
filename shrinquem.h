// shrinquem - An algorithm for logic minimization.
// Copyright (C) 2021 Damon Bohls <damonbohls@gmail.com>
// MIT License: https://github.com/d-bohls/shrinquem/blob/main/LICENSE

#if !defined(INC_SHRINQUEM_H)
#define INC_SHRINQUEM_H

typedef char triLogic;

#define LOGIC_FALSE     (0)
#define LOGIC_TRUE      (1)
#define LOGIC_DONT_CARE (2)

typedef enum
{
    STATUS_OKAY = 0,
    STATUS_TOO_FEW_VARIABLES,
    STATUS_TOO_MANY_VARIABLES,
    STATUS_OUT_OF_MEMORY,
    STATUS_NULL_ARGUMENT,
} shrinquemStatus;

typedef struct SumOfProducts
{
    unsigned long numVars;
    unsigned long numTerms;
    unsigned long* terms;
    unsigned long* dontCares;
    char* equation;
} SumOfProducts;

void FinalizeSumOfProducts(
    SumOfProducts* sumOfProducts);

shrinquemStatus ReduceLogic(
    const triLogic truthTable[],
    SumOfProducts* sumOfProducts);

shrinquemStatus GenerateEquationString(
    SumOfProducts* sumOfProducts,
    const char** const varNames);

triLogic EvaluateSumOfProducts(
    const SumOfProducts sumOfProducts,
    const unsigned long input);

// functions used for metrics and testing
void ResetTermCounters();
unsigned long GetNumTermsKept();
unsigned long GetNumTermsRemoved();

#endif // !defined(INC_SHRINQUEM_H)
