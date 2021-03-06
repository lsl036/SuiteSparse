//------------------------------------------------------------------------------
// GB_mex_assign: C<Mask>(I,J) = accum (C (I,J), A)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

// This function is a wrapper for GrB_Matrix_assign, GrB_Matrix_assign_T
// GrB_Vector_assign, and GrB_Vector_assign_T.  For these uses, the Mask must
// always be the same size as C.

// This mexFunction does not call GrB_Row_assign or GrB_Col_assign, since
// the Mask is a single row or column in these cases, and C is not modified
// outside that single row (for GrB_Row_assign) or column (for GrB_Col_assign).

// This function does the same thing as the MATLAB mimic GB_spec_assign.m.

//------------------------------------------------------------------------------

#include "GB_mex.h"

#define USAGE "C =GB_mex_assign (C, Mask, accum, A, I, J, desc) or (C, Work)"

#define FREE_ALL                        \
{                                       \
    GB_MATRIX_FREE (&A) ;               \
    GB_MATRIX_FREE (&Mask) ;            \
    GB_MATRIX_FREE (&C) ;               \
    GrB_Descriptor_free (&desc) ;       \
    GB_mx_put_global (true, 0) ;        \
}

#define GET_DEEP_COPY \
    C = GB_mx_mxArray_to_Matrix (pargin [0], "C input", true, true) ;

#define FREE_DEEP_COPY GB_MATRIX_FREE (&C) ;

GrB_Matrix C = NULL ;
GrB_Matrix Mask = NULL ;
GrB_Matrix A = NULL ;
GrB_Descriptor desc = NULL ;
GrB_BinaryOp accum = NULL ;
GrB_Index *I = NULL, ni = 0, I_range [3] ;
GrB_Index *J = NULL, nj = 0, J_range [3] ;
bool ignore ;
bool malloc_debug = false ;
GrB_Info info = GrB_SUCCESS ;
GrB_Info assign (void) ;

GrB_Info many_assign
(
    int nwork,
    int fA,
    int fI,
    int fJ,
    int faccum,
    int fMask,
    int fdesc,
    mxClassID cclass,
    const mxArray *pargin [ ]
) ;

//------------------------------------------------------------------------------
// assign: perform a single assignment
//------------------------------------------------------------------------------

#define OK(method)                      \
{                                       \
    info = method ;                     \
    if (info != GrB_SUCCESS)            \
    {                                   \
        return (info) ;                 \
    }                                   \
}

GrB_Info assign ( )
{
    GB_WHERE ("assign") ;

    bool at = (desc != NULL && desc->in0 == GrB_TRAN) ;
    GrB_Info info ;

    // printf ("\n--- assign:\n") ;
    ASSERT_MATRIX_OK (C, "C", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (Mask, "Mask", GB0) ;
    ASSERT_MATRIX_OK (A, "A", GB0) ;
    ASSERT_BINARYOP_OK_OR_NULL (accum, "accum", GB0) ;
    ASSERT_DESCRIPTOR_OK_OR_NULL (desc, "desc", GB0) ;

    if (GB_NROWS (A) == 1 && GB_NCOLS (A) == 1 && GB_NNZ (A) == 1)
    {
        // scalar expansion to matrix or vector
        GB_void *Ax = A->x ;

        if (ni == 1 && nj == 1 && Mask == NULL && I != GrB_ALL && J != GrB_ALL
            && GB_op_is_second (accum, C->type) && A->type->code <= GB_FP64_code
            && desc == NULL)
        {
            // test GrB_Matrix_setElement
            #define ASSIGN(suffix,type)                                     \
            {                                                               \
                type x = ((type *) Ax) [0] ;                                \
                OK (GrB_Matrix_setElement ## suffix (C, x, I [0], J [0])) ; \
            } break ;

            switch (A->type->code)
            {
                case GB_BOOL_code   : ASSIGN (_BOOL,   bool) ;
                case GB_INT8_code   : ASSIGN (_INT8,   int8_t) ;
                case GB_UINT8_code  : ASSIGN (_UINT8,  uint8_t) ;
                case GB_INT16_code  : ASSIGN (_INT16,  int16_t) ;
                case GB_UINT16_code : ASSIGN (_UINT16, uint16_t) ;
                case GB_INT32_code  : ASSIGN (_INT32,  int32_t) ;
                case GB_UINT32_code : ASSIGN (_UINT32, uint32_t) ;
                case GB_INT64_code  : ASSIGN (_INT64,  int64_t) ;
                case GB_UINT64_code : ASSIGN (_UINT64, uint64_t) ;
                case GB_FP32_code   : ASSIGN (_FP32,   float) ;
                case GB_FP64_code   : ASSIGN (_FP64,   double) ;
                case GB_UDT_code    :
                default:
                    FREE_ALL ;
                    mexErrMsgTxt ("unsupported class") ;
            }
            #undef ASSIGN

            ASSERT_MATRIX_OK (C, "C after setElement", GB0) ;

        }
        if (C->vdim == 1)
        {

            // test GrB_Vector_assign_scalar functions
            #define ASSIGN(suffix,type)                                 \
            {                                                           \
                type x = ((type *) Ax) [0] ;                            \
                OK (GrB_Vector_assign ## suffix ((GrB_Vector) C,        \
                    (GrB_Vector) Mask, accum, x, I, ni, desc)) ;        \
            } break ;

            switch (A->type->code)
            {
                case GB_BOOL_code   : ASSIGN (_BOOL,   bool) ;
                case GB_INT8_code   : ASSIGN (_INT8,   int8_t) ;
                case GB_UINT8_code  : ASSIGN (_UINT8,  uint8_t) ;
                case GB_INT16_code  : ASSIGN (_INT16,  int16_t) ;
                case GB_UINT16_code : ASSIGN (_UINT16, uint16_t) ;
                case GB_INT32_code  : ASSIGN (_INT32,  int32_t) ;
                case GB_UINT32_code : ASSIGN (_UINT32, uint32_t) ;
                case GB_INT64_code  : ASSIGN (_INT64,  int64_t) ;
                case GB_UINT64_code : ASSIGN (_UINT64, uint64_t) ;
                case GB_FP32_code   : ASSIGN (_FP32,   float) ;
                case GB_FP64_code   : ASSIGN (_FP64,   double) ;
                case GB_UDT_code    :
                {
                    OK (GrB_Vector_assign_UDT ((GrB_Vector) C,
                        (GrB_Vector) Mask, accum, Ax, I, ni, desc)) ;
                }
                break ;
                default:
                    FREE_ALL ;
                    mexErrMsgTxt ("unsupported class") ;
            }
            #undef ASSIGN

        }
        else
        {

            // test Matrix_assign_scalar functions
            // printf ("scalar assign to matrix\n") ;
            #define ASSIGN(suffix,type)                                 \
            {                                                           \
                type x = ((type *) Ax) [0] ;                            \
                OK (GrB_Matrix_assign ## suffix (C, Mask, accum, x,     \
                    I, ni, J, nj,desc)) ;                               \
            } break ;

            switch (A->type->code)
            {
                case GB_BOOL_code   : ASSIGN (_BOOL,   bool) ;
                case GB_INT8_code   : ASSIGN (_INT8,   int8_t) ;
                case GB_UINT8_code  : ASSIGN (_UINT8,  uint8_t) ;
                case GB_INT16_code  : ASSIGN (_INT16,  int16_t) ;
                case GB_UINT16_code : ASSIGN (_UINT16, uint16_t) ;
                case GB_INT32_code  : ASSIGN (_INT32,  int32_t) ;
                case GB_UINT32_code : ASSIGN (_UINT32, uint32_t) ;
                case GB_INT64_code  : ASSIGN (_INT64,  int64_t) ;
                case GB_UINT64_code : ASSIGN (_UINT64, uint64_t) ;
                case GB_FP32_code   : ASSIGN (_FP32,   float) ;
                case GB_FP64_code   : ASSIGN (_FP64,   double) ;
                case GB_UDT_code    :
                {
                    OK (GrB_Matrix_assign_UDT (C, Mask, accum,
                        Ax, I, ni, J, nj, desc)) ;
                }
                break ;

                default:
                    FREE_ALL ;
                    mexErrMsgTxt ("unsupported class") ;
            }
            #undef ASSIGN

        }

    }
    else if (C->vdim == 1 && A->vdim == 1 &&
        (Mask == NULL || Mask->vdim == 1) && !at)
    {
        // test GrB_Vector_assign
        OK (GrB_Vector_assign ((GrB_Vector) C, (GrB_Vector) Mask, accum,
            (GrB_Vector) A, I, ni, desc)) ;
    }
    else
    {
        // standard submatrix assignment
        OK (GrB_Matrix_assign (C, Mask, accum, A, I, ni, J, nj, desc)) ;
    }

    ASSERT_MATRIX_OK (C, "Final C before wait", GB0) ;
    OK (GrB_wait ( )) ;
    return (info) ;
}

//------------------------------------------------------------------------------
// many_assign: do a sequence of assignments
//------------------------------------------------------------------------------

// The list of assignments is in a struct array

GrB_Info many_assign
(
    int nwork,
    int fA,
    int fI,
    int fJ,
    int faccum,
    int fMask,
    int fdesc,
    mxClassID cclass,
    const mxArray *pargin [ ]
)
{
    GB_WHERE ("many_assign") ;

    GrB_Info info = GrB_SUCCESS ;

    for (int64_t k = 0 ; k < nwork ; k++)
    {

        //----------------------------------------------------------------------
        // get the kth work to do
        //----------------------------------------------------------------------

        // each struct has fields A, I, J, and optionally Mask, accum, and desc

        mxArray *p ;

        // [ turn off malloc debugging
        bool save = GB_Global_malloc_debug_get ( ) ;
        GB_Global_malloc_debug_set (false) ;

        // get Mask (shallow copy)
        Mask = NULL ;
        if (fMask >= 0)
        {
            p = mxGetFieldByNumber (pargin [1], k, fMask) ;
            Mask = GB_mx_mxArray_to_Matrix (p, "Mask", false, false) ;
            if (Mask == NULL && !mxIsEmpty (p))
            {
                FREE_ALL ;
                mexErrMsgTxt ("Mask failed") ;
            }
        }

        // get A (shallow copy)
        p = mxGetFieldByNumber (pargin [1], k, fA) ;
        A = GB_mx_mxArray_to_Matrix (p, "A", false, true) ;
        if (A == NULL)
        {
            FREE_ALL ;
            mexErrMsgTxt ("A failed") ;
        }

        // get accum; default: NOP, default class is class(C)
        accum = NULL ;
        if (faccum >= 0)
        {
            p = mxGetFieldByNumber (pargin [1], k, faccum) ;
            if (!GB_mx_mxArray_to_BinaryOp (&accum, p, "accum",
                GB_NOP_opcode, cclass,
                C->type == Complex, A->type == Complex))
            {
                FREE_ALL ;
                mexErrMsgTxt ("accum failed") ;
            }
        }

        // get I
        p = mxGetFieldByNumber (pargin [1], k, fI) ;
        if (!GB_mx_mxArray_to_indices (&I, p, &ni, I_range, &ignore))
        {
            FREE_ALL ;
            mexErrMsgTxt ("I failed") ;
        }

        // get J
        p = mxGetFieldByNumber (pargin [1], k, fJ) ;
        if (!GB_mx_mxArray_to_indices (&J, p, &nj, J_range, &ignore))
        {
            FREE_ALL ;
            mexErrMsgTxt ("J failed") ;
        }

        // get desc
        desc = NULL ;
        if (fdesc > 0)
        {
            p = mxGetFieldByNumber (pargin [1], k, fdesc) ;
            if (!GB_mx_mxArray_to_Descriptor (&desc, p, "desc"))
            {
                FREE_ALL ;
                mexErrMsgTxt ("desc failed") ;
            }
        }

        // restore malloc debugging to test the method
        GB_Global_malloc_debug_set (save) ; // ]

        //----------------------------------------------------------------------
        // C<Mask>(I,J) = A
        //----------------------------------------------------------------------

        info = assign ( ) ;

        GB_MATRIX_FREE (&A) ;
        GB_MATRIX_FREE (&Mask) ;
        GrB_Descriptor_free (&desc) ;

        if (info != GrB_SUCCESS)
        {
            return (info) ;
        }
    }

    ASSERT_MATRIX_OK (C, "Final C before wait", GB0) ;
    OK (GrB_wait ( )) ;
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_mex_assign mexFunction
//------------------------------------------------------------------------------

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    malloc_debug = GB_mx_get_global (true) ;
    A = NULL ;
    C = NULL ;
    Mask = NULL ;
    desc = NULL ;

    GB_WHERE (USAGE) ;

    // printf ("\n========================= GB_mex_assign:\n") ;

    if (nargout > 1 || ! (nargin == 2 || nargin == 6 || nargin == 7))
    {
        mexErrMsgTxt ("Usage: " USAGE) ;
    }

    //--------------------------------------------------------------------------
    // get C (make a deep copy)
    //--------------------------------------------------------------------------

    GET_DEEP_COPY ;
    if (C == NULL)
    {
        FREE_ALL ;
        mexErrMsgTxt ("C failed") ;
    }
    mxClassID cclass = GB_mx_Type_to_classID (C->type) ;

    if (nargin == 2)
    {

        //----------------------------------------------------------------------
        // get a list of work to do: a struct array of length nwork
        //----------------------------------------------------------------------

        // each entry is a struct with fields:
        // Mask, accum, A, I, J, desc

        if (!mxIsStruct (pargin [1]))
        {
            FREE_ALL ;
            mexErrMsgTxt ("2nd argument must be a struct") ;
        }

        int nwork = mxGetNumberOfElements (pargin [1]) ;
        int nf = mxGetNumberOfFields (pargin [1]) ;
        for (int f = 0 ; f < nf ; f++)
        {
            mxArray *p ;
            for (int k = 0 ; k < nwork ; k++)
            {
                p = mxGetFieldByNumber (pargin [1], k, f) ;
            }
        }

        int fA = mxGetFieldNumber (pargin [1], "A") ;
        int fI = mxGetFieldNumber (pargin [1], "I") ;
        int fJ = mxGetFieldNumber (pargin [1], "J") ;
        int faccum = mxGetFieldNumber (pargin [1], "accum") ;
        int fMask = mxGetFieldNumber (pargin [1], "Mask") ;
        int fdesc = mxGetFieldNumber (pargin [1], "desc") ;

        if (fA < 0 || fI < 0 || fJ < 0) mexErrMsgTxt ("A,I,J required") ;

        METHOD (many_assign (nwork, fA, fI, fJ, faccum, fMask, fdesc, cclass,
            pargin)) ;

    }
    else
    {

        //----------------------------------------------------------------------
        // C<Mask>(I,J) = A, with a single assignment
        //----------------------------------------------------------------------

        // get Mask (shallow copy)
        Mask = GB_mx_mxArray_to_Matrix (pargin [1], "Mask", false, false) ;
        if (Mask == NULL && !mxIsEmpty (pargin [1]))
        {
            FREE_ALL ;
            mexErrMsgTxt ("Mask failed") ;
        }

        // get A (shallow copy)
        A = GB_mx_mxArray_to_Matrix (pargin [3], "A", false, true) ;
        if (A == NULL)
        {
            FREE_ALL ;
            mexErrMsgTxt ("A failed") ;
        }

        // get accum; default: NOP, default class is class(C)
        accum = NULL ;
        if (!GB_mx_mxArray_to_BinaryOp (&accum, pargin [2], "accum",
            GB_NOP_opcode, cclass, C->type == Complex, A->type == Complex))
        {
            FREE_ALL ;
            mexErrMsgTxt ("accum failed") ;
        }

        // get I
        if (!GB_mx_mxArray_to_indices (&I, pargin [4], &ni, I_range, &ignore))
        {
            FREE_ALL ;
            mexErrMsgTxt ("I failed") ;
        }

        // get J
        if (!GB_mx_mxArray_to_indices (&J, pargin [5], &nj, J_range, &ignore))
        {
            FREE_ALL ;
            mexErrMsgTxt ("J failed") ;
        }

        // get desc
        if (!GB_mx_mxArray_to_Descriptor (&desc, PARGIN (6), "desc"))
        {
            FREE_ALL ;
            mexErrMsgTxt ("desc failed") ;
        }

        // C<Mask>(I,J) = A

        METHOD (assign ( )) ;
    }

    //--------------------------------------------------------------------------
    // return C to MATLAB as a struct
    //--------------------------------------------------------------------------

    pargout [0] = GB_mx_Matrix_to_mxArray (&C, "C assign result", true) ;
    FREE_ALL ;
}

