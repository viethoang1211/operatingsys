// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"


#define MaxFileLength 32 
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in kernel->machine.h.
//----------------------------------------------------------------------

// Input: - User space address (int)
// - Limit of buffer (int)
// Output:- Buffer (char*)
// Purpose: Copy buffer from User memory space to System memory space
// void IncreasePC()
// {
// 	int counter = kernel->machine->ReadRegister(PCReg);
//    	kernel->machine->WriteRegister(PrevPCReg, counter);
//     	counter = kernel->machine->ReadRegister(NextPCReg);
//     	kernel->machine->WriteRegister(PCReg, counter);
//    	kernel->machine->WriteRegister(NextPCReg, counter + 4);
// }


void IncreasePC()
{
	int counter = kernel->machine->ReadRegister(PCReg);
   	kernel->machine->WriteRegister(PrevPCReg, counter);
    	counter = kernel->machine->ReadRegister(NextPCReg);
    	kernel->machine->WriteRegister(PCReg, counter);
   	kernel->machine->WriteRegister(NextPCReg, counter + 4);
}
char* User2System(int virtAddr,int limit)
{
 int i;// index
 int oneChar;
 char* kernelBuf = NULL;
 kernelBuf = new char[limit +1];//need for terminal string
 if (kernelBuf == NULL)
 return kernelBuf;
 memset(kernelBuf,0,limit+1);
 //printf("\n Filename u2s:");
 for (i = 0 ; i < limit ;i++)
 {
 kernel->machine->ReadMem(virtAddr+i,1,&oneChar);
 kernelBuf[i] = (char)oneChar;
 //printf("%c",kernelBuf[i]);
 if (oneChar == 0)
 break;
 }
 return kernelBuf;
}


// Input: - User space address (int)
// - Limit of buffer (int)
// - Buffer (char[])
// Output:- Number of bytes copied (int)
// Purpose: Copy buffer from System memory space to User memory space
int System2User(int virtAddr,int len,char* buffer)
{
 if (len < 0) return -1;
 if (len == 0)return len;
 int i = 0;
 int oneChar = 0 ;
 do{
 oneChar= (int) buffer[i];
 kernel->machine->WriteMem(virtAddr+i,1,oneChar);
 i ++;
 }while(i < len && oneChar != 0);
 return i;
} 

void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

    switch (which) {
    case SyscallException:
      switch(type) {

	// halt
      case SC_Halt:
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();

	ASSERTNOTREACHED();
	break;

	//add 
      case SC_Add:
	DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	int result;
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	
	/* Modify return point */
	// {
	//   /* set previous programm counter (debugging only)*/
	//   kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	//   /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	//   kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	//   /* set next programm counter for brach execution */
	//   kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	// }

	return;
	
	ASSERTNOTREACHED();

	break;


	// create file 
	  case SC_Create:
	{
		int virtAddr;
		char* filename;
		DEBUG('a',"\n SC_Create call ...");
		DEBUG('a',"\n Reading virtual address of filename");
		// Lấy tham số tên tập tin từ thanh ghi r4
		virtAddr = kernel->machine->ReadRegister(4);
		DEBUG ('a',"\n Reading filename.");
		// MaxFileLength là = 32
		filename = User2System(virtAddr,MaxFileLength+1);
		if (filename == NULL)
		{
		 printf("\n Not enough memory in system");
		 DEBUG('a',"\n Not enough memory in system");
		 kernel->machine->WriteRegister(2,-1); // trả về lỗi cho chương
		 // trình người dùng
		 delete filename;
		 return;
		}
		DEBUG('a',"\n Finish reading filename.");
		if (!kernel->fileSystem->Create(filename,0))
		{
		 printf("\n Error create file '%s'",filename);
		 kernel->machine->WriteRegister(2,-1);
		 delete filename;
		 return;
		}
		printf("Create file success \n");
		kernel->machine->WriteRegister(2,0); // trả về cho chương trình
		 // người dùng thành công
		delete filename; 
		IncreasePC();
		return;

	    ASSERTNOTREACHED();


		break;
	}

	case SC_Open:
		{
			// Input: arg1: Dia chi cua chuoi name, arg2: type
			// Output: Tra ve OpenFileID neu thanh, -1 neu loi
			// Chuc nang: Tra ve ID cua file.
	 
			//OpenFileID Open(char *name, int type)
			int virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so name tu thanh ghi so 4
			int type = kernel->machine->ReadRegister(5); // Lay tham so type tu thanh ghi so 5
			char* filename;
			filename = User2System(virtAddr, MaxFileLength); // Copy chuoi tu vung nho User Space sang System Space voi bo dem name dai MaxFileLength
			//Kiem tra xem OS con mo dc file khong
			
			// update 4/1/2018
			int freeSlot = kernel->fileSystem->FindFreeSlot();
			if (freeSlot != -1) //Chi xu li khi con slot trong
			{
				if (type == 0 || type == 1) //chi xu li khi type = 0 hoac 1
				{
					
					if ((kernel->fileSystem->openf[freeSlot] = kernel->fileSystem->Open(filename, type)) != NULL) //Mo file thanh cong
					{
						kernel->machine->WriteRegister(2, freeSlot); //tra ve OpenFileID
						printf("Open file success \n");
					}
				}
				else  
				{
					kernel->machine->WriteRegister(2, -1); 
				}
				delete[] filename;

				IncreasePC();
				break;
			}
			kernel->machine->WriteRegister(2, -1); //Khong mo duoc file return -1
			
			delete[] filename;

			IncreasePC();
			break;
		}

	case SC_Close:
	{
		//Input id cua file(OpenFileID)
		// Output: 0: thanh cong, -1 that bai
		int fid = kernel->machine->ReadRegister(4); // Lay id cua file tu thanh ghi so 4
		if (fid >= 0 && fid <= 19) //Chi xu li khi fid nam trong [0, 19]
		{
			if (kernel->fileSystem->openf[fid]) //neu mo file thanh cong
			{
				delete kernel->fileSystem->openf[fid]; //Xoa vung nho luu tru file
				kernel->fileSystem->openf[fid] = NULL; //Gan vung nho NULL
				kernel->machine->WriteRegister(2, 0);
				break;
			}
		}
		kernel->machine->WriteRegister(2, -1);
		break;
	}



      default:
	cerr << "Unexpected system call " << type << "\n";
	break;
      }
      break;
    default:
      cerr << "Unexpected user mode exception" << (int)which << "\n";
      break;
    }
	// IncreasePC();
    ASSERTNOTREACHED();
	
}
// hi
