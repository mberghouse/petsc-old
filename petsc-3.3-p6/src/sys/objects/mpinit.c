

#include <petscsys.h>        /*I  "petscsys.h"   I*/

static MPI_Comm saved_PETSC_COMM_WORLD = 0;
MPI_Comm PETSC_COMM_LOCAL_WORLD        = 0;           /* comm for a single node (local set of processes) */
PetscBool  PetscHMPIWorker           = PETSC_FALSE;  /* this is a regular process, nonworker process */
void* PetscHMPICtx                   = 0;

extern PetscErrorCode  PetscHMPIHandle(MPI_Comm);

#if defined(PETSC_HAVE_MPI_COMM_SPAWN)
#undef __FUNCT__  
#define __FUNCT__ "PetscHMPISpawn"
/*@C
   PetscHMPISpawn - Initialize additional processes to be used as "worker" processes. This is not generally 
     called by users. One should use -hmpi_spawn_size <n> to indicate that you wish to have n-1 new MPI 
     processes spawned for each current process.

   Not Collective (could make collective on MPI_COMM_WORLD, generate one huge comm and then split it up)

   Input Parameter:
.  nodesize - size of each compute node that will share processors

   Options Database:
.   -hmpi_spawn_size nodesize

   Notes: This is only supported on systems with an MPI 2 implementation that includes the MPI_Comm_Spawn() routine.

$    Comparison of two approaches for HMPI usage (MPI started with N processes)
$
$    -hmpi_spawn_size <n> requires MPI 2, results in n*N total processes with N directly used by application code
$                                           and n-1 worker processes (used by PETSc) for each application node.
$                           You MUST launch MPI so that only ONE MPI process is created for each hardware node.
$
$    -hmpi_merge_size <n> results in N total processes, N/n used by the application code and the rest worker processes
$                            (used by PETSc)
$                           You MUST launch MPI so that n MPI processes are created for each hardware node.
$
$    petscmpiexec -n 2 ./ex1 -hmpi_spawn_size 3 gives 2 application nodes (and 4 PETSc worker nodes)
$    petscmpiexec -n 6 ./ex1 -hmpi_merge_size 3 gives the SAME 2 application nodes and 4 PETSc worker nodes
$       This is what would use if each of the computers hardware nodes had 3 CPUs.
$
$      These are intended to be used in conjunction with USER HMPI code. The user will have 1 process per
$   computer (hardware) node (where the computer node has p cpus), the user's code will use threads to fully
$   utilize all the CPUs on the node. The PETSc code will have p processes to fully use the compute node for 
$   PETSc calculations. The user THREADS and PETSc PROCESSES will NEVER run at the same time so the p CPUs 
$   are always working on p task, never more than p.
$
$    See PCHMPI for a PETSc preconditioner that can use this functionality
$

   For both PetscHMPISpawn() and PetscHMPIMerge() PETSC_COMM_WORLD consists of one process per "node", PETSC_COMM_LOCAL_WORLD
   consists of all the processes in a "node."

   In both cases the user's code is running ONLY on PETSC_COMM_WORLD (that was newly generated by running this command).

   Level: developer

   Concepts: HMPI
   
.seealso: PetscFinalize(), PetscInitializeFortran(), PetscGetArgs(), PetscHMPIFinalize(), PetscInitialize(), PetscHMPIMerge(), PetscHMPIRun()

@*/
PetscErrorCode  PetscHMPISpawn(PetscMPIInt nodesize)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;
  MPI_Comm       parent,children;
							   
  PetscFunctionBegin;
  ierr = MPI_Comm_get_parent(&parent);CHKERRQ(ierr);
  if (parent == MPI_COMM_NULL) {  /* the original processes started by user */
    char programname[PETSC_MAX_PATH_LEN];
    char **argv;

    ierr = PetscGetProgramName(programname,PETSC_MAX_PATH_LEN);CHKERRQ(ierr);
    ierr = PetscGetArguments(&argv);CHKERRQ(ierr);
    ierr = MPI_Comm_spawn(programname,argv,nodesize-1,MPI_INFO_NULL,0,PETSC_COMM_SELF,&children,MPI_ERRCODES_IGNORE);CHKERRQ(ierr);
    ierr = PetscFreeArguments(argv);CHKERRQ(ierr);
    ierr = MPI_Intercomm_merge(children,0,&PETSC_COMM_LOCAL_WORLD);CHKERRQ(ierr); 

    ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
    ierr = PetscInfo2(0,"PETSc HMPI successfully spawned: number of nodes = %d node size = %d\n",size,nodesize);CHKERRQ(ierr);
    saved_PETSC_COMM_WORLD = PETSC_COMM_WORLD;
  } else { /* worker nodes that get spawned */
    ierr              = MPI_Intercomm_merge(parent,1,&PETSC_COMM_LOCAL_WORLD);CHKERRQ(ierr); 
    ierr              = PetscHMPIHandle(PETSC_COMM_LOCAL_WORLD);CHKERRQ(ierr);
    PetscHMPIWorker = PETSC_TRUE; /* so that PetscHMPIFinalize() will not attempt a broadcast from this process */
    ierr              = PetscEnd();  /* cannot continue into user code */
  }
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIMerge"
/*@C
   PetscHMPIMerge - Initializes the PETSc and MPI to work with HMPI. This is not usually called
      by the user. One should use -hmpi_merge_size <n> to indicate the node size of merged communicator
      to be.

   Collective on MPI_COMM_WORLD or PETSC_COMM_WORLD if it has been set

   Input Parameter:
+  nodesize - size of each compute node that will share processors
.  func - optional function to call on the master nodes
-  ctx - context passed to function on master nodes

   Options Database:
.   -hmpi_merge_size <n>

   Level: developer

$    Comparison of two approaches for HMPI usage (MPI started with N processes)
$
$    -hmpi_spawn_size <n> requires MPI 2, results in n*N total processes with N directly used by application code
$                                           and n-1 worker processes (used by PETSc) for each application node.
$                           You MUST launch MPI so that only ONE MPI process is created for each hardware node.
$
$    -hmpi_merge_size <n> results in N total processes, N/n used by the application code and the rest worker processes
$                            (used by PETSc)
$                           You MUST launch MPI so that n MPI processes are created for each hardware node.
$
$    petscmpiexec -n 2 ./ex1 -hmpi_spawn_size 3 gives 2 application nodes (and 4 PETSc worker nodes)
$    petscmpiexec -n 6 ./ex1 -hmpi_merge_size 3 gives the SAME 2 application nodes and 4 PETSc worker nodes
$       This is what would use if each of the computers hardware nodes had 3 CPUs.
$
$      These are intended to be used in conjunction with USER HMPI code. The user will have 1 process per
$   computer (hardware) node (where the computer node has p cpus), the user's code will use threads to fully
$   utilize all the CPUs on the node. The PETSc code will have p processes to fully use the compute node for 
$   PETSc calculations. The user THREADS and PETSc PROCESSES will NEVER run at the same time so the p CPUs 
$   are always working on p task, never more than p.
$
$    See PCHMPI for a PETSc preconditioner that can use this functionality
$

   For both PetscHMPISpawn() and PetscHMPIMerge() PETSC_COMM_WORLD consists of one process per "node", PETSC_COMM_LOCAL_WORLD
   consists of all the processes in a "node."

   In both cases the user's code is running ONLY on PETSC_COMM_WORLD (that was newly generated by running this command).

   Concepts: HMPI
   
.seealso: PetscFinalize(), PetscInitializeFortran(), PetscGetArgs(), PetscHMPIFinalize(), PetscInitialize(), PetscHMPISpawn(), PetscHMPIRun()

@*/
PetscErrorCode  PetscHMPIMerge(PetscMPIInt nodesize,PetscErrorCode (*func)(void*),void *ctx)
{
  PetscErrorCode ierr;
  PetscMPIInt    size,rank,*ranks,i;
  MPI_Group      group,newgroup;

  PetscFunctionBegin;
  saved_PETSC_COMM_WORLD = PETSC_COMM_WORLD;

  ierr = MPI_Comm_size(saved_PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size % nodesize) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Total number of process nodes %d is not divisible by number of processes per node %d",size,nodesize);
  ierr = MPI_Comm_rank(saved_PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);


  /* create two communicators 
      *) one that contains the first process from each node: 0,nodesize,2*nodesize,...
      *) one that contains all processes in a node:  (0,1,2...,nodesize-1), (nodesize,nodesize+1,...2*nodesize-), ...
  */
  ierr = MPI_Comm_group(saved_PETSC_COMM_WORLD,&group);CHKERRQ(ierr);
  ierr = PetscMalloc((size/nodesize)*sizeof(PetscMPIInt),&ranks);CHKERRQ(ierr);
  for (i=0; i<(size/nodesize); i++) ranks[i] = i*nodesize;
  ierr = MPI_Group_incl(group,size/nodesize,ranks,&newgroup);CHKERRQ(ierr);
  ierr = PetscFree(ranks);CHKERRQ(ierr);
  ierr = MPI_Comm_create(saved_PETSC_COMM_WORLD,newgroup,&PETSC_COMM_WORLD);CHKERRQ(ierr);
  if (rank % nodesize) PETSC_COMM_WORLD = 0; /* mark invalid processes for easy debugging */
  ierr = MPI_Group_free(&group);CHKERRQ(ierr);
  ierr = MPI_Group_free(&newgroup);CHKERRQ(ierr);

  ierr = MPI_Comm_split(saved_PETSC_COMM_WORLD,rank/nodesize,rank % nodesize,&PETSC_COMM_LOCAL_WORLD);CHKERRQ(ierr);

  ierr = PetscInfo2(0,"PETSc HMPI successfully started: number of nodes = %d node size = %d\n",size/nodesize,nodesize);CHKERRQ(ierr);
  ierr = PetscInfo1(0,"PETSc HMPI process %sactive\n",(rank % nodesize) ? "in" : "");CHKERRQ(ierr);

  PetscHMPICtx = ctx;
  /* 
     All process not involved in user application code wait here
  */
  if (!PETSC_COMM_WORLD) {
    ierr              = PetscHMPIHandle(PETSC_COMM_LOCAL_WORLD);CHKERRQ(ierr);
    PETSC_COMM_WORLD  = saved_PETSC_COMM_WORLD;
    PetscHMPIWorker = PETSC_TRUE; /* so that PetscHMPIFinalize() will not attempt a broadcast from this process */
    ierr = PetscInfo(0,"PETSc HMPI inactive process becoming active");CHKERRQ(ierr);
  } else {
    if (func) {
      ierr = (*func)(ctx);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIFinalize"
/*@C
   PetscHMPIFinalize - Finalizes the PETSc and MPI to work with HMPI. Called by PetscFinalize() cannot
       be called by user.

   Collective on the entire system

   Level: developer
           
.seealso: PetscFinalize(), PetscGetArgs(), PetscHMPIMerge(), PCHMPIRun()

@*/
PetscErrorCode  PetscHMPIFinalize(void)
{
  PetscErrorCode ierr = 0;
  PetscInt       command = 3;

  PetscFunctionBegin;
  if (!PetscHMPIWorker && PETSC_COMM_LOCAL_WORLD) {
    ierr = MPI_Bcast(&command,1,MPIU_INT,0,PETSC_COMM_LOCAL_WORLD);CHKERRQ(ierr); /* broadcast to my worker group to end program */
    PETSC_COMM_WORLD = saved_PETSC_COMM_WORLD;
    ierr = PetscInfo(0,"PETSc HMPI active process ending PetscHMPIMerge()");CHKERRQ(ierr);
  }
  PetscFunctionReturn(ierr);
}

static PetscInt numberobjects = 0;
static void     *objects[100];

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIHandle"
/*@C
   PetscHMPIHandle - Receives commands from the master node and processes them

   Collective on MPI_Comm

   Input Parameter:
.   comm - Must be PETSC_COMM_LOCAL_WORLD

   Level: developer

   Notes: this is usually handled automatically, likely you do not need to use this directly

   Developer Notes: Since comm must be PETSC_COMM_LOCAL_WORLD, why have this argument?
           
.seealso: PetscHMPIMerge(), PCHMPIRun(), PCHMPINew()

@*/
PetscErrorCode  PetscHMPIHandle(MPI_Comm comm)
{
  PetscErrorCode ierr;
  PetscInt       command = 0; /* dummy value so MPI-Uni doesn't think it is not set*/
  PetscBool      exitwhileloop = PETSC_FALSE;

  PetscFunctionBegin;
  while (!exitwhileloop) {
    ierr = MPI_Bcast(&command,1,MPIU_INT,0,comm);CHKERRQ(ierr);
    switch (command) {
    case 0: { /* allocate some memory on this worker process */
      size_t   n = 0; /* dummy value so MPI-Uni doesn't think it is not set*/
      void     *ptr;
      ierr = MPI_Bcast(&n,1,MPIU_SIZE_T,0,comm);CHKERRQ(ierr);
      /* cannot use PetscNew() cause it requires struct argument */
      ierr = PetscMalloc(n,&ptr);CHKERRQ(ierr);
      ierr = PetscMemzero(ptr,n);CHKERRQ(ierr);
      objects[numberobjects++] = ptr;
      break;
    }
    case 1: {  /* free some memory on this worker process */
      PetscInt i;
      ierr = MPI_Bcast(&i,1,MPIU_INT,0,comm);CHKERRQ(ierr);
      ierr = PetscFree(objects[i]);CHKERRQ(ierr);
      break;
    }
    case 2: {  /* run a function on this worker process */
      PetscInt       i;
      PetscErrorCode (*f)(MPI_Comm,void*);
      ierr = MPI_Bcast(&i,1,MPIU_INT,0,comm);CHKERRQ(ierr);
      ierr = MPI_Bcast(&f,1,MPIU_SIZE_T,0,comm);CHKERRQ(ierr);
      ierr = (*f)(comm,objects[i]);CHKERRQ(ierr);
      break;
    }
    case 4: {  /* run a function on this worker process with provided context */
      PetscInt       i;
      PetscErrorCode (*f)(MPI_Comm,void*,void*);
      ierr = MPI_Bcast(&i,1,MPIU_INT,0,comm);CHKERRQ(ierr);
      ierr = MPI_Bcast(&f,1,MPIU_SIZE_T,0,comm);CHKERRQ(ierr);
      ierr = (*f)(comm,PetscHMPICtx,objects[i]);CHKERRQ(ierr);
      break;
    }
    case 3: {
      exitwhileloop = PETSC_TRUE;
      break;
    }
    default:
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Unknown HMPI command %D",command);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIMalloc"
/*@C
   PetscHMPIMalloc - Creates a "c struct" on all nodes of an HMPI communicator

   Collective on MPI_Comm

   Input Parameters:
+   comm - Must be PETSC_COMM_LOCAL_WORLD
-   n  - amount of memory requested

   Level: developer

   Developer Notes: Since comm must be PETSC_COMM_LOCAL_WORLD, why have this argument?

.seealso: PetscHMPIMerge(), PCHMPIRun(), PCHMPIFree()

@*/
PetscErrorCode  PetscHMPIMalloc(MPI_Comm comm,size_t n,void **ptr)
{
  PetscErrorCode ierr;
  PetscInt       command = 0;

  PetscFunctionBegin;
  if (PetscHMPIWorker) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not using HMPI feature of PETSc");

  ierr = MPI_Bcast(&command,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(&n,1,MPIU_SIZE_T,0,comm);CHKERRQ(ierr); 
  /* cannot use PetscNew() cause it requires struct argument */
  ierr = PetscMalloc(n,ptr);CHKERRQ(ierr);
  ierr = PetscMemzero(*ptr,n);CHKERRQ(ierr);
  objects[numberobjects++] = *ptr;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIFree"
/*@C
   PetscHMPIFree - Frees a "c struct" on all nodes of an HMPI communicator

   Collective on MPI_Comm

   Input Parameters:
+   comm - Must be PETSC_COMM_LOCAL_WORLD
-   ptr - pointer to data to be freed, must have been obtained with PetscHMPIMalloc()

   Level: developer
           
  Developer Notes: Since comm must be PETSC_COMM_LOCAL_WORLD, why have this argument?

.seealso: PetscHMPIMerge(), PetscHMPIMalloc()

@*/
PetscErrorCode  PetscHMPIFree(MPI_Comm comm,void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       command = 1,i;

  PetscFunctionBegin;
  if (PetscHMPIWorker) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not using HMPI feature of PETSc");

  ierr = MPI_Bcast(&command,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  for (i=0; i<numberobjects; i++) {
    if (objects[i] == ptr) {
      ierr = MPI_Bcast(&i,1,MPIU_INT,0,comm);CHKERRQ(ierr);
      ierr = PetscFree(objects[i]);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
  }
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Pointer does not appear to have been created with PetscHMPIMalloc()");
  PetscFunctionReturn(ierr);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIRun"
/*@C
   PetscHMPIRun - runs a function on all the processes of a node

   Collective on MPI_Comm

   Input Parameters:
+   comm - communicator to run function on, must be PETSC_COMM_LOCAL_WORLD
.   f - function to run
-   ptr - pointer to data to pass to function; must be obtained with PetscHMPIMalloc()

   Level: developer
           
   Developer Notes: Since comm must be PETSC_COMM_LOCAL_WORLD, why have this argument?

.seealso: PetscHMPIMerge(), PetscHMPIMalloc(), PetscHMPIFree(), PetscHMPIRunCtx()

@*/
PetscErrorCode  PetscHMPIRun(MPI_Comm comm,PetscErrorCode (*f)(MPI_Comm,void *),void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       command = 2,i;

  PetscFunctionBegin;
  if (PetscHMPIWorker) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not using HMPI feature of PETSc");

  ierr = MPI_Bcast(&command,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  for (i=0; i<numberobjects; i++) {
    if (objects[i] == ptr) {
      ierr = MPI_Bcast(&i,1,MPIU_INT,0,comm);CHKERRQ(ierr);
      ierr = MPI_Bcast(&f,1,MPIU_SIZE_T,0,comm);CHKERRQ(ierr);
      ierr = (*f)(comm,ptr);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
  }
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Pointer does not appear to have been created with PetscHMPIMalloc()");
  PetscFunctionReturn(ierr);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscHMPIRunCtx"
/*@C
   PetscHMPIRunCtx - runs a function on all the processes of a node

   Collective on MPI_Comm

   Input Parameters:
+   comm - communicator to run function on, must be PETSC_COMM_LOCAL_WORLD
.   f - function to run
-   ptr - pointer to data to pass to function; must be obtained with PetscHMPIMalloc()

   Notes: This is like PetscHMPIRun() except it also passes the context passed in PetscHMPIMerge()
   Level: developer
           
   Developer Notes: Since comm must be PETSC_COMM_LOCAL_WORLD, why have this argument?

.seealso: PetscHMPIMerge(), PetscHMPIMalloc(), PetscHMPIFree(), PetscHMPIRun()

@*/
PetscErrorCode  PetscHMPIRunCtx(MPI_Comm comm,PetscErrorCode (*f)(MPI_Comm,void*,void *),void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       command = 4,i;

  PetscFunctionBegin;
  if (PetscHMPIWorker) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not using HMPI feature of PETSc");

  ierr = MPI_Bcast(&command,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  for (i=0; i<numberobjects; i++) {
    if (objects[i] == ptr) {
      ierr = MPI_Bcast(&i,1,MPIU_INT,0,comm);CHKERRQ(ierr);
      ierr = MPI_Bcast(&f,1,MPIU_SIZE_T,0,comm);CHKERRQ(ierr);
      ierr = (*f)(comm,PetscHMPICtx,ptr);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
  }
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Pointer does not appear to have been created with PetscHMPIMalloc()");
  PetscFunctionReturn(ierr);
}