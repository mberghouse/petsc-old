#include "petscsys.h"
#include "petscfix.h"
#include "petsc-private/fortranimpl.h"
/* vecnest.c */
/* Fortran interface file */

/*
* This file was generated automatically by bfort from the C source
* file.  
 */

#ifdef PETSC_USE_POINTER_CONVERSION
#if defined(__cplusplus)
extern "C" { 
#endif 
extern void *PetscToPointer(void*);
extern int PetscFromPointer(void *);
extern void PetscRmPointer(void*);
#if defined(__cplusplus)
} 
#endif 

#else

#define PetscToPointer(a) (*(long *)(a))
#define PetscFromPointer(a) (long)(a)
#define PetscRmPointer(a)
#endif

#include "petscvec.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define veccreatenest_ VECCREATENEST
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define veccreatenest_ veccreatenest
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL  veccreatenest_(MPI_Fint * comm,PetscInt *nb,IS is[],Vec x[],Vec *Y, int *__ierr ){
*__ierr = VecCreateNest(
	MPI_Comm_f2c( *(comm) ),*nb,is,x,Y);
}
#if defined(__cplusplus)
}
#endif
