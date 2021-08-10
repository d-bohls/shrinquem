// shrinquem - An algorithm for logic minimization.
// Copyright (C) 2021 Damon Bohls <damonbohls@gmail.com>
// MIT License: https://github.com/d-bohls/shrinquem/blob/main/LICENSE

#include <stdlib.h>
#include <string.h> // used for strlen
#include "shrinquem.h"

#define BITS_PER_BYTE (8)

static const unsigned long MAX_NUM_VARIABLES = sizeof(long) * BITS_PER_BYTE;

static unsigned long numTermsKept = 0;
static unsigned long numTermsRemoved = 0;

static unsigned long EstimateMaxNumOfMinterms(
    const unsigned long numVars,
    const triLogic truthTable[]);

static void RemoveNonprimeImplicants(
    SumOfProducts* sumOfProducts);

void FinalizeSumOfProducts(SumOfProducts* sumOfProducts)
{
    sumOfProducts->numVars = 0;
    sumOfProducts->numTerms = 0;

    if (sumOfProducts->terms)
    {
        free(sumOfProducts->terms);
        sumOfProducts->terms = NULL;
    }

    if (sumOfProducts->dontCares)
    {
        free(sumOfProducts->dontCares);
        sumOfProducts->dontCares = NULL;
    }

    if (sumOfProducts->equation)
    {
        free(sumOfProducts->equation);
        sumOfProducts->equation = NULL;
    }
}

/*************************************************************************
ReduceLogic
Purpose - generates an array representation of the minimum sum-of-products equation

Data is formatted as follows:

Say there are three variables. The input variable numVars will be 3. The input
variable truthTable is an array of the outputs of the logic equation.

             Var1  Var2  Var3  | Output
           ====================|=============
             0     0     0     | truthTable[0]
             0     0     1     | truthTable[1]
             0     1     0     | truthTable[2]
             0     1     1     | truthTable[3]
             1     0     0     | truthTable[4]
             1     0     1     | truthTable[5]
             1     1     0     | truthTable[6]
             1     1     1     | truthTable[7]

Therefore, the truthTable array should have 2^3 = 8 elements.  Now let's
take the following truth table as an example.

             Var1  Var2  Var3  | Output
           ====================|=============
             0     0     0     | 0
             0     0     1     | 0
             0     1     0     | 0
             0     1     1     | 1
             1     0     0     | 1
             1     0     1     | 1
             1     1     0     | 0
             1     1     1     | 1

For this truth table,

numVars = 3
truthTable[8] = {0,0,0,1,1,1,0,1}

After logic reduction, the truth table can be reduced to the following equation.

Var1 * Var2' + Var2 * Var3

This equation has two terms:

Term 1 is Var1 * Var2'
Term 2 is Var2 * Var3

Therefore, the output variable numTerms will be 2. The two output arrays (terms and dontCares)
will each have 2 elements; one for each term. They will be as follows

numTerms = 2

For term 1...

dontCares[0] = b001 = 1   <~~ Var3 (bit position 0) is a "don't care"
terms[0]     = b100 = 4   <~~ Var1 (bit position 2) must be true and Var2 (bit position 1) must be false

For term 2...

dontCares[1] = b100 = 4   <~~ Var1 (bit position 2) is a "don't care"
terms[1]     = b011 = 3   <~~ Var2 (bit position 1) and Var3 (bit position 0) both must be true

*************************************************************************/

shrinquemStatus ReduceLogic(
    const triLogic truthTable[],
    SumOfProducts* sumOfProducts)
{
    shrinquemStatus status = STATUS_OKAY;
    triLogic* resolved = NULL;

    // initialize and allocate

    if (truthTable == NULL)
    {
        status = STATUS_NULL_ARGUMENT;
        goto cleanupAndExit;
    }
    else if (sumOfProducts->numVars < 1)
    {
        status = STATUS_TOO_FEW_VARIABLES;
        goto cleanupAndExit;
    }
    else if (sumOfProducts->numVars > MAX_NUM_VARIABLES)
    {
        status = STATUS_TOO_MANY_VARIABLES;
        goto cleanupAndExit;
    }

    unsigned long sizeTruthtable = 1 << sumOfProducts->numVars;
    unsigned long maxNumOfMinterms = EstimateMaxNumOfMinterms(sumOfProducts->numVars, truthTable);
    sumOfProducts->numTerms = 0;
    sumOfProducts->terms = NULL; // the caller should not have allocated any memory
    sumOfProducts->dontCares = NULL; // the caller should not have allocated any memory
    resolved = calloc(sizeTruthtable, sizeof(triLogic));
    sumOfProducts->terms = malloc(maxNumOfMinterms * sizeof(long));
    sumOfProducts->dontCares = malloc(maxNumOfMinterms * sizeof(long));

    if (resolved == NULL || sumOfProducts->terms == NULL || sumOfProducts->dontCares == NULL)
    {
        status = STATUS_OUT_OF_MEMORY;
        goto cleanupAndExit;
    }

    // loop through each entry in the truth table and derive the terms for the reduced logic
    for (unsigned long iInput = 0; iInput < sizeTruthtable; iInput++)
    {
        if ((truthTable[iInput] == LOGIC_TRUE) && !resolved[iInput])
        {
            unsigned long iTerm = sumOfProducts->numTerms;
            sumOfProducts->numTerms++;
            sumOfProducts->terms[iTerm] = iInput; // the term starts out equal to the minterm 
            sumOfProducts->dontCares[iTerm] = 0;  // initially there are no "don't cares"

            // loop through each bit to see if it can be replaced by a "don't care"
            for (unsigned long iBitTest = 0; iBitTest < sumOfProducts->numVars; iBitTest++)
            {
                unsigned long bitMaskTest = 1 << iBitTest;
                sumOfProducts->terms[iTerm] ^= bitMaskTest;
                // test all minterms associated with the term by checking all the "don't care" combinations
                // start by clearing all "don't care" bits
                sumOfProducts->terms[iTerm] &= ~(sumOfProducts->dontCares[iTerm]);

                while (1)
                {
                    if (truthTable[sumOfProducts->terms[iTerm]] == LOGIC_FALSE)
                    {
                        // we can't replace this variable with a don't care, so flip the bit back and exit
                        sumOfProducts->terms[iTerm] ^= bitMaskTest;
                        break;
                    }

                    // get the next minterm to test
                    unsigned long iBitDC;
                    for (iBitDC = 0; iBitDC < iBitTest; iBitDC++)
                    {
                        unsigned long bitMaskDC = 1 << iBitDC;
                        if (sumOfProducts->dontCares[iTerm] & bitMaskDC)
                        {
                            if (sumOfProducts->terms[iTerm] & bitMaskDC)
                            {
                                sumOfProducts->terms[iTerm] &= ~bitMaskDC;
                            }
                            else
                            {
                                sumOfProducts->terms[iTerm] |= bitMaskDC;
                                break;
                            }
                        }
                    }

                    // check to see if this bit/variable is a "don't care"
                    if (iBitDC == iBitTest)
                    {
                        sumOfProducts->dontCares[iTerm] |= bitMaskTest;
                        break;
                    }
                }
            }

            // At this point, we have expanded the term to cover as many minterms as possible.
            // Now go through all minterms associated with this term to mark them as resolved.
            // Start by clearing all "don't care" bits.
            sumOfProducts->terms[iTerm] &= ~(sumOfProducts->dontCares[iTerm]);

            while (1)
            {
                resolved[sumOfProducts->terms[iTerm]] = LOGIC_TRUE;

                // get the next minterm to set as resolved
                unsigned long iBitDC;
                for (iBitDC = 0; iBitDC < sumOfProducts->numVars; iBitDC++)
                {
                    unsigned long bitMaskDC = 1 << iBitDC;
                    if (sumOfProducts->dontCares[iTerm] & bitMaskDC)
                    {
                        if (sumOfProducts->terms[iTerm] & bitMaskDC)
                        {
                            sumOfProducts->terms[iTerm] &= ~bitMaskDC;
                        }
                        else
                        {
                            sumOfProducts->terms[iTerm] |= bitMaskDC;
                            break;
                        }
                    }
                }

                if (iBitDC == sumOfProducts->numVars)
                {
                    break;
                }
            }
        }
    }

cleanupAndExit:

    if (resolved)
    {
        free(resolved);
        resolved = NULL;
    }

    if (status == STATUS_OKAY)
    {
        // we're just making these buffers smaller so it should never fail, but ignore the case that it does
        void* p;
        p = realloc(sumOfProducts->terms, sumOfProducts->numTerms * sizeof(long));
        sumOfProducts->terms = (p || sumOfProducts->numTerms == 0) ? p : sumOfProducts->terms;
        p = realloc(sumOfProducts->dontCares, sumOfProducts->numTerms * sizeof(long));
        sumOfProducts->dontCares = (p || sumOfProducts->numTerms == 0) ? p : sumOfProducts->dontCares;

        RemoveNonprimeImplicants(sumOfProducts);
    }

    if (status != STATUS_OKAY)
    {
        if (sumOfProducts->numTerms)
            sumOfProducts->numTerms = 0;

        if (sumOfProducts->terms)
        {
            free(sumOfProducts->terms);
            sumOfProducts->terms = NULL;
        }

        if (sumOfProducts->dontCares)
        {
            free(sumOfProducts->dontCares);
            sumOfProducts->dontCares = NULL;
        }
    }

    return status;
}

/*************************************************************************
GenerateEquationString
Purpose - generates a null-terminated string representation of the
  minimum sum-of-products equation generated by the ReduceLogic function
*************************************************************************/

shrinquemStatus GenerateEquationString(
    SumOfProducts* sumOfProducts,
    const char** const varNames)
{
    shrinquemStatus retVal = STATUS_OKAY;
    unsigned long done = 0;
    unsigned long iVar;
    unsigned long iTerm;
    unsigned long iEquPos;
    unsigned long iTermPos;
    unsigned long iCharPos;
    unsigned long bitMask;
    size_t outputSize;
    char** varNamesAuto = NULL;
    const char** varNamesToUse = NULL;
    size_t* varNameSizes = NULL;

    // the caller should not have allocated any memory for the equation
    sumOfProducts->equation = NULL;

    // first check for the case where the equation is '0'
    if (sumOfProducts->numTerms <= 0)
    {
        sumOfProducts->equation = (char*)malloc(2 * sizeof(char));
        if (sumOfProducts->equation == NULL)
            return STATUS_OUT_OF_MEMORY;

        sumOfProducts->equation[0] = '0';
        sumOfProducts->equation[1] = 0;
        done = 1;
    }
    else
    {

        // now check for a '1' (one term with all don't cares)
        if (sumOfProducts->numTerms == 1)
        {
            for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
            {
                bitMask = 1 << iVar;
                if ((sumOfProducts->dontCares[0] & bitMask) == 0)
                {
                    break;
                }
            }

            if (iVar == sumOfProducts->numVars)
            {
                sumOfProducts->equation = (char*)malloc(2 * sizeof(char));
                if (sumOfProducts->equation == NULL)
                    return STATUS_OUT_OF_MEMORY;
                sumOfProducts->equation[0] = '1';
                sumOfProducts->equation[1] = 0;
                done = 1;
            }
        }

        // if the equation isn't '0' or '1' then build the string
        if (done == 0)
        {

            // auto-name variables if names were not provided
            if (varNames != NULL)
            {
                varNamesToUse = varNames;
            }
            else
            {
                varNamesAuto = (char**)malloc(sumOfProducts->numVars * sizeof(char*));
                varNamesToUse = varNamesAuto;
                if (varNamesAuto == NULL)
                    return STATUS_OUT_OF_MEMORY;

                for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
                {
                    varNamesAuto[iVar] = NULL;
                }

                for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
                {
                    if (varNamesAuto[iVar] == NULL)
                    {
                        varNamesAuto[iVar] = (char*)malloc(2 * sizeof(char));
                        if (varNamesAuto[iVar] == 0)
                            return STATUS_OUT_OF_MEMORY;
                        (varNamesAuto[iVar])[0] = 'A' + (char)iVar;
                        (varNamesAuto[iVar])[1] = 0;
                    }
                }
            }

            // first calculate the size of the string names for the variables
            varNameSizes = (size_t*)malloc(sizeof(long*) * sumOfProducts->numVars);
            if (varNameSizes == NULL)
                return STATUS_OUT_OF_MEMORY;

            for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
            {
                varNameSizes[iVar] = strlen(varNamesToUse[iVar]);
            }

            // now run through the equation to determine the length of the output string
            iTermPos = 0;
            iEquPos = 0;
            outputSize = 0;
            for (iTerm = 0; iTerm < sumOfProducts->numTerms; iTerm++)
            {
                for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
                {
                    bitMask = 1 << (sumOfProducts->numVars - iVar - 1);
                    if ((sumOfProducts->dontCares[iTerm] & bitMask) == 0)
                    {
                        outputSize += varNameSizes[iVar];
                        if ((sumOfProducts->terms[iTerm] & bitMask) == 0)
                        {
                            outputSize++;
                        }
                    }

                    iTermPos++;
                }

                if (iTerm < (sumOfProducts->numTerms - 1))
                {
                    outputSize += 3; // account for the " + "
                }
                else
                {
                    outputSize++; // account for the null terminator
                }
            }

            // allocate space for the equation
            sumOfProducts->equation = (char*)malloc(outputSize * sizeof(char*));
            if (sumOfProducts->equation == NULL)
                return STATUS_OUT_OF_MEMORY;

            // use termTable and numTerms to generate equation
            iTermPos = iEquPos = 0;
            for (iTerm = 0; iTerm < sumOfProducts->numTerms; iTerm++)
            {
                for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
                {
                    bitMask = 1 << (sumOfProducts->numVars - iVar - 1);
                    if ((sumOfProducts->dontCares[iTerm] & bitMask) == 0)
                    {
                        // copy the variable name
                        for (iCharPos = 0; iCharPos < varNameSizes[iVar]; iCharPos++)
                        {
                            sumOfProducts->equation[iEquPos++] = (varNamesToUse[iVar])[iCharPos];
                        }

                        // place the complement sign of false
                        if ((sumOfProducts->terms[iTerm] & bitMask) == 0)
                        {
                            sumOfProducts->equation[iEquPos++] = '\'';
                        }
                    }
                    iTermPos++;
                }

                if (iTerm < (sumOfProducts->numTerms - 1))
                {
                    sumOfProducts->equation[iEquPos++] = ' ';
                    sumOfProducts->equation[iEquPos++] = '+';
                    sumOfProducts->equation[iEquPos++] = ' ';
                }
                else
                {
                    sumOfProducts->equation[iEquPos++] = 0;
                }
            }

            done = 1;
        }
    }

    if (varNamesAuto != NULL)
    {
        for (iVar = 0; iVar < sumOfProducts->numVars; iVar++)
        {
            if (varNamesAuto[iVar] != NULL)
            {
                free(varNamesAuto[iVar]);
                varNamesAuto[iVar] = NULL;
            }
        }

        if (varNamesAuto)
        {
            free(varNamesAuto);
            varNamesAuto = NULL;
        }
    }

    if (varNameSizes)
    {
        free(varNameSizes);
        varNameSizes = NULL;
    }

    return retVal;
}

/*************************************************************************
EvaluateSumOfProducts
Purpose - evaluates the sum-of-products produced by ReduceLogic given
          a certain set of boolean inputs.
*************************************************************************/

triLogic EvaluateSumOfProducts(
    const SumOfProducts sumOfProducts,
    const unsigned long input)
{
    // clear out bits that might be set in the input which are beyond the number of variables we are evaluating
    const unsigned long inputMask = (1 << sumOfProducts.numVars) - 1;
    unsigned long constrainedInput = input & inputMask;

    for (unsigned long iTerm = 0; iTerm < sumOfProducts.numTerms; iTerm++)
    {
        // one TRUE product term makes the whole sum-of-products TRUE
        if ((constrainedInput | sumOfProducts.dontCares[iTerm]) == (sumOfProducts.terms[iTerm] | sumOfProducts.dontCares[iTerm]))
            return LOGIC_TRUE;
    }

    // no product terms were true, so the whole equation is false
    return LOGIC_FALSE;
}

void ResetTermCounters()
{
    numTermsKept = 0;
    numTermsRemoved = 0;
}

unsigned long GetNumTermsKept()
{
    return numTermsKept;
}

unsigned long GetNumTermsRemoved()
{
    return numTermsRemoved;
}


static unsigned long EstimateMaxNumOfMinterms(
    const unsigned long numVars,
    const triLogic truthTable[])
{
    // the maximum possible number of minterms is when the truth table has alternating zeros and ones, like a checkerboard.
    unsigned long sizeTruthtable = 1 << numVars;
    unsigned long maximumPossibleNumOfMinterms = sizeTruthtable / 2;

    // We know the final equation will have less than or equal to the non-zero minterms in the truth table.
    // Count them up so we can see if this is less.
    unsigned long numTrueMinterms = 0;
    for (unsigned long iInput = 0; iInput < sizeTruthtable; iInput++)
    {
        if (truthTable[iInput] == LOGIC_TRUE)
        {
            numTrueMinterms++;
        }
    }

    return min(maximumPossibleNumOfMinterms, numTrueMinterms);
}

/*************************************************************************
RemoveNonprimeImplicants
Purpose - removes terms which are non-prime implicants.
*************************************************************************/

static void RemoveNonprimeImplicants(
    SumOfProducts* sumOfProducts)
{
    unsigned long* refCntTable;
    unsigned long sizeTruthtable;
    unsigned long numOldTerms;
    unsigned long iOldTerm;
    unsigned long iNewTerm;
    unsigned long bitMaskDC;
    char isPrime;

    numOldTerms = sumOfProducts->numTerms;

    sizeTruthtable = 1 << sumOfProducts->numVars; // the truth table has 2^numVars elements

    refCntTable = (long*)calloc(sizeTruthtable, sizeof(long));

    // loop through each term and ref count the minterms that the term covers
    for (iOldTerm = 0; iOldTerm < numOldTerms; iOldTerm++)
    {
        // start by clearing all the don't care bits
        sumOfProducts->terms[iOldTerm] &= ~(sumOfProducts->dontCares[iOldTerm]);

        while (1)
        {
            refCntTable[sumOfProducts->terms[iOldTerm]]++;

            // get the next minterm to ref count
            unsigned long iBitDC;
            for (iBitDC = 0; iBitDC < sumOfProducts->numVars; iBitDC++)
            {
                bitMaskDC = 1 << iBitDC;
                if (sumOfProducts->dontCares[iOldTerm] & bitMaskDC)
                {
                    if (sumOfProducts->terms[iOldTerm] & bitMaskDC)
                    {
                        sumOfProducts->terms[iOldTerm] &= ~bitMaskDC;
                    }
                    else
                    {
                        sumOfProducts->terms[iOldTerm] |= bitMaskDC;
                        break;
                    }
                }
            }
            if (iBitDC == sumOfProducts->numVars)
            {
                break;
            }
        }
    }

    // now loop through each term again and remove terms if all its minterms are ref counted more than once
    for (iNewTerm = iOldTerm = 0; iOldTerm < numOldTerms; iOldTerm++)
    {
        isPrime = 0;
        sumOfProducts->terms[iOldTerm] &= ~(sumOfProducts->dontCares[iOldTerm]); // clear all the don't care bits

        while (1)
        {
            // exit early if minterm is ref counted once, this term is a prime implicant and we will keep it
            if (refCntTable[sumOfProducts->terms[iOldTerm]] == 1)
            {
                isPrime = 1;
                break;
            }

            // get the next minterm to ref count
            unsigned long iBitDC;
            for (iBitDC = 0; iBitDC < sumOfProducts->numVars; iBitDC++)
            {
                bitMaskDC = 1 << iBitDC;
                if (sumOfProducts->dontCares[iOldTerm] & bitMaskDC)
                {
                    if (sumOfProducts->terms[iOldTerm] & bitMaskDC)
                    {
                        sumOfProducts->terms[iOldTerm] &= ~bitMaskDC;
                    }
                    else
                    {
                        sumOfProducts->terms[iOldTerm] |= bitMaskDC;
                        break;
                    }
                }
            }
            if (iBitDC == sumOfProducts->numVars)
            {
                break;
            }
        }

        if (isPrime)
        {
            // keep this term, copy it down to the next spot
            if (iOldTerm != iNewTerm)
            {
                sumOfProducts->terms[iNewTerm] = sumOfProducts->terms[iOldTerm];
                sumOfProducts->dontCares[iNewTerm] = sumOfProducts->dontCares[iOldTerm];
            }
            iNewTerm++;
            numTermsKept++;
        }
        else
        {
            // this term is a non-prime implicant and will not be kept
            sumOfProducts->numTerms--;
            numTermsRemoved++;

            // de-ref count this term's minterms
            sumOfProducts->terms[iOldTerm] &= ~(sumOfProducts->dontCares[iOldTerm]);
            while (1)
            {
                refCntTable[sumOfProducts->terms[iOldTerm]]--;

                // get the next minterm the needs its ref count decremented
                unsigned long iBitDC;
                for (iBitDC = 0; iBitDC < sumOfProducts->numVars; iBitDC++)
                {
                    bitMaskDC = 1 << iBitDC;
                    if (sumOfProducts->dontCares[iOldTerm] & bitMaskDC)
                    {
                        if (sumOfProducts->terms[iOldTerm] & bitMaskDC)
                        {
                            sumOfProducts->terms[iOldTerm] &= ~bitMaskDC;
                        }
                        else
                        {
                            sumOfProducts->terms[iOldTerm] |= bitMaskDC;
                            break;
                        }
                    }
                }
                if (iBitDC == sumOfProducts->numVars)
                {
                    break;
                }
            }
        }
    }

    free(refCntTable);

    if (sumOfProducts->numTerms != numOldTerms)
    {
        // we're just making these buffers smaller so it should never fail, but ignore the case that it does
        void* p;
        p = realloc(sumOfProducts->terms, sumOfProducts->numTerms * sizeof(long));
        sumOfProducts->terms = (p || sumOfProducts->numTerms == 0) ? p : sumOfProducts->terms;
        p = realloc(sumOfProducts->dontCares, sumOfProducts->numTerms * sizeof(long));
        sumOfProducts->dontCares = (p || sumOfProducts->numTerms == 0) ? p : sumOfProducts->dontCares;
    }
}
