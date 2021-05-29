// shrinquem - An algorithm for logic minimization.
// Copyright (C) 2021 Damon Bohls <damonbohls@gmail.com>
// MIT License: https://github.com/d-bohls/shrinquem/blob/main/LICENSE

#if !defined(INC_SHRINQUEM_H)
#define INC_SHRINQUEM_H

typedef char triLogic;

#define LOGIC_FALSE     (0)
#define LOGIC_TRUE      (1)
#define LOGIC_DONT_CARE (2)

enum shrinqStatus
{
    STATUS_OKAY = 0,
    STATUS_TOO_FEW_VARIABLES,
    STATUS_TOO_MANY_VARIABLES,
    STATUS_OUT_OF_MEMORY,
    STATUS_NULL_ARGUMENT,
};

extern enum shrinqStatus
ReduceLogic(
    const unsigned long numVars,
    const triLogic truthTable[],
    unsigned long *numTerms,
    unsigned long *terms[],
    unsigned long *dontCares[]);

extern enum shrinqStatus
GenerateEquation(
    const unsigned long numVars,
    const unsigned long numTerms,
    const unsigned long terms[],
    const unsigned long dontCares[],
    char *varNames[],
    char *equation[]);

extern triLogic
EvaluateSumOfProducts(
    const unsigned long numVars,
    const unsigned long numTerms,
    const unsigned long terms[],
    const unsigned long dontCares[],
    const unsigned long input);

#endif /* !defined(INC_SHRINQUEM_H) */
