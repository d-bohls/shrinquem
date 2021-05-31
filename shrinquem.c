// shrinquem - An algorithm for logic minimization.
// Copyright (C) 2021 Damon Bohls <damonbohls@gmail.com>
// MIT License: https://github.com/d-bohls/shrinquem/blob/main/LICENSE

#define BITS_PER_BYTE (8)

#include <stdlib.h>
#include <string.h> // used for strlen
#include "shrinquem.h"

static unsigned long numTermsKept = 0;
static unsigned long numTermsRemoved = 0;

static void RemoveNonprimeImplicants(
    const unsigned long numVars,
    unsigned long *numTerms,
    unsigned long *terms[],
    unsigned long *dontCares[]);

/*************************************************************************
ReduceLogic
Purpose - generates an array representation of the minimum sum-of-products equation
Inputs
  numVars - number of variables, size of truthTable is 2^numVars
  truthTable - array of outputs for each minterm
Outputs:
  numTerms - number of rows in the 2D array termTable
  terms - each long represents a term, where each bit is a variable
  dontCares - each long represents a term, where each bit is a "don't care" for a variable

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

extern enum shrinqStatus ReduceLogic(
    const unsigned long numVars,
    const triLogic truthTable[],
    unsigned long *numTerms,
    unsigned long *terms[],
    unsigned long *dontCares[])
{
    enum shrinqStatus status = STATUS_OKAY;
    unsigned long maxVariables;
    unsigned long maxTerms;
    unsigned long sizeTruthtable;
    unsigned long numMinterms;
    unsigned long iInput;
    unsigned long iTerm;
    unsigned long iBitTest;
    unsigned long iBitDC;
    unsigned long bitMaskTest;
    unsigned long bitMaskDC;
    triLogic *resolved = NULL;

    /*=============================================================
        initialize and allocate
    =============================================================*/

    if (numTerms)
    {
        (*numTerms) = 0;
    }

    if (terms)
    {
        /* the caller should not have allocated any memory for the terms */
        (*terms) = NULL;
    }

    if (dontCares)
    {
        /* the caller should not have allocated any memory for the dontCares */
        (*dontCares) = NULL;
    }

    if (truthTable == NULL || numTerms == NULL || terms == NULL || dontCares == NULL)
    {
        status = STATUS_NULL_ARGUMENT;
        goto finalExit;
    }

    if (numVars < 1)
    {
        status = STATUS_TOO_FEW_VARIABLES;
        goto finalExit;
    }

    maxVariables = sizeof(long) * BITS_PER_BYTE;

    if (numVars > maxVariables)
    {
        status = STATUS_TOO_MANY_VARIABLES;
        goto finalExit;
    }

    sizeTruthtable = 1 << numVars; /* the truth table has 2^numVars elements */

    /* count the non-zero minterms so I can estimate the maximum amount of memory required */
    numMinterms = 0;
    for (iInput = 0; iInput < sizeTruthtable; iInput++)
    {
        if (truthTable[iInput] == LOGIC_TRUE)
        {
            numMinterms++;
        }
    }

    maxTerms = min(numMinterms, sizeTruthtable / 2);

    resolved = calloc(sizeTruthtable, sizeof(triLogic));
    (*terms) = malloc(maxTerms * sizeof(long));
    (*dontCares) = malloc(maxTerms * sizeof(long));

    if (resolved == NULL || terms == NULL || dontCares == NULL)
    {
        status = STATUS_OUT_OF_MEMORY;
        goto finalExit;
    }

    /*=============================================================
       reduce the logic
    =============================================================*/

    /* loop through each entry in the truth table and derive the terms for the reduced logic */
    for (iInput = 0; iInput < sizeTruthtable; iInput++)
    {
        if ((truthTable[iInput] == LOGIC_TRUE) && !resolved[iInput])
        {
            iTerm = (*numTerms);
            (*numTerms)++;
            (*terms)[iTerm] = iInput; /* the term starts out equal to the minterm */
            (*dontCares)[iTerm] = 0;  /* initially there are no "don't cares" */
            /* loop through each bit to see if it can be replaced by a "don't care" */
            for (iBitTest = 0; iBitTest < numVars; iBitTest++)
            {
                bitMaskTest = 1 << iBitTest;
                (*terms)[iTerm] ^= bitMaskTest; /* flip the test bit */
                /* test all minterms associated with the term by checking all the "don't care" combinations */
                /* start by clearing all "don't care" bits */
                (*terms)[iTerm] &= ~((*dontCares)[iTerm]);

                while (1)
                {
                    if (truthTable[(*terms)[iTerm]] == LOGIC_FALSE)
                    {
                        /* we can't replace this variable with a don't care, so flip the bit back and exit */
                        (*terms)[iTerm] ^= bitMaskTest;
                        break;
                    }
                    /* get the next minterm to test */
                    for (iBitDC = 0; iBitDC < iBitTest; iBitDC++)
                    {
                        bitMaskDC = 1 << iBitDC;
                        if ((*dontCares)[iTerm] & bitMaskDC)
                        {
                            if ((*terms)[iTerm] & bitMaskDC)
                            {
                                /* clear the bit and continue */
                                (*terms)[iTerm] &= ~bitMaskDC;
                            }
                            else
                            {
                                /* set the bit and exit */
                                (*terms)[iTerm] |= bitMaskDC;
                                break;
                            }
                        }
                    }
                    if (iBitDC == iBitTest)
                    {
                        /* we can replace this variable with a "don't care" and exit */
                        (*dontCares)[iTerm] |= bitMaskTest;
                        break;
                    }
                } /* end of the while loop */
            }
            /* at this point, we have expanded the term to cover as many minterms as possible */
            /* go through all minterms associated with this term to mark them as resolved */
            /* start by clearing all "don't care" bits */
            (*terms)[iTerm] &= ~((*dontCares)[iTerm]);

            while (1)
            {
                /* set this minterm as resolved */
                resolved[(*terms)[iTerm]] = LOGIC_TRUE;
                /* get the next minterm to set as resolved */
                for (iBitDC = 0; iBitDC < numVars; iBitDC++)
                {
                    bitMaskDC = 1 << iBitDC;
                    if ((*dontCares)[iTerm] & bitMaskDC)
                    {
                        if ((*terms)[iTerm] & bitMaskDC)
                        {
                            /* clear the bit and continue */
                            (*terms)[iTerm] &= ~bitMaskDC;
                        }
                        else
                        {
                            /* set the bit and exit */
                            (*terms)[iTerm] |= bitMaskDC;
                            break;
                        }
                    }
                }
                if (iBitDC == numVars)
                {
                    /* we're all done flagging resolved minterms */
                    break;
                }
            } /* end of while loop */
        }
    }

    /*=============================================================
      clean-up and free memory
    =============================================================*/

    /* free this memory first in case it helps the reallocs below */
    if (resolved)
    {
        free(resolved);
        resolved = NULL;
    }

    (*terms) = realloc((*terms), (*numTerms) * sizeof(long));
    (*dontCares) = realloc((*dontCares), (*numTerms) * sizeof(long));

    if (terms == NULL || dontCares == NULL)
    {
        status = STATUS_OUT_OF_MEMORY;
        goto finalExit;
    }

finalExit:

    if (resolved)
    {
        free(resolved);
        resolved = NULL;
    }

    if (status == STATUS_OKAY)
    {
        RemoveNonprimeImplicants(numVars, numTerms, terms, dontCares);
    }
    else
    {
        if (numTerms)
        {
            (*numTerms) = 0;
        }

        if (terms && *terms)
        {
            free(*terms);
            (*terms) = NULL;
        }

        if (dontCares && *dontCares)
        {
            free(*dontCares);
            (*dontCares) = NULL;
        }
    }

    return status;
}

/*************************************************************************
GenerateEquation
Purpose - generates a null-terminated string representation of the
  minimum sum-of-products equation generated by the ReduceLogic function
Inputs:
  numVars - number of variables
  varNames - string array naming each variable
  numTerms - use the value returned by the ReduceLogic function
  terms - use the value returned by the ReduceLogic function
  dontCares - use the value returned by the ReduceLogic function
Outputs:
  equation - null-terminated string representation of the minimum sum-of-products equation
*************************************************************************/

extern enum shrinqStatus GenerateEquation(
    const unsigned long numVars,
    char* varNames[],
    const unsigned long numTerms,
    const unsigned long terms[],
    const unsigned long dontCares[],
    char *equation[])
{
    enum shrinqStatus retVal = STATUS_OKAY;
    unsigned long done = 0;
    unsigned long iVar;
    unsigned long iTerm;
    unsigned long iEquPos;
    unsigned long iTermPos;
    unsigned long iCharPos;
    unsigned long bitMask;
    size_t *varNameSizes = NULL;
    size_t outputSize;
    unsigned long freeVarNames = 0;

    /*=============================================================
        generate the equation string
    =============================================================*/

    if (equation == NULL)
    {
        return STATUS_NULL_ARGUMENT;
    }
    else
    {
        /* the caller should not have allocated any memory for the equation */
        (*equation) = NULL;
    }

    /* first check for the case where the equation is '0' */
    if (numTerms <= 0)
    {
        (*equation) = (char *)malloc(2 * sizeof(char));
        if ((*equation) == 0)
            return STATUS_OUT_OF_MEMORY;
        (*equation)[0] = '0';
        (*equation)[1] = 0;
        done = 1;
    }
    else
    {

        /* now check for a '1' (one term with all don't cares) */
        if (numTerms == 1)
        {
            for (iVar = 0; iVar < numVars; iVar++)
            {
                bitMask = 1 << iVar;
                if ((dontCares[0] & bitMask) == 0)
                {
                    break;
                }
            }
            if (iVar == numVars)
            {
                (*equation) = (char *)malloc(2 * sizeof(char));
                if ((*equation) == 0)
                    return STATUS_OUT_OF_MEMORY;
                (*equation)[0] = '1';
                (*equation)[1] = 0;
                done = 1;
            }
        }

        /* if the equation isn't '0' or '1' then build the string */
        if (done == 0)
        {

            /* auto-name variables */
            if (varNames == NULL)
            {
                freeVarNames = 1;
                varNames = (char **)malloc(numVars * sizeof(char *));
                if (varNames == 0)
                    return STATUS_OUT_OF_MEMORY;
                for (iVar = 0; iVar < numVars; iVar++)
                {
                    varNames[iVar] = NULL;
                }
            }
            for (iVar = 0; iVar < numVars; iVar++)
            {
                if (varNames[iVar] == NULL)
                {
                    varNames[iVar] = (char *)malloc(2 * sizeof(char));
                    if (varNames[iVar] == 0)
                        return STATUS_OUT_OF_MEMORY;
                    (varNames[iVar])[0] = 'A' + (char)iVar;
                    (varNames[iVar])[1] = 0;
                }
            }

            /* first calculate the size of the string names for the variables */
            varNameSizes = (size_t *)malloc(sizeof(long *) * numVars);
            if (varNameSizes == NULL)
                return STATUS_OUT_OF_MEMORY;
            for (iVar = 0; iVar < numVars; iVar++)
            {
                varNameSizes[iVar] = strlen(varNames[iVar]);
            }

            /* now run through the equation to determine what size the output string will be */
            iTermPos = 0;
            iEquPos = 0;
            outputSize = 0;
            for (iTerm = 0; iTerm < numTerms; iTerm++)
            {
                for (iVar = 0; iVar < numVars; iVar++)
                {
                    bitMask = 1 << (numVars - iVar - 1);
                    if ((dontCares[iTerm] & bitMask) == 0)
                    {
                        outputSize += varNameSizes[iVar];
                        if ((terms[iTerm] & bitMask) == 0)
                        {
                            outputSize++;
                        }
                    }

                    iTermPos++;
                }
                if (iTerm < (numTerms - 1))
                {
                    outputSize += 3; /* account for the " + " */
                }
                else
                {
                    outputSize++; /* account for the null terminator */
                }
            }

            /* allocate space for the equation */
            (*equation) = (char *)malloc(outputSize * sizeof(char *));
            if ((*equation) == NULL)
                return STATUS_OUT_OF_MEMORY;

            /* use termTable and numTerms to generate equation */
            iTermPos = iEquPos = 0;
            for (iTerm = 0; iTerm < numTerms; iTerm++)
            {
                for (iVar = 0; iVar < numVars; iVar++)
                {
                    bitMask = 1 << (numVars - iVar - 1);
                    if ((dontCares[iTerm] & bitMask) == 0)
                    {
                        /* copy the variable name */
                        for (iCharPos = 0; iCharPos < varNameSizes[iVar]; iCharPos++)
                        {
                            (*equation)[iEquPos++] = (varNames[iVar])[iCharPos];
                        }
                        /* place the complement sign of false */
                        if ((terms[iTerm] & bitMask) == 0)
                        {
                            (*equation)[iEquPos++] = '\'';
                        }
                    }
                    iTermPos++;
                }
                if (iTerm < (numTerms - 1))
                {
                    (*equation)[iEquPos++] = ' ';
                    (*equation)[iEquPos++] = '+';
                    (*equation)[iEquPos++] = ' ';
                }
                else
                {
                    (*equation)[iEquPos++] = 0;
                }
            }
            done = 1;
        }
    }

    /*=============================================================
      clean-up and free memory
    =============================================================*/

    if (freeVarNames)
    {
        for (iVar = 0; iVar < numVars; iVar++)
        {
            if (varNames[iVar] != NULL)
            {
                free(varNames[iVar]);
                varNames[iVar] = NULL;
            }
        }
        if (varNames)
        {
            free(varNames);
            varNames = NULL;
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
Inputs:
  numVars  - number of variables/inputs
  numTerms - number of terms in the sum-of-products, returned by ReduceLogic
  terms    - terms in the sum-of-products, returned by ReduceLogic
  dontCars - dontCares in the sum-of-products, returned by ReduceLogic
  input    - represents a bit array of the inputs
Outputs:
  the boolean result of the sum-of-products evaluated for the inputs
*************************************************************************/

extern triLogic EvaluateSumOfProducts(
    const unsigned long numVars,
    const unsigned long numTerms,
    const unsigned long terms[],
    const unsigned long dontCares[],
    const unsigned long input)
{
    /* clear out bits that might be set in the input which are beyond the number of variables we are evaluating */
    const unsigned long mask = (1 << numVars) - 1;
    unsigned long constrainedInput = input & mask;

    for (unsigned long iTerm = 0; iTerm < numTerms; iTerm++)
    {
        /* one TRUE product term makes the whole sum-of-products TRUE */
        if ((constrainedInput | dontCares[iTerm]) == (terms[iTerm] | dontCares[iTerm]))
            return LOGIC_TRUE;
    }

    /* no product terms were true, so the whole equation is false */
    return LOGIC_FALSE;
}

extern void ResetTermCounters()
{
    numTermsKept = 0;
    numTermsRemoved = 0;
}

extern unsigned long GetNumTermsKept()
{
    return numTermsKept;
}

extern unsigned long GetNumTermsRemoved()
{
    return numTermsRemoved;
}

/*************************************************************************
RemoveNonprimeImplicants
Purpose - removes terms which are non-prime implicants.
Inputs:
  numVars - number of variables/inputs
  numTerms - number of terms in the sum-of-products, returned by ReduceLogic
  terms - terms in the sum-of-products, returned by ReduceLogic
  dontCares - dontCares in the sum-of-products, returned by ReduceLogic
Outputs:
  numTerms, terms, and dontCares are modified directly when terms can be removed
*************************************************************************/

static void RemoveNonprimeImplicants(
    const unsigned long numVars,
    unsigned long* numTerms,
    unsigned long* terms[],
    unsigned long* dontCares[])
{
    unsigned long* refCntTable;
    unsigned long sizeTruthtable;
    unsigned long numOldTerms;
    unsigned long iOldTerm;
    unsigned long iNewTerm;
    unsigned long bitMaskDC;
    char isPrime;

    numOldTerms = (*numTerms);

    sizeTruthtable = 1 << numVars; // the truth table has 2^numVars elements

    refCntTable = (long*)calloc(sizeTruthtable, sizeof(long));

    // loop through each term and ref count the minterms that the term covers
    for (iOldTerm = 0; iOldTerm < numOldTerms; iOldTerm++)
    {
        // start by clearing all the don't care bits
        (*terms)[iOldTerm] &= ~((*dontCares)[iOldTerm]);

        while (1)
        {
            refCntTable[(*terms)[iOldTerm]]++;

            // get the next minterm to ref count
            unsigned long iBitDC;
            for (iBitDC = 0; iBitDC < numVars; iBitDC++)
            {
                bitMaskDC = 1 << iBitDC;
                if ((*dontCares)[iOldTerm] & bitMaskDC)
                {
                    if ((*terms)[iOldTerm] & bitMaskDC)
                    {
                        (*terms)[iOldTerm] &= ~bitMaskDC;
                    }
                    else
                    {
                        (*terms)[iOldTerm] |= bitMaskDC;
                        break;
                    }
                }
            }
            if (iBitDC == numVars)
            {
                break;
            }
        }
    }

    // now loop through each term again and remove terms if all its minterms are ref counted more than once
    for (iNewTerm = iOldTerm = 0; iOldTerm < numOldTerms; iOldTerm++)
    {
        isPrime = 0;
        (*terms)[iOldTerm] &= ~((*dontCares)[iOldTerm]); // clear all the don't care bits

        while (1)
        {
            // exit early if minterm is ref counted once, this term is a prime implicant and we will keep it
            if (refCntTable[(*terms)[iOldTerm]] == 1)
            {
                isPrime = 1;
                break;
            }

            // get the next minterm to ref count
            unsigned long iBitDC;
            for (iBitDC = 0; iBitDC < numVars; iBitDC++)
            {
                bitMaskDC = 1 << iBitDC;
                if ((*dontCares)[iOldTerm] & bitMaskDC)
                {
                    if ((*terms)[iOldTerm] & bitMaskDC)
                    {
                        (*terms)[iOldTerm] &= ~bitMaskDC;
                    }
                    else
                    {
                        (*terms)[iOldTerm] |= bitMaskDC;
                        break;
                    }
                }
            }
            if (iBitDC == numVars)
            {
                break;
            }
        }

        if (isPrime)
        {
            // keep this term, copy it down to the next spot
            if (iOldTerm != iNewTerm)
            {
                (*terms)[iNewTerm] = (*terms)[iOldTerm];
                (*dontCares)[iNewTerm] = (*dontCares)[iOldTerm];
            }
            iNewTerm++;
            numTermsKept++;
        }
        else
        {
            // this term is a non-prime implicant and will not be kept
            (*numTerms)--;
            numTermsRemoved++;

            // de-ref count this term's minterms
            (*terms)[iOldTerm] &= ~((*dontCares)[iOldTerm]);
            while (1)
            {
                refCntTable[(*terms)[iOldTerm]]--;

                // get the next minterm the needs its ref count decremented
                unsigned long iBitDC;
                for (iBitDC = 0; iBitDC < numVars; iBitDC++)
                {
                    bitMaskDC = 1 << iBitDC;
                    if ((*dontCares)[iOldTerm] & bitMaskDC)
                    {
                        if ((*terms)[iOldTerm] & bitMaskDC)
                        {
                            (*terms)[iOldTerm] &= ~bitMaskDC;
                        }
                        else
                        {
                            (*terms)[iOldTerm] |= bitMaskDC;
                            break;
                        }
                    }
                }
                if (iBitDC == numVars)
                {
                    break;
                }
            }
        }
    }

    free(refCntTable);

    if ((*numTerms) != numOldTerms)
    {
        *terms = realloc(*terms, (*numTerms) * sizeof(long));
        *dontCares = realloc(*dontCares, (*numTerms) * sizeof(long));
    }
}
