#if !defined(__PETSCVERSION_H)
#define __PETSCVERSION_H

#define PETSC_VERSION_RELEASE    1
#define PETSC_VERSION_MAJOR      3
#define PETSC_VERSION_MINOR      3
#define PETSC_VERSION_SUBMINOR   0
#define PETSC_VERSION_PATCH      6
#define PETSC_VERSION_DATE       "Jun, 5, 2012"
#define PETSC_VERSION_PATCH_DATE "Mon Feb 11 12:26:34 CST 2013"

#if !defined (PETSC_VERSION_HG)
#define PETSC_VERSION_HG         "e4fbe4834f28c1f9858832b33fd5706ff7ec3c62"
#endif

#if !defined(PETSC_VERSION_DATE_HG)
#define PETSC_VERSION_DATE_HG         "Mon Feb 11 12:06:41 2013 -0600"
#endif

#define PETSC_VERSION_(MAJOR,MINOR,SUBMINOR) \
  ((PETSC_VERSION_MAJOR == (MAJOR)) &&       \
   (PETSC_VERSION_MINOR == (MINOR)) &&       \
   (PETSC_VERSION_SUBMINOR == (SUBMINOR)) && \
   (PETSC_VERSION_RELEASE  == 1))

#define PETSC_VERSION_LT(MAJOR,MINOR,SUBMINOR) \
  (PETSC_VERSION_MAJOR < (MAJOR) ||            \
   (PETSC_VERSION_MAJOR == (MAJOR) &&          \
    (PETSC_VERSION_MINOR < (MINOR) ||          \
     (PETSC_VERSION_MINOR == (MINOR) &&        \
      (PETSC_VERSION_SUBMINOR < (SUBMINOR))))))

#define PETSC_VERSION_LE(MAJOR,MINOR,SUBMINOR) \
  (PETSC_VERSION_LT(MAJOR,MINOR,SUBMINOR) || \
   PETSC_VERSION_(MAJOR,MINOR,SUBMINOR))

#endif