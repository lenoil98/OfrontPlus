MODULE [foreign] bbLinLibc;

	(*
		GNU/Linux (Ubuntu 17.10)
		i386
	*)

	IMPORT SYSTEM;

	CONST
		NULL* = 0H;
		FALSE* = 0;
		TRUE* = 1;

		CLOCKS_PER_SEC* = 1000000;

		MAP_FAILED* = -1;

		(* MAP_PRIVATE, MAP_ANON (intFlags) *)
		MAP_SHARED* = {0}; (* Share changes *)
		MAP_PRIVATE* = {1}; (* Changes are private *)
		MAP_SHARED_VALIDATE* = {0,1}; (* share + validate extension flags *)
		MAP_TYPE* = {0..3}; (* Mask for type of mapping *)
		MAP_FIXED* = {4}; (* Interpret addr exactly *)
		MAP_ANONYMOUS* = {5}; (* don't use a file *)
		MAP_FILE* = {};
		MAP_ANON* = MAP_ANONYMOUS;


		(* PROT_READ, PROT_WRITE, PROT_EXEC (intFlags) *)
		PROT_READ* = {0}; (* page can be read *)
		PROT_WRITE* = {1}; (* page can be written *)
		PROT_EXEC* = {2}; (* page can be executed *)
		PROT_SEM* = {3}; (* page may be used for atomic ops *)
		PROT_NONE* = {}; (* page can not be accessed *)
		PROT_GROWSDOWN* = {24}; (* mprotect flag: extend change to start of growsdown vma *)
		PROT_GROWSUP* = {25}; (* mprotect flag: extend change to end of growsup vma *)


		(* SIG_UNBLOCK, SIG_SETMASK (int) *)
		SIG_BLOCK* = 0; (* for blocking signals *)
		SIG_UNBLOCK* = 1; (* for unblocking signals *)
		SIG_SETMASK* = 2; (* for setting the signal mask *)


		(* FPE_INTDIV, FPE_INTOVF, FPE_FLTDIV, FPE_FLTOVF, FPE_FLTUND, FPE_FLTRES, FPE_FLTINV, FPE_FLTSUB (int) *)
		FPE_INTDIV* = 1; (* integer divide by zero *)
		FPE_INTOVF* = 2; (* integer overflow *)
		FPE_FLTDIV* = 3; (* floating point divide by zero *)
		FPE_FLTOVF* = 4; (* floating point overflow *)
		FPE_FLTUND* = 5; (* floating point underflow *)
		FPE_FLTRES* = 6; (* floating point inexact result *)
		FPE_FLTINV* = 7; (* floating point invalid operation *)
		FPE_FLTSUB* = 8; (* subscript out of range *)


		(* SA_SIGINFO (intFlags) *)
		SA_NOCLDSTOP* = {0};
		SA_NOCLDWAIT* = {1};
		SA_SIGINFO* = {2};
		SA_ONSTACK* = {27};
		SA_RESTART* = {28};
		SA_NODEFER* = {30};
		SA_RESETHAND* = {31};


		(* SIGINT, SIGILL, SIGFPE, SIGSEGV, SIGKILL, SIGSTOP, SIGWINCH, SIGTHR (int) *)
		SIGHUP* = 1;
		SIGINT* = 2;
		SIGQUIT* = 3;
		SIGILL* = 4;
		SIGTRAP* = 5;
		SIGABRT* = 6;
		SIGIOT* = 6;
		SIGBUS* = 7;
		SIGFPE* = 8;
		SIGKILL* = 9;
		SIGUSR1* = 10;
		SIGSEGV* = 11;
		SIGUSR2* = 12;
		SIGPIPE* = 13;
		SIGALRM* = 14;
		SIGTERM* = 15;
		SIGSTKFLT* = 16;
		SIGCHLD* = 17;
		SIGCONT* = 18;
		SIGSTOP* = 19;
		SIGTSTP* = 20;
		SIGTTIN* = 21;
		SIGTTOU* = 22;
		SIGURG* = 23;
		SIGXCPU* = 24;
		SIGXFSZ* = 25;
		SIGVTALRM* = 26;
		SIGPROF* = 27;
		SIGWINCH* = 28;
		SIGIO* = 29;
		SIGPOLL* = SIGIO;
		SIGLOST* = 29;
		SIGPWR* = 30;
		SIGSYS* = 31;
		SIGUNUSED* = 31;
		SIGRTMIN* = 32;

		_NSIG* = 64;

(*
		PAGE_SIZE* = 4096;
*)
		_SC_PAGESIZE* = 30;


		SIGSTKSZ* = 8192;

		(* ENOENT, EEXIST, EACCES, ENOMEM, EDQUOT, EMFILE, ENOTDIR (int) *)
		EPERM* = 1; (* Operation not permitted *)
		ENOENT* = 2; (* No such file or directory *)
		ESRCH* = 3; (* No such process *)
		EINTR* = 4; (* Interrupted system call *)
		EIO* = 5; (* I/O error *)
		ENXIO* = 6; (* No such device or address *)
		E2BIG* = 7; (* Argument list too long *)
		ENOEXEC* = 8; (* Exec format error *)
		EBADF* = 9; (* Bad file number *)
		ECHILD* = 10; (* No child processes *)
		EAGAIN* = 11; (* Try again *)
		ENOMEM* = 12; (* Out of memory *)
		EACCES* = 13; (* Permission denied *)
		EFAULT* = 14; (* Bad address *)
		ENOTBLK* = 15; (* Block device required *)
		EBUSY* = 16; (* Device or resource busy *)
		EEXIST* = 17; (* File exists *)
		EXDEV* = 18; (* Cross-device link *)
		ENODEV* = 19; (* No such device *)
		ENOTDIR* = 20; (* Not a directory *)
		EISDIR* = 21; (* Is a directory *)
		EINVAL* = 22; (* Invalid argument *)
		ENFILE* = 23; (* File table overflow *)
		EMFILE* = 24; (* Too many open files *)
		ENOTTY* = 25; (* Not a typewriter *)
		ETXTBSY* = 26; (* Text file busy *)
		EFBIG* = 27; (* File too large *)
		ENOSPC* = 28; (* No space left on device *)
		ESPIPE* = 29; (* Illegal seek *)
		EROFS* = 30; (* Read-only file system *)
		EMLINK* = 31; (* Too many links *)
		EPIPE* = 32; (* Broken pipe *)
		EDOM* = 33; (* Math argument out of domain of func *)
		ERANGE* = 34; (* Math result not representable *)
		EDEADLK* = 35; (* Resource deadlock would occur *)
		ENAMETOOLONG* = 36; (* File name too long *)
		ENOLCK* = 37; (* No record locks available *)
		ENOSYS* = 38; (* Invalid system call number *)
		ENOTEMPTY* = 39; (* Directory not empty *)
		ELOOP* = 40; (* Too many symbolic links encountered *)
		EWOULDBLOCK* = EAGAIN; (* Operation would block *)
		ENOMSG* = 42; (* No message of desired type *)
		EIDRM* = 43; (* Identifier removed *)
		ECHRNG* = 44; (* Channel number out of range *)
		EL2NSYNC* = 45; (* Level 2 not synchronized *)
		EL3HLT* = 46; (* Level 3 halted *)
		EL3RST* = 47; (* Level 3 reset *)
		ELNRNG* = 48; (* Link number out of range *)
		EUNATCH* = 49; (* Protocol driver not attached *)
		ENOCSI* = 50; (* No CSI structure available *)
		EL2HLT* = 51; (* Level 2 halted *)
		EBADE* = 52; (* Invalid exchange *)
		EBADR* = 53; (* Invalid request descriptor *)
		EXFULL* = 54; (* Exchange full *)
		ENOANO* = 55; (* No anode *)
		EBADRQC* = 56; (* Invalid request code *)
		EBADSLT* = 57; (* Invalid slot *)
		EDEADLOCK* = EDEADLK;
		EBFONT* = 59; (* Bad font file format *)
		ENOSTR* = 60; (* Device not a stream *)
		ENODATA* = 61; (* No data available *)
		ETIME* = 62; (* Timer expired *)
		ENOSR* = 63; (* Out of streams resources *)
		ENONET* = 64; (* Machine is not on the network *)
		ENOPKG* = 65; (* Package not installed *)
		EREMOTE* = 66; (* Object is remote *)
		ENOLINK* = 67; (* Link has been severed *)
		EADV* = 68; (* Advertise error *)
		ESRMNT* = 69; (* Srmount error *)
		ECOMM* = 70; (* Communication error on send *)
		EPROTO* = 71; (* Protocol error *)
		EMULTIHOP* = 72; (* Multihop attempted *)
		EDOTDOT* = 73; (* RFS specific error *)
		EBADMSG* = 74; (* Not a data message *)
		EOVERFLOW* = 75; (* Value too large for defined data type *)
		ENOTUNIQ* = 76; (* Name not unique on network *)
		EBADFD* = 77; (* File descriptor in bad state *)
		EREMCHG* = 78; (* Remote address changed *)
		ELIBACC* = 79; (* Can not access a needed shared library *)
		ELIBBAD* = 80; (* Accessing a corrupted shared library *)
		ELIBSCN* = 81; (* .lib section in a.out corrupted *)
		ELIBMAX* = 82; (* Attempting to link in too many shared libraries *)
		ELIBEXEC* = 83; (* Cannot exec a shared library directly *)
		EILSEQ* = 84; (* Illegal byte sequence *)
		ERESTART* = 85; (* Interrupted system call should be restarted *)
		ESTRPIPE* = 86; (* Streams pipe error *)
		EUSERS* = 87; (* Too many users *)
		ENOTSOCK* = 88; (* Socket operation on non-socket *)
		EDESTADDRREQ* = 89; (* Destination address required *)
		EMSGSIZE* = 90; (* Message too long *)
		EPROTOTYPE* = 91; (* Protocol wrong type for socket *)
		ENOPROTOOPT* = 92; (* Protocol not available *)
		EPROTONOSUPPORT* = 93; (* Protocol not supported *)
		ESOCKTNOSUPPORT* = 94; (* Socket type not supported *)
		EOPNOTSUPP* = 95; (* Operation not supported on transport endpoint *)
		EPFNOSUPPORT* = 96; (* Protocol family not supported *)
		EAFNOSUPPORT* = 97; (* Address family not supported by protocol *)
		EADDRINUSE* = 98; (* Address already in use *)
		EADDRNOTAVAIL* = 99; (* Cannot assign requested address *)
		ENETDOWN* = 100; (* Network is down *)
		ENETUNREACH* = 101; (* Network is unreachable *)
		ENETRESET* = 102; (* Network dropped connection because of reset *)
		ECONNABORTED* = 103; (* Software caused connection abort *)
		ECONNRESET* = 104; (* Connection reset by peer *)
		ENOBUFS* = 105; (* No buffer space available *)
		EISCONN* = 106; (* Transport endpoint is already connected *)
		ENOTCONN* = 107; (* Transport endpoint is not connected *)
		ESHUTDOWN* = 108; (* Cannot send after transport endpoint shutdown *)
		ETOOMANYREFS* = 109; (* Too many references: cannot splice *)
		ETIMEDOUT* = 110; (* Connection timed out *)
		ECONNREFUSED* = 111; (* Connection refused *)
		EHOSTDOWN* = 112; (* Host is down *)
		EHOSTUNREACH* = 113; (* No route to host *)
		EALREADY* = 114; (* Operation already in progress *)
		EINPROGRESS* = 115; (* Operation now in progress *)
		ESTALE* = 116; (* Stale file handle *)
		EUCLEAN* = 117; (* Structure needs cleaning *)
		ENOTNAM* = 118; (* Not a XENIX named type file *)
		ENAVAIL* = 119; (* No XENIX semaphores available *)
		EISNAM* = 120; (* Is a named type file *)
		EREMOTEIO* = 121; (* Remote I/O error *)
		EDQUOT* = 122; (* Quota exceeded *)
		ENOMEDIUM* = 123; (* No medium found *)
		EMEDIUMTYPE* = 124; (* Wrong medium type *)
		ECANCELED* = 125; (* Operation Canceled *)
		ENOKEY* = 126; (* Required key not available *)
		EKEYEXPIRED* = 127; (* Key has expired *)
		EKEYREVOKED* = 128; (* Key has been revoked *)
		EKEYREJECTED* = 129; (* Key was rejected by service *)
		EOWNERDEAD* = 130; (* Owner died *)
		ENOTRECOVERABLE* = 131; (* State not recoverable *)
		ERFKILL* = 132; (* Operation not possible due to RF-kill *)
		EHWPOISON* = 133; (* Memory page has hardware error *)


		WAIT_ANY* = -1;
		WCONTINUED* = {3};
		WNOHANG* = {0};
		WUNTRACED* = {1};


		NAME_MAX* = 255;

		SEEK_SET* = 0;
		SEEK_CUR* = 1;
		SEEK_END* = 2;

		STDIN_FILENO* = 0;
		STDOUT_FILENO* = 1;
		STDERR_FILENO* = 2;

		P_tmpdir* = "/tmp";

		(* O_RDWR, O_NONBLOCK (intFlags) *)
		O_ACCMODE* = {0,1};
		O_RDONLY* = {};
		O_WRONLY* = {0};
		O_RDWR* = {1};
		O_CREAT* = {6}; (* not fcntl *)
		O_EXCL* = {7}; (* not fcntl *)
		O_NOCTTY* = {8}; (* not fcntl *)
		O_TRUNC* = {9}; (* not fcntl *)
		O_APPEND* = {10};
		O_NONBLOCK* = {11};
		O_DSYNC* = {12}; (* used to be O_SYNC, see below *)
		O_DIRECT* = {14}; (* direct disk access hint *)
		O_LARGEFILE* = {15};
		O_DIRECTORY* = {16}; (* must be a directory *)
		O_NOFOLLOW* = {17}; (* don't follow links *)
		O_NOATIME* = {18};
		O_CLOEXEC* = {19}; (* set close_on_exec *)
		O_PATH* = {21};


		CLOCK_REALTIME* = 0;
		CLOCK_MONOTONIC* = 1;
		CLOCK_PROCESS_CPUTIME_ID* = 2;
		CLOCK_THREAD_CPUTIME_ID* = 3;
		CLOCK_MONOTONIC_RAW* = 4;
		CLOCK_REALTIME_COARSE* = 5;
		CLOCK_MONOTONIC_COARSE* = 6;
		CLOCK_BOOTTIME* = 7;
		CLOCK_REALTIME_ALARM* = 8;
		CLOCK_BOOTTIME_ALARM* = 9;
		CLOCK_SGI_CYCLE* = 10;
		CLOCK_TAI* = 11;


	TYPE
		StrArray* = POINTER TO ARRAY [untagged] OF PtrSTR;
		PtrSTR* = POINTER TO ARRAY [untagged] OF SHORTCHAR;

		(* PtrVoid, int, long, size_t, ssize_t, off_t, time_t, clock_t, sigjmp_buf *)
		(* mode_t, intFlags, sigset_t (set) *)
		PtrVoid* = INTEGER;
		int* = INTEGER;
		long* = INTEGER;
		ulong* = INTEGER;
		size_t* = SYSTEM.ADRINT;
		ssize_t* = SYSTEM.ADRINT;
		off_t* = INTEGER;
		clock_t* = INTEGER;
		clockid_t* = INTEGER;
		time_t* = INTEGER;
		mode_t* = SET;
		pid_t* = INTEGER;
		uid_t* = INTEGER;
		gid_t* = INTEGER;
		dev_t* = LONGINT;
		ino_t* = INTEGER;
		nlink_t* = INTEGER;
		blkcnt_t = INTEGER;
		blksize_t = INTEGER;
		int8_t* = SHORTCHAR;
		u_int8_t* = SHORTCHAR;
		int16_t* = SHORTINT;
		u_int16_t* = SHORTINT;
		int32_t* = INTEGER;
		u_int32_t* = INTEGER;
		int64_t* = LONGINT;
		u_int64_t* = LONGINT;
		wchar_t* = INTEGER;
		sigjmp_buf* = ARRAY [untagged] 39 OF INTEGER;
		intFlags* = SET;
		FILE = ARRAY [untagged] 37 OF INTEGER;
		sigset_t* = RECORD [untagged] 
			dummy: ARRAY [untagged] 128 OF BYTE;
		END;
		PtrSigset_t* = POINTER [untagged] TO sigset_t;


		tm* = POINTER TO tmDesc;
		tmDesc* = RECORD [untagged]
			(* NOTE: check record size *)
			(* tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec, tm_wday [ , tm_gmtoff ] *)
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/types/struct_tm.h: *)
				tm_sec*: int; (* Seconds.     [0-60] (1 leap second) *)
				tm_min*: int; (* Minutes.     [0-59] *)
				tm_hour*: int; (* Hours.       [0-23] *)
				tm_mday*: int; (* Day.         [1-31] *)
				tm_mon*: int; (* Month.       [0-11] *)
				tm_year*: int; (* Year - 1900.  *)
				tm_wday*: int; (* Day of week. [0-6] *)
				tm_yday*: int; (* Days in year.[0-365] *)
				tm_isdst*: int; (* DST.         [-1/0/1] *)

				tm_gmtoff*: int; (* long int *) (* Seconds east of UTC *)
				tm_zone*: PtrSTR; (* Timezone abbreviation *)
		END;

		(*Ptrsiginfo_t* = POINTER TO siginfo_t;
		siginfo_t = RECORD [untagged]
			(* si_code, fault address *)
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/types/siginfo_t.h: *)
				si_signo*: int; 	(* Signal number *)
				si_errno*: int;  	(* An errno value *)
				si_code*: int;   	(* Signal code *)

				_sifields*: RECORD [union]
					_pad: ARRAY [untagged] 29 OF int;
					_kill*: RECORD [untagged]
						si_pid*: pid_t;
						si_uid*: uid_t
					END;
					_timer*: RECORD [untagged]
						si_tid*: int;
						si_overrun*: int;
						si_sigval*: sigval_t
					END;
					_rt*: RECORD [untagged]
						si_pid*: pid_t;
						si_uid*: uid_t;
						si_sigval*: sigval_t
					END;
					_sigchild*: RECORD [untagged]
						si_pid: pid_t;
						si_uid*: uid_t;
						si_status*: int;
						si_utime*: clock_t;
						si_stime*: clock_t
					END;
					_sigfault*: RECORD [untagged]
						si_addr*: PtrVoid;
						si_addr_lsb*: SHORTINT;
						_bounds*: RECORD [union]
							_addr_bnd*: RECORD [untagged]
								_lower*: PtrVoid;
								_upper*: PtrVoid
							END;
							_pkey: INTEGER
						END
					END;
					_sigpoll: RECORD [untagged]
						si_band*: int; (* long int *);
						si_fd*: int
					END;
					_sigsys*: RECORD [untagged]
						_call_addr*: PtrVoid;
						_syscall*: int;
						_arch*: int (* unsigned int *)
					END
				END;
		END;*)

		Ptrucontext_t* = POINTER TO ucontext_t;
		ucontext_t = RECORD [untagged]
			(* IP, SP, FP *)
				uc_flags*: INTEGER;	(* unsigned long int *)
				uc_link*: Ptrucontext_t;
				uc_stack*: stack_t;
				uc_mcontext*: RECORD [untagged] (* mcontext_t *)
					gregs*: gregset_t;
					fpregs*: fpregset_t;
					oldmask*: INTEGER; (* unsigned long int *)
					cr2*: INTEGER; (* unsigned long int *)
				END;
				uc_sigmask: sigset_t;
				__fpregs_mem*: fpstate;

		END;

		(*sigaction_t* = RECORD [untagged]
(*
			sa_sigaction*: PROCEDURE [ccall] (sig: INTEGER; siginfo: Ptrsiginfo_t; context: Ptrucontext_t),
			sa_flags*: intFlags, sa_mask*: sigset_t
*)
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/asm/signal.h, /usr/include/i386-linux-gnu/bits/sigaction.h *)
				sa_sigaction*: PROCEDURE [ccall] (sig: INTEGER; siginfo: Ptrsiginfo_t; ptr: Ptrucontext_t); (* union with sa_handler*: PtrProc;*)
				sa_mask*: sigset_t;
				sa_flags*: intFlags;
				sa_restorer*: PROCEDURE [ccall];
		END;*)

		stack_t* = RECORD [untagged]
(*
			ss_sp*: PtrVoid, ss_size*: size_t, ss_flags*: intFlags
*)
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/types/stack_t.h: *)
				ss_sp*: PtrVoid;
				ss_flags*: intFlags;
				ss_size*: size_t;
		END;

		stat_t* = RECORD [untagged]
(*
			NOTE: check record size
			st_mode*: mode_t, st_size*: off_t, st_mtime*: time_t
*)
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/stat.h: *)
				st_dev*: dev_t;
				__pad1: SHORTINT;
				st_ino*: ino_t;
				st_mode*: mode_t;
				st_nlink*: nlink_t;
				st_uid*: uid_t;
				st_gid*: gid_t;
				st_rdev*: dev_t;
				__pad2: SHORTINT;
				st_size*: off_t;
				st_blksize*: blksize_t;
				st_blocks*: blkcnt_t;
				st_atim*: timespec_t;
				st_mtim*: timespec_t;
				st_ctim*: timespec_t;
				__glibc_reserved4: INTEGER; (* long int *)
				__glibc_reserved5: INTEGER; (* long int *)
		END;

		PtrFILE* = PtrVoid;
		PtrDIR* = PtrVoid;

		PtrDirent* = POINTER TO Dirent;
		Dirent = RECORD [untagged]
(*
			d_name*: ARRAY [untagged] NAME_MAX + 1 OF SHORTCHAR
*)
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/dirent.h: *)
				d_ino*: ino_t; (* inode number *)
				d_off*: off_t; (* offset to this dirent *)
				d_reclen*: SHORTINT; (* length of this d_name *)
				d_type*: BYTE;
				d_name*: ARRAY [untagged] 256 OF SHORTCHAR;
		END;

		timespec_t* = RECORD [untagged]
			(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/types/struct_timespec.h: *)
				tv_sec*: time_t;
				tv_nsec*: INTEGER;
		END;

(*
	VAR
		stdin*: INTEGER;
		timezone*: INTEGER; (* or tm.tm_gmtoff *)
*)
(*
	PROCEDURE- [ccall] __errno_location* (): PtrVoid;
*)
(*
	(* POSIX.1 *)
		PROCEDURE- stat* (path: PtrSTR; VAR sp: stat_t): int;
*)
	CONST
		(* Ubuntu 18.04 /usr/include/i386-linux-gnu/bits/stat.h *)
			_STAT_VER_LINUX* = 3;

	TYPE
		off64_t* = LONGINT;
		blkcnt64_t* = LONGINT;
		ino64_t* = LONGINT;

		(* Ubuntu 18.04 /usr/include/i386-linux-gnu/bits/stat.h: *)
			stat64_t* = RECORD [untagged]
				st_dev*: dev_t;
				__pad1: int; (* unsigned int *)
				__st_ino: ino_t;
				st_mode*: mode_t;
				st_nlink*: nlink_t;
				st_uid*: uid_t;
				st_gid*: gid_t;
				st_rdev*: dev_t;
				__pad2: int; (* unsigned int *)
				st_size*: off64_t;
				st_blksize*: blksize_t;
				st_blocks*: blkcnt64_t;
				st_atim*: timespec_t;
				st_mtim*: timespec_t;
				st_ctim*: timespec_t;
				st_ino*: ino64_t;
			END;

		(* Ubuntu 17.10 /usr/include/i386-linux-gnu/bits/types/sigval_t.h: *)
			(*sigval_t* = RECORD [union]
				sival_int*: int;
				sival_ptr*: PtrVoid
			END;*)

		(* Ubuntu 17.10 /usr/include/i386-linux-gnu/sys/ucontext.h: *)
			greg_t* = int;
			gregset_t* = ARRAY [untagged] 19 OF greg_t;
			fpregset_t* = POINTER [untagged] TO fpstate;
			fpreg* = RECORD [untagged]
				significand*: ARRAY [untagged] 4 OF SHORTINT; (* unsigned short int *)
				exponent*: SHORTINT; (* unsigned short int *)
			END;
			fpstate* = RECORD [untagged]
				cw*: INTEGER; (* unsigned long int *)
				sw*: INTEGER; (* unsigned long int *)
				tag*: INTEGER; (* unsigned long int *)
				ipoff*: INTEGER; (* unsigned long int *)
				cssel*: INTEGER; (* unsigned long int *)
				dataoff*: INTEGER; (* unsigned long int *)
				datasel*: INTEGER; (* unsigned long int *)
				_st: ARRAY [untagged] 8 OF fpreg;
				status*: INTEGER; (* unsigned long int *)
			END;
(*
	VAR
		timezone*: INTEGER; (* seconds from GMT *)
		stdin*, stdout*, stderr* : PtrFILE;
*)

	PROCEDURE- includestdio "#include <stdio.h>";
	PROCEDURE- includestdlib "#include <stdlib.h>";

	PROCEDURE- __errno_location*(): PtrVoid;

	PROCEDURE- __xstat* (version: int; filename: PtrSTR; VAR buf: stat_t): int;
	PROCEDURE- __xstat64* (version: int; filename: PtrSTR; VAR buf: stat64_t): int;
	PROCEDURE- fopen64* (path, mode: PtrSTR): PtrFILE;
	PROCEDURE- lseek64* (fd: int; offset: off64_t; whence: int): off64_t;
	PROCEDURE- fseeko64* (stream: PtrFILE; off: off64_t; whence: int): int;

	(*PROCEDURE- sigsetjmp* ["__sigsetjmp"] (VAR env: sigjmp_buf; savemask: int): int;*)


	(* ANSI C 89 *)
		PROCEDURE- clock* (): clock_t;

	(* POSIX.1 *)
		PROCEDURE- clock_gettime* (clock_id: clockid_t; VAR tp: timespec_t): int;

	PROCEDURE- mmap* (adr: PtrVoid; len: size_t; prot: intFlags; flags: intFlags; fd: int; offset: off_t): PtrVoid;
	(* BSD *)
		PROCEDURE- munmap* (adr: PtrVoid; len: size_t): int;
		PROCEDURE- mprotect* (adr: PtrVoid; len: size_t; prot: intFlags): int;

(*
	PROCEDURE- calloc* (nmemb: size_t; size: size_t): PtrVoid;
	(* ANSI C 89 *)
		PROCEDURE- malloc* (size: size_t): PtrVoid;
*)
		PROCEDURE- free* (ptr: PtrVoid) "free(ptr)";

	(* AT&T *)
		PROCEDURE- time* (VAR [nil] t: time_t): time_t;
	PROCEDURE- gmtime* (VAR [nil] t: time_t): tm;
	PROCEDURE- localtime* (VAR [nil] t: time_t): tm;

	(* POSIX.1 *)
(*
		PROCEDURE- sigsetjmp* (VAR env: sigjmp_buf; savemask: int): int;
*)
		PROCEDURE- siglongjmp* (VAR env: sigjmp_buf; val: int);

	(* POSIX.1 *)
		PROCEDURE- sigemptyset* (set: PtrSigset_t): int;
		PROCEDURE- sigfillset* (set: PtrSigset_t): int;
		PROCEDURE- sigaddset* (set: PtrSigset_t; signo: int): int;
		PROCEDURE- sigprocmask* (how: int; set: PtrSigset_t; oset: PtrSigset_t): int;

	(* POSIX.1 *)
		(*PROCEDURE- sigaction* (sig: int; VAR [nil] act: sigaction_t; VAR [nil] oact: sigaction_t): int;*)

	(* BSD *)
		PROCEDURE- sigaltstack* (VAR [nil] ss: stack_t; VAR [nil] oss: stack_t): int;

	(* ANSI C 89 *)
		PROCEDURE- getenv* (s: PtrSTR): PtrSTR;

	(* ANSI C 89 *)
		PROCEDURE- fopen* (path, mode: PtrSTR): PtrFILE;
		PROCEDURE- fdopen* (fildes: int; mode: PtrSTR): PtrFILE;
		PROCEDURE- fclose* (stream: PtrFILE): int;
		PROCEDURE- fread* (ptr: PtrVoid; size: size_t; nmemb: size_t; stream: PtrFILE): size_t;
		PROCEDURE- fwrite* (ptr: PtrVoid; size: size_t; nmemb: size_t; stream: PtrFILE): size_t
			"(SYSTEM_ADRINT)fwrite(ptr, (size_t)size, (size_t)nmemb, (FILE*)stream)";
		PROCEDURE- fflush* (s: PtrFILE): int;
		PROCEDURE- printf* (s: PtrSTR): int;
	(* ANSI C 89, XPG4 *)
		PROCEDURE- fseek* (stream: PtrFILE; offset: long; whence: int): int;

	(* POSIX.1 *)
		PROCEDURE- fileno* (stream: PtrFILE): int;

	(* POSIX.1 *)
		PROCEDURE- rename* (from, to: PtrSTR): int;
		PROCEDURE- mkdir* (path: PtrSTR; mode: mode_t): int;
		PROCEDURE- opendir* (filename: PtrSTR): PtrDIR;
		PROCEDURE- readdir* (dirp: PtrDIR): PtrDirent;
		PROCEDURE- closedir* (dirp: PtrDIR): int;
	(* ANSI C 89, XPG4.2 *)
		PROCEDURE- remove* (path: PtrSTR): int;

	(* POSIX.1 *)
		PROCEDURE- getcwd* (buf: PtrSTR; size: size_t): PtrSTR;

	(* ANSI C 99 *)
		PROCEDURE- exit* (status: int);

	(* ANSI C 89 *)
		PROCEDURE- strftime* (buf: PtrSTR; maxsize: size_t; format: PtrSTR; timeptr: tm): size_t;

	(* XXX: use fread instead *)
		PROCEDURE- fgets* (str: PtrSTR; size: int; stream: PtrFILE): PtrSTR;

	(* POSIX.1 *)
		PROCEDURE- open* (path: PtrSTR; flags: intFlags; mode: mode_t): int;
		PROCEDURE- write* (d: int; buf: PtrVoid; nbytes: size_t): ssize_t;
		PROCEDURE- read* (d: int; buf: PtrVoid; nbytes: size_t): ssize_t;
		PROCEDURE- close* (d: int): int;

	(* POSIX.1 *)
		PROCEDURE- chmod* (path: PtrSTR; mode: mode_t): int;
		PROCEDURE- fchmod* (fd: int; mode: mode_t): int;

	(* POSIX.1 *)
		PROCEDURE- fork* (): pid_t;
		PROCEDURE- waitpid* (wpid: pid_t; VAR [nil] status: int; options: intFlags): pid_t;

	(* POSIX.1 *)
		PROCEDURE- execv* (path: PtrSTR; argv: POINTER [untagged] TO ARRAY [untagged] OF PtrSTR): int;
		PROCEDURE- execvp* (file: PtrSTR; argv: POINTER [untagged] TO ARRAY [untagged] OF PtrSTR): int;

	(* POSIX.2 *)
		PROCEDURE- system* (string: PtrSTR): int;

	(* POSIX.1 *)
		PROCEDURE- sysconf* (name: int): long;

	PROCEDURE- popen* (command, type: PtrSTR): PtrFILE;
	PROCEDURE- pclose* (stream: PtrFILE): int;

END bbLinLibc.
