MODULE bbKernel;
(**
	project	= "armBox"
	organization	= "www.iermakov.ru"
	contributors	= "Ermakov Systima"
	version	= "System/Rsrc/About"
	copyright	= "System/Rsrc/About"
	license	= "GPL 3.0"
	changes	= ""
	issues	= ""

**)

	(* A. V. Shiryaev, 2012.11, 2013.10, 2016.11, 2018.06
		Linux Kernel
		Based on 1.7.1-a1 (build number 710) Windows Kernel
		Some parts taken from OpenBUGS linKernel

		Most Windows-specific code removed
		Some Windows-specific code commented and marked red
		Windows COM-specific code re-marked from green to gray
		Linux(/OpenBSD,/FreeBSD)-specific code marked green

		TODO:
			handle stack overflow exceptions
			Quit from TrapHandler
	*)

	IMPORT S := SYSTEM, Libc := bbLinLibc, LibW := bbLinLibW(*, Dl := LinDl, Rt := LinRt*);

	CONST
		strictStackSweep = TRUE;

		nameLen* = 256;

		littleEndian* = TRUE;
		timeResolution* = 1000;	(* ticks per second *)

		processor* = 10;	(* i386 *)

		objType* = "ocf";	(* file types *)
		symType* = "osf";
		docType* = "odc";

		(* loader constants *)
		done* = 0;
		fileNotFound* = 1;
		syntaxError* = 2;
		objNotFound* = 3;
		illegalFPrint* = 4;
		cyclicImport* = 5;
		noMem* = 6;
		commNotFound* = 7;
		commSyntaxError* = 8;
		moduleNotFound* = 9;

		any = 1000000;

		CX = 1;
		SP = 4;	(* register number of stack pointer *)
		FP = 5;	(* register number of frame pointer *)
		ML = 3;	(* register which holds the module list at program start *)

		N = 128 DIV 16;	(* free lists *)

		(* kernel flags in module desc *)
		init = 16; dyn = 17; dll = 24; iptrs = 30;

		(* meta interface consts *)
		mConst = 1; mTyp = 2; mVar = 3; mProc = 4; mField = 5;

		debug = FALSE;


(*
		sigStackSize = MAX(Libc.SIGSTKSZ, 65536);
*)

		trapReturn = 1; (* Return value for sigsetjmp given from siglongjmp *)

		(* constants for the message boxes *) 
		mbClose* = -1; mbOk* = 0; mbCancel* =1; mbRetry* = 2; mbIgnore* = 3; mbYes* = 4; mbNo* = 5;

	TYPE
		ADRINT = S.ADRINT;

		Name* = ARRAY nameLen OF CHAR;
		Utf8Name* = ARRAY nameLen OF SHORTCHAR;
		Command* = PROCEDURE;

		Module* = POINTER TO RECORD [untagged]
			next-: Module;
			opts-: SET;	(* 0..15: compiler opts, 16..31: kernel flags *)
			refcnt-: INTEGER;	(* <0: module invalidated *)
			compTime-, loadTime-: ARRAY 6 OF SHORTINT;
			body-: Command;	(* currently used as a pointer to the module's body *)
			term-: Command;	(* terminator *)
			nofimps-, nofptrs-: INTEGER;
			csize-, dsize-, rsize-: INTEGER;
			code-, data-, refs-: INTEGER;
			procBase-, varBase-: INTEGER;	(* meta base addresses *)
			names-: POINTER TO ARRAY [untagged] OF SHORTCHAR;	(* names[0] = 0X *)
			ptrs-: POINTER TO ARRAY [untagged] OF INTEGER;
			imports-: POINTER TO ARRAY [untagged] OF Module;
			export-: Directory;	(* exported objects (name sorted) *)
			name-: Utf8Name
		END;

		Type* = POINTER TO RECORD [untagged]
			(* record: ptr to method n at offset - 4 * (n+1) *)
			size-: INTEGER;	(* record: size, array: #elem, dyn array: 0, proc: sigfp *)
			mod-: Module;
			id-: INTEGER;	(* name idx * 256 + lev * 16 + attr * 4 + form *)
			base-: ARRAY 16 OF Type;	(* signature if form = ProcTyp *)
			fields-: Directory;	(* new fields (declaration order) *)
			ptroffs-: ARRAY any OF INTEGER	(* array of any length *)
		END;

		Object* = POINTER TO ObjDesc;

		ObjDesc* = RECORD [untagged]
			fprint-: INTEGER;
			offs-: INTEGER;	(* pvfprint for record types *)
			id-: INTEGER;	(* name idx * 256 + vis * 16 + mode *)
			struct-: Type	(* id of basic type or pointer to typedesc/signature *)
		END;

		Directory* = POINTER TO RECORD [untagged]
			num-: INTEGER;	(* number of entries *)
			obj-: ARRAY any OF ObjDesc	(* array of any length *)
		END;
		
		Signature* = POINTER TO RECORD [untagged]
			retStruct-: Type;	(* id of basic type or pointer to typedesc or 0 *)
			num-: INTEGER;	(* number of parameters *)
			par-: ARRAY any OF RECORD [untagged]	(* parameters *)
				id-: INTEGER;	(* name idx * 256 + kind *)
				struct-: Type	(* id of basic type or pointer to typedesc *)
			END
		END;

		Handler* = PROCEDURE;
(*
		Reducer* = POINTER TO ABSTRACT RECORD
			next: Reducer
		END;

		Identifier* = ABSTRACT RECORD
			typ*: INTEGER;
			obj-: ANYPTR
		END;

		TrapCleaner* = POINTER TO ABSTRACT RECORD
			next: TrapCleaner
		END;

		TryHandler* = PROCEDURE (a, b, c: INTEGER);


		(* meta extension suport *)

		ItemExt* = POINTER TO ABSTRACT RECORD END;

		ItemAttr* = RECORD
			obj*, vis*, typ*, adr*: INTEGER;
			mod*: Module;
			desc*: Type;
			ptr*: S.PTR;
			ext*: ItemExt
		END;

		Hook* = POINTER TO ABSTRACT RECORD END;

		LoaderHook* = POINTER TO ABSTRACT RECORD (Hook) 
			res*: INTEGER;
			importing*, imported*, object*: ARRAY 256 OF CHAR
		END;

		GuiHook* = POINTER TO ABSTRACT RECORD (Hook) END; (* Implemented by HostGnome *)

		Block = POINTER TO RECORD [untagged]
			tag: Type;
			last: INTEGER;		(* arrays: last element *)
			actual: INTEGER;	(* arrays: used during mark phase *)
			first: INTEGER		(* arrays: first element *)
		END;

		FreeBlock = POINTER TO FreeDesc;

		FreeDesc = RECORD [untagged]
			tag: Type;		(* f.tag = ADR(f.size) *)
			size: INTEGER;
			next: FreeBlock
		END;

		Cluster = POINTER TO RECORD [untagged]
			size: INTEGER;	(* total size *)
			next: Cluster;
			max: INTEGER	(* exe: reserved size, dll: original address *)
			(* start of first block *)
		END;

		FList = POINTER TO RECORD
			next: FList;
			blk: Block;
			iptr, aiptr: BOOLEAN
		END;

		CList = POINTER TO RECORD
			next: CList;
			do: Command;
			trapped: BOOLEAN
		END;


		PtrType = RECORD v: S.PTR END;	(* used for array of pointer *)
		Char8Type = RECORD v: SHORTCHAR END;
		Char16Type = RECORD v: CHAR END;
		Int8Type = RECORD v: BYTE END;
		Int16Type = RECORD v: SHORTINT END;
		Int32Type = RECORD v: INTEGER END;
		Int64Type = RECORD v: LONGINT END;
		BoolType = RECORD v: BOOLEAN END;
		SetType = RECORD v: SET END;
		Real32Type = RECORD v: SHORTREAL END;
		Real64Type = RECORD v: REAL END;
		ProcType = RECORD v: PROCEDURE END;
		UPtrType = RECORD v: INTEGER END;
		StrPtr = POINTER TO ARRAY [untagged] OF SHORTCHAR;


	VAR
		baseStack: ADRINT;	(* modList, root, and baseStack must be together for remote debugging *)
		root: Cluster;	(* cluster list *)
		modList-: Module;	(* root of module list *)
		trapCount-: INTEGER;
		err-, pc-, sp-, fp-, stack-, val-: INTEGER;
		comSig-: INTEGER;	(* command signature *)
		argc-: INTEGER;
		argv-: Libc.StrArray;

		free: ARRAY N OF FreeBlock;	(* free list *)
		sentinelBlock: FreeDesc;
		sentinel: FreeBlock;
		candidates: ARRAY 1024 OF INTEGER;
		nofcand: INTEGER;
		allocated: INTEGER;	(* bytes allocated on BlackBox heap *)
		total: INTEGER;	(* current total size of BlackBox heap *)
		used: INTEGER;	(* bytes allocated on system heap *)
		finalizers: FList;
		hotFinalizers: FList;
		cleaners: CList;
		reducers: Reducer;
		trapStack: TrapCleaner;
		actual: Module;	(* valid during module initialization *)

		res: INTEGER;	(* auxiliary global variables used for trap handling *)
		old: INTEGER;

		trapViewer, trapChecker: Handler;
		trapped, guarded, secondTrap: BOOLEAN;
		interrupted: BOOLEAN;
		static, inDll, terminating: BOOLEAN;
		restart: Command;

		(* told, shift: INTEGER;	(* used in Time() *) *)

		loader: LoaderHook;
		loadres: INTEGER;

		wouldFinalize: BOOLEAN;

		watcher*: PROCEDURE (event: INTEGER);	(* for debugging *)


(*
		sigStack: Libc.PtrVoid;
*)
		
		zerofd: INTEGER;
		pageSize: INTEGER;

		loopContext: Libc.sigjmp_buf; (* trap return context, if no Kernel.Try has been used. *)
		currentTryContext: POINTER TO Libc.sigjmp_buf; (* trap return context, if Kernel.Try has been used. *)
		isReadableContext: Libc.sigjmp_buf; (* for IsReadable *)
		isReadableCheck: BOOLEAN;

		guiHook: GuiHook;
		


	(* procedure for memory erase *)

	PROCEDURE Erase (adr: Libc.PtrVoid; words: INTEGER);
	BEGIN
		WHILE words # 0 DO S.PUT(adr, 0); INC(adr, 4); DEC(words) END
	END Erase;

	(* code procedure for stack allocate *)

	PROCEDURE [code] ALLOC (* argument in CX *)
	(*
		PUSH	EAX
		ADD	ECX,-5
		JNS	L0
		XOR	ECX,ECX
	L0: AND	ECX,-4	(n-8+3)/4*4
		MOV	EAX,ECX
		AND	EAX,4095
		SUB	ESP,EAX
		MOV	EAX,ECX
		SHR	EAX,12
		JEQ	L2
	L1: PUSH	0
		SUB	ESP,4092
		DEC	EAX
		JNE	L1
	L2: ADD	ECX,8
		MOV	EAX,[ESP,ECX,-4]
		PUSH	EAX
		MOV	EAX,[ESP,ECX,-4]
		SHR	ECX,2
		RET
	*);

	PROCEDURE [code] GetSystemModList (): Module "((Kernel_Module)SYSTEM_modlist)";
	PROCEDURE [code] GetFP (): ADRINT "(Kernel_ADRINT)__builtin_frame_address(0)";
	PROCEDURE [code] GetArgc (): INTEGER "SYSTEM_argCount";
	PROCEDURE [code] GetArgv (): Libc.StrArray "SYSTEM_argVector";

	PROCEDURE (VAR id: Identifier) Identified* (): BOOLEAN,	NEW, ABSTRACT;
	PROCEDURE (r: Reducer) Reduce* (full: BOOLEAN),	NEW, ABSTRACT;
	PROCEDURE (c: TrapCleaner) Cleanup*,	NEW, EMPTY;


	(* meta extension suport *)

	PROCEDURE (e: ItemExt) Lookup* (name: ARRAY OF CHAR; VAR i: ANYREC), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) Index* (index: INTEGER; VAR elem: ANYREC), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) Deref* (VAR ref: ANYREC), NEW, ABSTRACT;

	PROCEDURE (e: ItemExt) Valid* (): BOOLEAN, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) Size* (): INTEGER, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) BaseTyp* (): INTEGER, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) Len* (): INTEGER, NEW, ABSTRACT;

	PROCEDURE (e: ItemExt) Call* (OUT ok: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) BoolVal* (): BOOLEAN, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutBoolVal* (x: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) CharVal* (): CHAR, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutCharVal* (x: CHAR), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) IntVal* (): INTEGER, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutIntVal* (x: INTEGER), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) LongVal* (): LONGINT, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutLongVal* (x: LONGINT), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) RealVal* (): REAL, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutRealVal* (x: REAL), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) SetVal* (): SET, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutSetVal* (x: SET), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PtrVal* (): ANYPTR, NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutPtrVal* (x: ANYPTR), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) GetSStringVal* (OUT x: ARRAY OF SHORTCHAR;
																	OUT ok: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutSStringVal* (IN x: ARRAY OF SHORTCHAR;
																	OUT ok: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) GetStringVal* (OUT x: ARRAY OF CHAR; OUT ok: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (e: ItemExt) PutStringVal* (IN x: ARRAY OF CHAR; OUT ok: BOOLEAN), NEW, ABSTRACT;


	(* -------------------- miscellaneous tools -------------------- *)

	PROCEDURE Msg (IN str: ARRAY OF CHAR);
		VAR ss: ARRAY 1024 OF SHORTCHAR; res, l: INTEGER;
	BEGIN
		ss := SHORT(str);
		l := LEN(ss$);
		ss[l] := 0AX; ss[l + 1] := 0X;
		res := Libc.printf(ss)
	END Msg;

	PROCEDURE Int (x: LONGINT);
		VAR j, k: INTEGER; ch: CHAR; a, s: ARRAY 32 OF CHAR;
	BEGIN
		IF x # MIN(LONGINT) THEN
			IF x < 0 THEN s[0] := "-"; k := 1; x := -x ELSE k := 0 END;
			j := 0; REPEAT a[j] := CHR(x MOD 10 + ORD("0")); x := x DIV 10; INC(j) UNTIL x = 0
		ELSE
			a := "8085774586302733229"; s[0] := "-"; k := 1;
			j := 0; WHILE a[j] # 0X DO INC(j) END
		END;
		ASSERT(k + j < LEN(s), 20);
		REPEAT DEC(j); ch := a[j]; s[k] := ch; INC(k) UNTIL j = 0;
		s[k] := 0X;
		Msg(s);
	END Int;
	
	PROCEDURE (h: GuiHook)  MessageBox* (
		title, msg: ARRAY OF CHAR; buttons: SET): INTEGER, NEW, ABSTRACT;
	PROCEDURE (h: GuiHook)  Beep*, NEW, ABSTRACT;

	(* Is extended by HostGnome to show dialogs. If no dialog is present or
	    if the dialog is not closed by using one button, then "mbClose" is returned *)
	PROCEDURE MessageBox* (title, msg: ARRAY OF CHAR; buttons: SET): INTEGER;
		VAR res: INTEGER;
	BEGIN
		IF guiHook # NIL THEN
			res := guiHook.MessageBox(title, msg, buttons)
		ELSE
			Msg(" ");
			Msg("****");
			Msg("* " + title);
			Msg("* " + msg);
			Msg("****");
			res := mbClose;
		END;
		RETURN res
	END MessageBox;

	PROCEDURE SetGuiHook* (hook: GuiHook);
	BEGIN
		guiHook := hook
	END SetGuiHook;*)

	PROCEDURE IsAlpha* (ch: CHAR): BOOLEAN;
	BEGIN
		RETURN LibW.iswalpha(ORD(ch)) # 0	
	END IsAlpha;

	PROCEDURE Upper* (ch: CHAR): CHAR;
	BEGIN
		IF ("a" <= ch) & (ch <= "z") THEN RETURN CAP(ch) (* common case optimized *)
		ELSIF ch > 7FX THEN RETURN CHR(LibW.towupper(ORD(ch)))
		ELSE RETURN ch
		END
	END Upper;

	PROCEDURE IsUpper* (ch: CHAR): BOOLEAN;
	BEGIN
		IF ("A" <= ch) & (ch <= "Z") THEN RETURN TRUE
		ELSIF ch > 7FX THEN RETURN LibW.iswupper(ORD(ch)) # 0
		ELSE RETURN FALSE
		END
	END IsUpper;

	PROCEDURE Lower* (ch: CHAR): CHAR;
	BEGIN
		IF ("A" <= ch) & (ch <= "Z") THEN RETURN CHR(ORD(ch) + 32)
		ELSIF ch > 7FX THEN RETURN CHR(LibW.towlower(ORD(ch)))
		ELSE RETURN ch
		END
	END Lower;

	PROCEDURE IsLower* (ch: CHAR): BOOLEAN;
	BEGIN
		IF ("a" <= ch) & (ch <= "z") THEN RETURN TRUE
		ELSIF ch > 7FX THEN RETURN LibW.iswlower(ORD(ch)) # 0
		ELSE RETURN FALSE
		END
	END IsLower;

	PROCEDURE Utf8ToString* (IN in: ARRAY OF SHORTCHAR; OUT out: ARRAY OF CHAR; 
											OUT res: INTEGER);
		VAR i, j, val, max: INTEGER; ch: SHORTCHAR;
		
		PROCEDURE FormatError();
		BEGIN out := in$; res := 2 (*format error*)
		END FormatError;
		
	BEGIN
		ch := in[0]; i := 1; j := 0; max := LEN(out) - 1;
		WHILE (ch # 0X) & (j < max) DO
			IF ch < 80X THEN
				out[j] := ch; INC(j)
			ELSIF ch < 0E0X THEN
				val := ORD(ch) - 192;
				IF val < 0 THEN FormatError; RETURN END ;
				ch := in[i]; INC(i); val := val * 64 + ORD(ch) - 128;
				IF (ch < 80X) OR (ch >= 0E0X) THEN FormatError; RETURN END ;
				out[j] := CHR(val); INC(j)
			ELSIF ch < 0F0X THEN 
				val := ORD(ch) - 224;
				ch := in[i]; INC(i); val := val * 64 + ORD(ch) - 128;
				IF (ch < 80X) OR (ch >= 0E0X) THEN FormatError; RETURN END ;
				ch := in[i]; INC(i); val := val * 64 + ORD(ch) - 128;
				IF (ch < 80X) OR (ch >= 0E0X) THEN FormatError; RETURN END ;
				out[j] := CHR(val); INC(j)
			ELSE
				FormatError; RETURN
			END ;
			ch := in[i]; INC(i)
		END;
		out[j] := 0X;
		IF ch = 0X THEN res := 0 (*ok*) ELSE res := 1 (*truncated*) END
	END Utf8ToString;

	PROCEDURE StringToUtf8* (IN in: ARRAY OF CHAR; OUT out: ARRAY OF SHORTCHAR; 
											OUT res: INTEGER);
		VAR i, j, val, max: INTEGER;
	BEGIN
		i := 0; j := 0; max := LEN(out) - 3;
		WHILE (in[i] # 0X) & (j < max) DO
			val := ORD(in[i]); INC(i);
			IF val < 128 THEN
				out[j] := SHORT(CHR(val)); INC(j)
			ELSIF val < 2048 THEN
				out[j] := SHORT(CHR(val DIV 64 + 192)); INC(j);
				out[j] := SHORT(CHR(val MOD 64 + 128)); INC(j)
			ELSE
				out[j] := SHORT(CHR(val DIV 4096 + 224)); INC(j); 
				out[j] := SHORT(CHR(val DIV 64 MOD 64 + 128)); INC(j);
				out[j] := SHORT(CHR(val MOD 64 + 128)); INC(j)
			END;
		END;
		out[j] := 0X;
		IF in[i] = 0X THEN res := 0 (*ok*) ELSE res :=  1 (*truncated*) END
	END StringToUtf8;

	PROCEDURE SplitName* (IN name: ARRAY OF CHAR; OUT head, tail: ARRAY OF CHAR);
		(* portable *)
		VAR i, j: INTEGER; ch, lch: CHAR;
	BEGIN
		i := 0; ch := name[0];
		IF ch # 0X THEN
			REPEAT
				head[i] := ch; lch := ch; INC(i); ch := name[i]
			UNTIL (ch = 0X) OR (ch = ".") OR IsUpper(ch) & ~IsUpper(lch);
			IF ch = "." THEN i := 0; ch := name[0] END;
			head[i] := 0X; j := 0;
			WHILE ch # 0X DO tail[j] := ch; INC(i); INC(j); ch := name[i] END;
			tail[j] := 0X;
			IF tail = "" THEN tail := head$; head := "" END
		ELSE head := ""; tail := ""
		END
	END SplitName;

	PROCEDURE MakeFileName* (VAR name: ARRAY OF CHAR; IN type: ARRAY OF CHAR);
		VAR i, j: INTEGER; ext: ARRAY 8 OF CHAR; ch: CHAR;
	BEGIN
		i := 0;
		WHILE (name[i] # 0X) & (name[i] # ".") DO INC(i) END;
		IF name[i] = "." THEN
			IF name[i + 1] = 0X THEN name[i] := 0X END
		ELSE
			IF type = "" THEN ext := docType ELSE ext := type$ END;
			IF i < LEN(name) - LEN(ext$) - 1 THEN
				name[i] := "."; INC(i); j := 0; ch := ext[0];
				WHILE ch # 0X DO
					name[i] := Lower(ch); INC(i); INC(j); ch := ext[j]
				END;
				name[i] := 0X
			END
		END
	END MakeFileName;(*

	PROCEDURE Time* (): LONGINT;
		VAR (* t: INTEGER; *)
			res: INTEGER;
			tp: Rt.timespec_t;
	BEGIN
		(*
		t := WinApi.GetTickCount();
		IF t < told THEN INC(shift) END;
		told := t;
		RETURN shift * 100000000L + t
		*)

		(* A. V. Shiryaev: Linux *)
			res := Rt.clock_gettime(Rt.CLOCK_MONOTONIC, tp);
			ASSERT(res = 0);
			RETURN LONG(tp.tv_sec) * 1000 + tp.tv_nsec DIV 1000000
	END Time;

	PROCEDURE Beep* ();
		VAR ss: ARRAY 2 OF SHORTCHAR;
	BEGIN
		IF guiHook # NIL THEN
			guiHook.Beep
		ELSE
			ss[0] := 007X; ss[1] := 0X;
			res := Libc.printf(ss); res := Libc.fflush(Libc.NULL)
		END
	END Beep;

	PROCEDURE SearchProcVar* (var: INTEGER; VAR m: Module; VAR adr: INTEGER);
	BEGIN
		adr := var; m := NIL;
		IF var # 0 THEN
			m := modList;
			WHILE (m # NIL) & ((var < m.code) OR (var >= m.code + m.csize)) DO m := m.next END;
			IF m # NIL THEN DEC(adr, m.code) END
		END
	END SearchProcVar;


	(* -------------------- system memory management --------------------- *)

	(* A. V. Shiryaev, 2012.10: NOTE: it seems that GC works correctly with positive addesses only *)

(*
	PROCEDURE HeapAlloc (adr: INTEGER; size: INTEGER; prot: SET): Libc.PtrVoid;
		VAR
			x: Libc.PtrVoid;
			res: INTEGER;
	BEGIN
		x := Libc.calloc(1, size); (* calloc initialize allocated space to zero *)
		IF x # Libc.NULL THEN
			res := Libc.mprotect(x, size, prot);
			IF res # 0 THEN
				Libc.free(x);
				x := Libc.NULL;
				Msg("Kernel.HeapAlloc: mprotect failed!");
				HALT(100)
			END
		END;
		RETURN x
	END HeapAlloc;
*)
	PROCEDURE HeapAlloc (adr: Libc.PtrVoid; size: INTEGER; prot: SET): Libc.PtrVoid;
		VAR x: Libc.PtrVoid;
	BEGIN
		x := Libc.mmap(adr, size, prot, Libc.MAP_PRIVATE + Libc.MAP_ANON, zerofd, 0);
		IF x = Libc.MAP_FAILED THEN
			x := Libc.NULL
		ELSE
			ASSERT(size MOD 4 = 0, 100);
			Erase(x, size DIV 4)
		END;
		RETURN x
	END HeapAlloc;

(*
	PROCEDURE HeapFree (adr: Libc.PtrVoid; size: INTEGER);
		VAR res: INTEGER;
	BEGIN
(*
		ASSERT(size MOD 4 = 0, 100);
		Erase(adr, size DIV 4);
		res := Libc.mprotect(adr, size, Libc.PROT_NONE);
		ASSERT(res = 0, 101);
*)
		Libc.free(adr)
	END HeapFree;
*)
	PROCEDURE HeapFree (adr: Libc.PtrVoid; size: INTEGER);
		VAR res: INTEGER;
	BEGIN
(*
		ASSERT(size MOD 4 = 0, 100);
		Erase(adr, size DIV 4);
		res := Libc.mprotect(adr, size, Libc.PROT_NONE);
		ASSERT(res = 0, 101);
*)
		res := Libc.munmap(adr, size);
		ASSERT(res = 0, 102)
	END HeapFree;

	PROCEDURE AllocHeapMem (size: INTEGER; VAR c: Cluster);
		(* allocate at least size bytes, typically at least 256 kbytes are allocated *)
		CONST N = 65536;	(* cluster size for dll *)
			prot = Libc.PROT_READ + Libc.PROT_WRITE (* + Libc.PROT_EXEC *);
		VAR adr: INTEGER;
			allocated: INTEGER;
	BEGIN
		INC(size, 16);
		ASSERT(size > 0, 100); adr := 0;
		IF size < N THEN adr := HeapAlloc(65536, N, prot) END;
		IF adr = 0 THEN adr := HeapAlloc(65536, size, prot); allocated := size ELSE allocated := N END;
		IF adr = 0 THEN c := NIL
		ELSE
			c := S.VAL(Cluster, (adr + 15) DIV 16 * 16); c.max := adr;
			c.size := allocated - (S.VAL(INTEGER, c) - adr);
			INC(used, c.size); INC(total, c.size)
		END
		(* post: (c = NIL) OR (c MOD 16 = 0) & (c.size >= size) *)
	END AllocHeapMem;

	PROCEDURE FreeHeapMem (c: Cluster);
	BEGIN
		DEC(used, c.size); DEC(total, c.size);
		HeapFree(c.max, (S.VAL(INTEGER, c) - c.max) + c.size)
	END FreeHeapMem;

	PROCEDURE AllocModMem* (descSize, modSize: INTEGER; VAR descAdr, modAdr: INTEGER);
		CONST
			prot = Libc.PROT_READ + Libc.PROT_WRITE (* + Libc.PROT_EXEC *);
	BEGIN
		descAdr := HeapAlloc(0, descSize, prot);
		IF descAdr # 0 THEN
			modAdr := HeapAlloc(0, modSize, prot);
			IF modAdr # 0 THEN INC(used, descSize + modSize)
			ELSE HeapFree(descAdr, descSize); descAdr := 0
			END
		ELSE modAdr := 0
		END
	END AllocModMem;

	PROCEDURE DeallocModMem* (descSize, modSize, descAdr, modAdr: INTEGER);
	BEGIN
		DEC(used, descSize + modSize);
		HeapFree(descAdr, descSize);
		HeapFree(modAdr, modSize)
	END DeallocModMem;

	PROCEDURE InvalModMem (modSize, modAdr: INTEGER);
	BEGIN
		DEC(used, modSize);
		HeapFree(modAdr, modSize)
	END InvalModMem;

(*
	PROCEDURE IsReadable* (from, to: INTEGER): BOOLEAN;
		(* check wether memory between from (incl.) and to (excl.) may be read *)
	BEGIN
		RETURN WinApi.IsBadReadPtr(from, to - from) = 0
	END IsReadable;
*)

	(* Alexander Shiryaev, 2012.10: Linux: can be implemented through mincore/madvise *)
	(* This procedure can be called from TrapHandler also *)
	PROCEDURE IsReadable* (from, to: INTEGER): BOOLEAN;
		(* check wether memory between from (incl.) and to (excl.) may be read *)
		VAR res: BOOLEAN; res1: INTEGER;
			x: SHORTCHAR;
			mask, omask: Libc.sigset_t;
	BEGIN
		(* save old sigmask and unblock SIGSEGV *)
			res1 := Libc.sigemptyset(S.ADR(mask));
			ASSERT(res1 = 0, 100);
			res1 := Libc.sigaddset(S.ADR(mask), Libc.SIGSEGV);
			ASSERT(res1 = 0, 101);
			res1 := Libc.sigprocmask(Libc.SIG_UNBLOCK, S.ADR(mask), S.ADR(omask));
			ASSERT(res1 = 0, 102);

		res := FALSE;
		res1 := Libc.sigsetjmp(isReadableContext, Libc.TRUE);
		IF res1 = 0 THEN
			isReadableCheck := TRUE;
			(* read memory *)
			REPEAT
				S.GET(from, x);
				INC(from)
			UNTIL from = to;
			res := TRUE
		ELSE
			ASSERT(res1 = 1, 103)
		END;
		isReadableCheck := FALSE;

		(* restore saved sigmask *)
			res1 := Libc.sigprocmask(Libc.SIG_SETMASK, S.ADR(omask), NIL);
			ASSERT(res1 = 0, 104);

		RETURN res
	END IsReadable;

	(* --------------------- NEW implementation (portable) -------------------- *)

	PROCEDURE^ NewBlock (size: INTEGER): Block;

	PROCEDURE NewRec* (typ: INTEGER): INTEGER;	(* implementation of NEW(ptr) *)
		VAR size: INTEGER; b: Block; tag: Type; l: FList;
	BEGIN
		IF ODD(typ) THEN	(* record contains interface pointers *)
			tag := S.VAL(Type, typ - 1);
			b := NewBlock(tag.size);
			IF b = NIL THEN RETURN 0 END;
			b.tag := tag;
			l := S.VAL(FList, S.ADR(b.last));	(* anchor new object! *)
			l := S.VAL(FList, NewRec(S.TYP(FList)));	(* NEW(l) *)
			l.blk := b; l.iptr := TRUE; l.next := finalizers; finalizers := l;
			RETURN S.ADR(b.last)
		ELSE
			tag := S.VAL(Type, typ);
			b := NewBlock(tag.size);
			IF b = NIL THEN RETURN 0 END;
			b.tag := tag; S.GET(typ - 4, size);
			IF size # 0 THEN	(* record uses a finalizer *)
				l := S.VAL(FList, S.ADR(b.last));	(* anchor new object! *)
				l := S.VAL(FList, NewRec(S.TYP(FList)));	(* NEW(l) *)
				l.blk := b; l.next := finalizers; finalizers := l
			END;
			RETURN S.ADR(b.last)
		END
	END NewRec;

	PROCEDURE NewArr* (eltyp, nofelem, nofdim: INTEGER): INTEGER;	(* impl. of NEW(ptr, dim0, dim1, ...) *)
		VAR b: Block; size, headSize: INTEGER; t: Type; fin: BOOLEAN; l: FList;
	BEGIN
		IF (nofdim < 0) OR (nofdim > (MAX(INTEGER) - 12) DIV 4) THEN RETURN 0 END;
		headSize := 4 * nofdim + 12; fin := FALSE;
		CASE eltyp OF
(*
		| -1: eltyp := S.ADR(IntPtrType); fin := TRUE
*)
		| -1: HALT(100)
		| 0: eltyp := S.ADR(PtrType)
		| 1: eltyp := S.ADR(Char8Type)
		| 2: eltyp := S.ADR(Int16Type)
		| 3: eltyp := S.ADR(Int8Type)
		| 4: eltyp := S.ADR(Int32Type)
		| 5: eltyp := S.ADR(BoolType)
		| 6: eltyp := S.ADR(SetType)
		| 7: eltyp := S.ADR(Real32Type)
		| 8: eltyp := S.ADR(Real64Type)
		| 9: eltyp := S.ADR(Char16Type)
		| 10: eltyp := S.ADR(Int64Type)
		| 11: eltyp := S.ADR(ProcType)
		| 12: eltyp := S.ADR(UPtrType)
		ELSE	(* eltyp is desc *)
			IF ODD(eltyp) THEN DEC(eltyp); fin := TRUE END
		END;
		t := S.VAL(Type, eltyp);
		ASSERT(t.size > 0, 100);
		IF (nofelem < 0) OR (nofelem > (MAX(INTEGER) - headSize) DIV t.size) THEN RETURN 0 END;
		size := headSize + nofelem * t.size;
		b := NewBlock(size);
		IF b = NIL THEN RETURN 0 END;
		b.tag := S.VAL(Type, eltyp + 2);	(* tag + array mark *)
		b.last := S.ADR(b.last) + size - t.size;	(* pointer to last elem *)
		b.first := S.ADR(b.last) + headSize;	(* pointer to first elem *)
		IF fin THEN
			l := S.VAL(FList, S.ADR(b.last));	(* anchor new object! *)
			l := S.VAL(FList, NewRec(S.TYP(FList)));	(* NEW(l) *)
			l.blk := b; l.aiptr := TRUE; l.next := finalizers; finalizers := l
		END;
		RETURN S.ADR(b.last)
	END NewArr;


	(* -------------------- handler installation (portable) --------------------- *)

	PROCEDURE ThisFinObj* (VAR id: Identifier): ANYPTR;
		VAR l: FList;
	BEGIN
		ASSERT(id.typ # 0, 100);
		l := finalizers;
		WHILE l # NIL DO
			IF S.VAL(INTEGER, l.blk.tag) = id.typ THEN
				id.obj := S.VAL(ANYPTR, S.ADR(l.blk.last));
				IF id.Identified() THEN RETURN id.obj END
			END;
			l := l.next
		END;
		RETURN NIL
	END ThisFinObj;

	PROCEDURE InstallReducer* (r: Reducer);
	BEGIN
		r.next := reducers; reducers := r
	END InstallReducer;

	PROCEDURE InstallTrapViewer* (h: Handler);
	BEGIN
		trapViewer := h
	END InstallTrapViewer;

	PROCEDURE InstallTrapChecker* (h: Handler);
	BEGIN
		trapChecker := h
	END InstallTrapChecker;

	PROCEDURE PushTrapCleaner* (c: TrapCleaner);
		VAR t: TrapCleaner;
	BEGIN
		t := trapStack; WHILE (t # NIL) & (t # c) DO t := t.next END;
		ASSERT(t = NIL, 20);
		c.next := trapStack; trapStack := c
	END PushTrapCleaner;

	PROCEDURE PopTrapCleaner* (c: TrapCleaner);
		VAR t: TrapCleaner;
	BEGIN
		t := NIL;
		WHILE (trapStack # NIL) & (t # c) DO
			t := trapStack; trapStack := trapStack.next
		END
	END PopTrapCleaner;

	PROCEDURE InstallCleaner* (p: Command);
		VAR c: CList;
	BEGIN
		c := S.VAL(CList, NewRec(S.TYP(CList)));	(* NEW(c) *)
		c.do := p; c.trapped := FALSE; c.next := cleaners; cleaners := c
	END InstallCleaner;

	PROCEDURE RemoveCleaner* (p: Command);
		VAR c0, c: CList;
	BEGIN
		c := cleaners; c0 := NIL;
		WHILE (c # NIL) & (c.do # p) DO c0 := c; c := c.next END;
		IF c # NIL THEN
			IF c0 = NIL THEN cleaners := cleaners.next ELSE c0.next := c.next END
		END
	END RemoveCleaner;

	PROCEDURE Cleanup*;
		VAR c, c0: CList;
	BEGIN
		c := cleaners; c0 := NIL;
		WHILE c # NIL DO
			IF ~c.trapped THEN
				c.trapped := TRUE; c.do; c.trapped := FALSE; c0 := c
			ELSE
				IF c0 = NIL THEN cleaners := cleaners.next
				ELSE c0.next := c.next
				END
			END;
			c := c.next
		END
	END Cleanup;

	(* -------------------- meta information (portable) --------------------- *)

	PROCEDURE (h: LoaderHook) ThisMod* (IN name: ARRAY OF CHAR): Module, NEW, ABSTRACT;

	PROCEDURE SetLoaderHook*(h: LoaderHook);
	BEGIN
		loader := h
	END SetLoaderHook;

	PROCEDURE InitModule (mod: Module);	(* initialize linked modules *)
	BEGIN
		IF ~(dyn IN mod.opts) & (mod.next # NIL) & ~(init IN mod.next.opts) THEN InitModule(mod.next) END;
		IF ~(init IN mod.opts) THEN
			actual := mod; mod.body(); actual := NIL
		END
	END InitModule;

	PROCEDURE ThisLoadedMod* (IN name: ARRAY OF CHAR): Module;	(* loaded modules only *)
		VAR m: Module; res: INTEGER; n: Utf8Name;
	BEGIN
		StringToUtf8(name, n, res); ASSERT(res = 0);
		loadres := done;
		m := modList;
		WHILE (m # NIL) & ((m.name # n) OR (m.refcnt < 0)) DO m := m.next END;
		IF (m # NIL) & ~(init IN m.opts) THEN InitModule(m) END;
		IF m = NIL THEN loadres := moduleNotFound END;
		RETURN m
	END ThisLoadedMod;

	PROCEDURE ThisMod* (IN name: ARRAY OF CHAR): Module;
	BEGIN
		IF loader # NIL THEN
			loader.res := done;
			RETURN loader.ThisMod(name)
		ELSE
			RETURN ThisLoadedMod(name)
		END
	END ThisMod;

	PROCEDURE LoadMod* (IN name: ARRAY OF CHAR);
		VAR m: Module;
	BEGIN
		m := ThisMod(name)
	END LoadMod;

	PROCEDURE GetLoaderResult* (OUT res: INTEGER; OUT importing, imported, object: ARRAY OF CHAR);
	BEGIN
		IF loader # NIL THEN
			res := loader.res;
			importing := loader.importing$;
			imported := loader.imported$;
			object := loader.object$
		ELSE
			res := loadres;
			importing := "";
			imported := "";
			object := ""
		END
	END GetLoaderResult;

	PROCEDURE ThisObject* (mod: Module; IN name: ARRAY OF CHAR): Object;
		VAR l, r, m, res: INTEGER; p: StrPtr; n: Utf8Name;
	BEGIN
		StringToUtf8(name, n, res); ASSERT(res = 0);
		l := 0; r := mod.export.num;
		WHILE l < r DO	(* binary search *)
			m := (l + r) DIV 2;
			p := S.VAL(StrPtr, S.ADR(mod.names[mod.export.obj[m].id DIV 256]));
			IF p^ = n THEN RETURN S.VAL(Object, S.ADR(mod.export.obj[m])) END;
			IF p^ < n THEN l := m + 1 ELSE r := m END
		END;
		RETURN NIL
	END ThisObject;

	PROCEDURE ThisDesc* (mod: Module; fprint: INTEGER): Object;
		VAR i, n: INTEGER;
	BEGIN
		i := 0; n := mod.export.num;
		WHILE (i < n) & (mod.export.obj[i].id DIV 256 = 0) DO 
			IF mod.export.obj[i].offs = fprint THEN RETURN S.VAL(Object, S.ADR(mod.export.obj[i])) END;
			INC(i)
		END;
		RETURN NIL
	END ThisDesc;

	PROCEDURE ThisField* (rec: Type; IN name: ARRAY OF CHAR): Object;
		VAR n, res: INTEGER; p: StrPtr; obj: Object; m: Module; nn: Utf8Name;
	BEGIN
		StringToUtf8(name, nn, res); ASSERT(res = 0);
		m := rec.mod;
		obj := S.VAL(Object, S.ADR(rec.fields.obj[0])); n := rec.fields.num;
		WHILE n > 0 DO
			p := S.VAL(StrPtr, S.ADR(m.names[obj.id DIV 256]));
			IF p^ = nn THEN RETURN obj END;
			DEC(n); INC(S.VAL(INTEGER, obj), 16)
		END;
		RETURN NIL
	END ThisField;

	PROCEDURE ThisCommand* (mod: Module; IN name: ARRAY OF CHAR): Command;
		VAR x: Object;
	BEGIN
		x := ThisObject(mod, name);
		IF (x # NIL) & (x.id MOD 16 = mProc) & (x.fprint = comSig) THEN
			RETURN S.VAL(Command, mod.procBase + x.offs)
		END;
		RETURN NIL
	END ThisCommand;

	PROCEDURE ThisType* (mod: Module; IN name: ARRAY OF CHAR): Type;
		VAR x: Object;
	BEGIN
		x := ThisObject(mod, name);
		IF (x # NIL) & (x.id MOD 16 = mTyp) & (S.VAL(INTEGER, x.struct) DIV 256 # 0) THEN
			RETURN x.struct
		ELSE
			RETURN NIL
		END
	END ThisType;

	PROCEDURE TypeOf* (IN rec: ANYREC): Type;
	BEGIN
		RETURN S.VAL(Type, S.TYP(rec))
	END TypeOf;

	PROCEDURE LevelOf* (t: Type): SHORTINT;
	BEGIN
		RETURN SHORT(t.id DIV 16 MOD 16)
	END LevelOf;

	PROCEDURE NewObj* (VAR o: S.PTR; t: Type);
		VAR i: INTEGER;
	BEGIN
		IF t.size = -1 THEN o := NIL
		ELSE
			i := 0; WHILE t.ptroffs[i] >= 0 DO INC(i) END;
			IF t.ptroffs[i+1] >= 0 THEN INC(S.VAL(INTEGER, t)) END;	(* with interface pointers *)
			o := S.VAL(S.PTR, NewRec(S.VAL(INTEGER, t)))	(* generic NEW *)
		END
	END NewObj;

	PROCEDURE GetModName* (mod: Module; OUT name: Name);
		VAR res: INTEGER;
	BEGIN
		Utf8ToString(mod.name, name, res); ASSERT(res = 0)
	END GetModName;

	PROCEDURE GetObjName* (mod: Module; obj: Object; OUT name: Name);
		VAR p: StrPtr; res: INTEGER;
	BEGIN
		p := S.VAL(StrPtr, S.ADR(mod.names[obj.id DIV 256]));
		Utf8ToString(p^$, name, res); ASSERT(res = 0)
	END GetObjName;

	PROCEDURE GetTypeName* (t: Type; OUT name: Name);
		VAR p: StrPtr; res: INTEGER;
	BEGIN
		p := S.VAL(StrPtr, S.ADR(t.mod.names[t.id DIV 256]));
		Utf8ToString(p^$, name, res); ASSERT(res = 0)
	END GetTypeName;

	PROCEDURE RegisterMod* (mod: Module);
		VAR i: INTEGER;
			t: Libc.time_t; tm: Libc.tm;
	BEGIN
		(* mod.next := modList; modList := mod; *) mod.refcnt := 0; INCL(mod.opts, dyn); i := 0;
		WHILE i < mod.nofimps DO
			IF mod.imports[i] # NIL THEN INC(mod.imports[i].refcnt) END;
			INC(i)
		END;

		t := Libc.time(NIL);
		tm := Libc.localtime(t);
		mod.loadTime[0] := SHORT(tm.tm_year + 1900); (* Linux counts years from 1900 but BlackBox from 0000 *)
		mod.loadTime[1] := SHORT(tm.tm_mon + 1) (* Linux month range 0-11 but BB month range 1-12 *);
		mod.loadTime[2] := SHORT(tm.tm_mday);
		mod.loadTime[3] := SHORT(tm.tm_hour);
		mod.loadTime[4] := SHORT(tm.tm_min);
		mod.loadTime[5] := SHORT(tm.tm_sec); 
		tm := NIL;

		IF ~(init IN mod.opts) THEN InitModule(mod) END
	END RegisterMod;

	PROCEDURE^ Collect*;

	PROCEDURE UnloadMod* (mod: Module);
		VAR i: INTEGER; t: Command;
	BEGIN
		IF mod.refcnt = 0 THEN
			t := mod.term; mod.term := NIL;
			IF t # NIL THEN t() END;	(* terminate module *)
			i := 0;
			WHILE i < mod.nofptrs DO	(* release global pointers *)
				S.PUT(mod.varBase + mod.ptrs[i], 0); INC(i)
			END;
(*
			ReleaseIPtrs(mod);	(* release global interface pointers *)
*)
			Collect;	(* call finalizers *)
			i := 0;
			WHILE i < mod.nofimps DO	(* release imported modules *)
				IF mod.imports[i] # NIL THEN DEC(mod.imports[i].refcnt) END;
				INC(i)
			END;
			mod.refcnt := -1;
			IF dyn IN mod.opts THEN	(* release memory *)
				InvalModMem(mod.data + mod.dsize - mod.refs, mod.refs)
			END
		END
	END UnloadMod;

	(* -------------------- reference information (portable) --------------------- *)

	PROCEDURE RefCh (VAR ref: INTEGER; OUT ch: SHORTCHAR);
	BEGIN
		S.GET(ref, ch); INC(ref)
	END RefCh;

	PROCEDURE RefNum (VAR ref: INTEGER; OUT x: INTEGER);
		VAR s, n: INTEGER; ch: SHORTCHAR;
	BEGIN
		s := 0; n := 0; RefCh(ref, ch);
		WHILE ORD(ch) >= 128 DO INC(n, ASH(ORD(ch) - 128, s) ); INC(s, 7); RefCh(ref, ch) END;
		x := n + ASH(ORD(ch) MOD 64 - ORD(ch) DIV 64 * 64, s)
	END RefNum;

	PROCEDURE RefName (VAR ref: INTEGER; OUT n: Utf8Name);
		VAR i: INTEGER; ch: SHORTCHAR;
	BEGIN
		i := 0; RefCh(ref, ch);
		WHILE ch # 0X DO n[i] := ch; INC(i); RefCh(ref, ch) END;
		n[i] := 0X
	END RefName;

	PROCEDURE GetRefProc* (VAR ref: INTEGER; OUT adr: INTEGER; OUT name: Utf8Name);
		VAR ch: SHORTCHAR;
	BEGIN
		S.GET(ref, ch);
		WHILE ch >= 0FDX DO	(* skip variables *)
			INC(ref); RefCh(ref, ch);
			IF ch = 10X THEN INC(ref, 4) END;
			RefNum(ref, adr); RefName(ref, name); S.GET(ref, ch)
		END;
		WHILE (ch > 0X) & (ch < 0FCX) DO	(* skip source refs *)
			INC(ref); RefNum(ref, adr); S.GET(ref, ch)
		END;
		IF ch = 0FCX THEN INC(ref); RefNum(ref, adr); RefName(ref, name)
		ELSE adr := 0
		END
	END GetRefProc;

	(* A. V. Shiryaev, 2012.11 *)
	PROCEDURE CheckRefVarReadable (ref: INTEGER): BOOLEAN;
		VAR ok: BOOLEAN; ch: SHORTCHAR;
			p: INTEGER; (* address *)

		PROCEDURE Get;
		BEGIN
			IF ok THEN
				IF IsReadable(ref, ref+1) THEN (* S.GET(ref, ch); INC(ref) *) RefCh(ref, ch)
				ELSE ok := FALSE
				END
			END
		END Get;

		PROCEDURE Num;
		BEGIN
			Get; WHILE ok & (ORD(ch) >= 128) DO Get END
		END Num;

		PROCEDURE Name;
		BEGIN
			Get; WHILE ok & (ch # 0X) DO Get END
		END Name;

	BEGIN
		ok := TRUE;
		Get; (* mode *)
		IF ok & (ch >= 0FDX) THEN
			Get; (* form *)
			IF ok & (ch = 10X) THEN
				IF IsReadable(ref, ref + 4) THEN (* desc *)
					S.GET(ref, p); INC(ref, 4);
					ok := IsReadable(p + 2 * 4, p + 3 * 4) (* desc.id *)
				ELSE ok := FALSE
				END
			END;
			Num; Name
		END;
		RETURN ok
	END CheckRefVarReadable;

	PROCEDURE GetRefVar* (VAR ref: INTEGER; OUT mode, form: SHORTCHAR; OUT desc: Type;
																OUT adr: INTEGER; OUT name: Utf8Name);
	BEGIN
		IF CheckRefVarReadable(ref) THEN
			S.GET(ref, mode); desc := NIL;
			IF mode >= 0FDX THEN
				mode := SHORT(CHR(ORD(mode) - 0FCH));
				INC(ref); RefCh(ref, form);
				IF form = 10X THEN
					S.GET(ref, desc); INC(ref, 4); form := SHORT(CHR(16 + desc.id MOD 4))
				END;
				RefNum(ref, adr); RefName(ref, name)
			ELSE
				mode := 0X; form := 0X; adr := 0
			END
		ELSE
			Msg("Kernel.GetRefVar failed!"); Int(ref);
			mode := 0X; form := 0X; adr := 0
		END
	END GetRefVar;

	PROCEDURE SourcePos* (mod: Module; codePos: INTEGER): INTEGER;
		VAR ref, pos, ad, d: INTEGER; ch: SHORTCHAR; name: Utf8Name;
	BEGIN
		IF mod # NIL THEN	(* mf, 12.02.04 *)
			ref := mod.refs; pos := 0; ad := 0; S.GET(ref, ch);
			WHILE ch # 0X DO
				WHILE (ch > 0X) & (ch < 0FCX) DO	(* srcref: {dAdr,dPos} *)
					INC(ad, ORD(ch)); INC(ref); RefNum(ref, d);
					IF ad > codePos THEN RETURN pos END;
					INC(pos, d); S.GET(ref, ch)
				END;
				IF ch = 0FCX THEN	(* proc: 0FCX,Adr,Name *)
					INC(ref); RefNum(ref, d); RefName(ref, name); S.GET(ref, ch);
					IF (d > codePos) & (pos > 0) THEN RETURN pos END 
				END;
				WHILE ch >= 0FDX DO	(* skip variables: Mode, Form, adr, Name *)
					INC(ref); RefCh(ref, ch);
					IF ch = 10X THEN INC(ref, 4) END;
					RefNum(ref, d); RefName(ref, name); S.GET(ref, ch)
				END
			END;
		END;
		RETURN -1
	END SourcePos;

	(* -------------------- dynamic link libraries --------------------- *)

(*
	PROCEDURE DlOpen (name: ARRAY OF SHORTCHAR): Dl.HANDLE;
		CONST flags = Dl.RTLD_LAZY + Dl.RTLD_GLOBAL;
		VAR h: Dl.HANDLE;
			i: INTEGER;
	BEGIN
		h := Dl.NULL;
		i := 0; WHILE (i < LEN(name)) & (name[i] # 0X) DO INC(i) END;
		IF i < LEN(name) THEN
			h := Dl.dlopen(name, flags);
			WHILE (h = Dl.NULL) & (i > 0) DO
				DEC(i);
				WHILE (i > 0) & (name[i] # '.') DO DEC(i) END;
				IF i > 0 THEN
					name[i] := 0X;
					h := Dl.dlopen(name, flags);
					(* IF h # Dl.NULL THEN Msg(name$) END *)
				END
			END
		END;
		RETURN h
	END DlOpen;
*)

	PROCEDURE LoadDll* (IN name: ARRAY OF CHAR; VAR ok: BOOLEAN);
		VAR h: Dl.HANDLE; res: INTEGER; s: Utf8Name;
	BEGIN
		ok := FALSE;
		StringToUtf8(name, s, res); ASSERT(res = 0);
		h := Dl.dlopen(s, Dl.RTLD_LAZY +  Dl.RTLD_GLOBAL);
		IF h # Dl.NULL THEN ok := TRUE END
	END LoadDll;

	PROCEDURE ThisDllObj* (mode, fprint: INTEGER; IN dll, name: ARRAY OF CHAR): INTEGER;
		VAR ad: INTEGER; h: Dl.HANDLE; res: INTEGER; s: Utf8Name;
	BEGIN
		ad := 0;
		IF mode IN {mVar, mProc} THEN
			StringToUtf8(dll, s, res); ASSERT(res = 0);
			h := Dl.dlopen(s, Dl.RTLD_LAZY + Dl.RTLD_GLOBAL);
			IF h # Dl.NULL THEN StringToUtf8(name, s, res); ASSERT(res = 0);
				ad := Dl.dlsym(h, s)
			END
		END;
		RETURN ad
	END ThisDllObj;

	(* -------------------- garbage collector (portable) --------------------- *)

	PROCEDURE Mark (this: Block);
		VAR father, son: Block; tag: Type; flag, offset, actual: INTEGER;
	BEGIN
		IF ~ODD(S.VAL(INTEGER, this.tag)) THEN
			father := NIL;
			LOOP
				INC(S.VAL(INTEGER, this.tag));
				flag := S.VAL(INTEGER, this.tag) MOD 4;
				tag := S.VAL(Type, S.VAL(INTEGER, this.tag) - flag);
				IF flag >= 2 THEN actual := this.first; this.actual := actual
				ELSE actual := S.ADR(this.last)
				END;
				LOOP
					offset := tag.ptroffs[0];
					IF offset < 0 THEN
						INC(S.VAL(INTEGER, tag), offset + 4);	(* restore tag *)
						IF (flag >= 2) & (actual < this.last) & (offset < -4) THEN	(* next array element *)
							INC(actual, tag.size); this.actual := actual
						ELSE	(* up *)
							this.tag := S.VAL(Type, S.VAL(INTEGER, tag) + flag);
							IF father = NIL THEN RETURN END;
							son := this; this := father;
							flag := S.VAL(INTEGER, this.tag) MOD 4;
							tag := S.VAL(Type, S.VAL(INTEGER, this.tag) - flag);
							offset := tag.ptroffs[0];
							IF flag >= 2 THEN actual := this.actual ELSE actual := S.ADR(this.last) END;
							S.GET(actual + offset, father); S.PUT(actual + offset, S.ADR(son.last));
							INC(S.VAL(INTEGER, tag), 4)
						END
					ELSE
						S.GET(actual + offset, son);
						IF son # NIL THEN
							DEC(S.VAL(INTEGER, son), 4);
							IF ~ODD(S.VAL(INTEGER, son.tag)) THEN	(* down *)
								this.tag := S.VAL(Type, S.VAL(INTEGER, tag) + flag);
								S.PUT(actual + offset, father); father := this; this := son;
								EXIT
							END
						END;
						INC(S.VAL(INTEGER, tag), 4)
					END
				END
			END
		END
	END Mark;

	PROCEDURE MarkGlobals;
		VAR m: Module; i, p: INTEGER;
	BEGIN
		m := modList;
		WHILE m # NIL DO
			IF m.refcnt >= 0 THEN
				i := 0;
				WHILE i < m.nofptrs DO
					S.GET(m.varBase + m.ptrs[i], p); INC(i);
					IF p # 0 THEN Mark(S.VAL(Block, p - 4)) END
				END
			END;
			m := m.next
		END
	END MarkGlobals;

(*  This is the specification for the code procedure following below: *)

	PROCEDURE Next (b: Block): Block;	(* next block in same cluster *)
		VAR size: INTEGER;
	BEGIN
		S.GET(S.VAL(INTEGER, b.tag) DIV 4 * 4, size);
		IF ODD(S.VAL(INTEGER, b.tag) DIV 2) THEN INC(size, b.last - S.ADR(b.last)) END;
		RETURN S.VAL(Block, S.VAL(INTEGER, b) + (size + 19) DIV 16 * 16)
	END Next;

	PROCEDURE CheckCandidates;
	(* pre: nofcand > 0 *)
		VAR i, j, h, p, end: INTEGER; c: Cluster; blk, next: Block;
	BEGIN
		(* sort candidates (shellsort) *)
		h := 1; REPEAT h := h*3 + 1 UNTIL h > nofcand;
		REPEAT h := h DIV 3; i := h;
			WHILE i < nofcand DO p := candidates[i]; j := i;
				WHILE (j >= h) & (candidates[j-h] > p) DO
					candidates[j] := candidates[j-h]; j := j-h
				END;
				candidates[j] := p; INC(i)
			END
		UNTIL h = 1;
		(* sweep *)
		c := root; i := 0;
		WHILE c # NIL DO
			blk := S.VAL(Block, S.VAL(INTEGER, c) + 12);
			end := S.VAL(INTEGER, blk) + (c.size - 12) DIV 16 * 16;
			WHILE candidates[i] < S.VAL(INTEGER, blk) DO
				INC(i);
				IF i = nofcand THEN RETURN END
			END;
			WHILE S.VAL(INTEGER, blk) < end DO
				next := Next(blk);
				IF candidates[i] < S.VAL(INTEGER, next) THEN
					IF (S.VAL(INTEGER, blk.tag) # S.ADR(blk.last))	(* not a free block *)
							& (~strictStackSweep OR (candidates[i] = S.ADR(blk.last))) THEN
						Mark(blk)
					END;
					REPEAT
						INC(i);
						IF i = nofcand THEN RETURN END
					UNTIL candidates[i] >= S.VAL(INTEGER, next)
				END;
				IF (S.VAL(INTEGER, blk.tag) MOD 4 = 0) & (S.VAL(INTEGER, blk.tag) # S.ADR(blk.last))
						& (blk.tag.base[0] = NIL) & (blk.actual > 0) THEN	(* referenced interface record *)
					Mark(blk)
				END;
				blk := next
			END;
			c := c.next
		END
	END CheckCandidates;

	PROCEDURE MarkLocals;
		VAR sp: ADRINT; p, min, max: INTEGER; c: Cluster;
	BEGIN
		sp := GetFP(); nofcand := 0; c := root;
		WHILE c.next # NIL DO c := c.next END;
		min := S.VAL(INTEGER, root); max := S.VAL(INTEGER, c) + c.size;
		WHILE sp < baseStack DO
			S.GET(sp, p);
			IF (p > min) & (p < max) & (~strictStackSweep OR (p MOD 16 = 0)) THEN
				candidates[nofcand] := p; INC(nofcand);
				IF nofcand = LEN(candidates) - 1 THEN CheckCandidates; nofcand := 0 END
			END;
			INC(sp, 4)
		END;
		candidates[nofcand] := max; INC(nofcand);	(* ensure complete scan for interface mark*)
		IF nofcand > 0 THEN CheckCandidates END
	END MarkLocals;

	PROCEDURE MarkFinObj;
		VAR f: FList;
	BEGIN
		wouldFinalize := FALSE;
		f := finalizers;
		WHILE f # NIL DO
			IF ~ODD(S.VAL(INTEGER, f.blk.tag)) THEN wouldFinalize := TRUE END;
			Mark(f.blk);
			f := f.next
		END;
		f := hotFinalizers;
		WHILE f # NIL DO IF ~ODD(S.VAL(INTEGER, f.blk.tag)) THEN wouldFinalize := TRUE END;
			Mark(f.blk);
			f := f.next
		END
	END MarkFinObj;

	PROCEDURE CheckFinalizers;
		VAR f, g, h, k: FList;
	BEGIN
		f := finalizers; g := NIL;
		IF hotFinalizers = NIL THEN k := NIL
		ELSE
			k := hotFinalizers;
			WHILE k.next # NIL DO k := k.next END
		END;
		WHILE f # NIL DO
			h := f; f := f.next;
			IF ~ODD(S.VAL(INTEGER, h.blk.tag)) THEN
				IF g = NIL THEN finalizers := f ELSE g.next := f END;
				IF k = NIL THEN hotFinalizers := h ELSE k.next := h END;
				k := h; h.next := NIL
			ELSE g := h
			END
		END;
		h := hotFinalizers;
		WHILE h # NIL DO Mark(h.blk); h := h.next END
	END CheckFinalizers;

	PROCEDURE ExecFinalizer (a, b, c: INTEGER);
		VAR f: FList; fin: PROCEDURE(this: ANYPTR);
	BEGIN
		f := S.VAL(FList, a);
		IF f.aiptr THEN (* ArrFinalizer(S.VAL(ANYPTR, S.ADR(f.blk.last))) *)
		ELSE
			S.GET(S.VAL(INTEGER, f.blk.tag) - 4, fin);	(* method 0 *)
			IF (fin # NIL) & (f.blk.tag.mod.refcnt >= 0) THEN fin(S.VAL(ANYPTR, S.ADR(f.blk.last))) END;
(*
			IF f.iptr THEN RecFinalizer(S.VAL(ANYPTR, S.ADR(f.blk.last))) END
*)
		END
	END ExecFinalizer;

	PROCEDURE^ Try* (h: TryHandler; a, b, c: INTEGER);	(* COMPILER DEPENDENT *)

	PROCEDURE CallFinalizers;
		VAR f: FList;
	BEGIN
		WHILE hotFinalizers # NIL DO
			f := hotFinalizers; hotFinalizers := hotFinalizers.next;
			Try(ExecFinalizer, S.VAL(INTEGER, f), 0, 0)
		END;
		wouldFinalize := FALSE
	END CallFinalizers;

	PROCEDURE Insert (blk: FreeBlock; size: INTEGER);	(* insert block in free list *)
		VAR i: INTEGER;
	BEGIN
		blk.size := size - 4; blk.tag := S.VAL(Type, S.ADR(blk.size));
		i := MIN(N - 1, (blk.size DIV 16));
		blk.next := free[i]; free[i] := blk
	END Insert;

	PROCEDURE Sweep (dealloc: BOOLEAN);
		VAR cluster, last, c: Cluster; blk, next: Block; fblk, b, t: FreeBlock; end, i: INTEGER;
	BEGIN
		cluster := root; last := NIL; allocated := 0;
		i := N;
		REPEAT DEC(i); free[i] := sentinel UNTIL i = 0;
		WHILE cluster # NIL DO
			blk := S.VAL(Block, S.VAL(INTEGER, cluster) + 12);
			end := S.VAL(INTEGER, blk) + (cluster.size - 12) DIV 16 * 16;
			fblk := NIL;
			WHILE S.VAL(INTEGER, blk) < end DO
				next := Next(blk);
				IF ODD(S.VAL(INTEGER, blk.tag)) THEN
					IF fblk # NIL THEN
						Insert(fblk, S.VAL(INTEGER, blk) - S.VAL(INTEGER, fblk));
						fblk := NIL
					END;
					DEC(S.VAL(INTEGER, blk.tag));	(* unmark *)
					INC(allocated, S.VAL(INTEGER, next) - S.VAL(INTEGER, blk))
				ELSIF fblk = NIL THEN
					fblk := S.VAL(FreeBlock, blk)
				END;
				blk := next
			END;
			IF dealloc & (S.VAL(INTEGER, fblk) = S.VAL(INTEGER, cluster) + 12) THEN	(* deallocate cluster *)
				c := cluster; cluster := cluster.next;
				IF last = NIL THEN root := cluster ELSE last.next := cluster END;
				FreeHeapMem(c)
			ELSE
				IF fblk # NIL THEN Insert(fblk, end - S.VAL(INTEGER, fblk)) END;
				last := cluster; cluster := cluster.next
			END
		END;
		(* reverse free list *)
		i := N;
		REPEAT
			DEC(i);
			b := free[i]; fblk := sentinel;
			WHILE b # sentinel DO t := b; b := t.next; t.next := fblk; fblk := t END;
			free[i] := fblk
		UNTIL i = 0
	END Sweep;

	PROCEDURE Collect*;
	BEGIN
		IF root # NIL THEN
			CallFinalizers;	(* trap cleanup *)
			IF debug & (watcher # NIL) THEN watcher(1) END;
			MarkGlobals;
			MarkLocals;
			CheckFinalizers;
			Sweep(TRUE);
			CallFinalizers
		END
	END Collect;
	
	PROCEDURE FastCollect*;
	BEGIN
		IF root # NIL THEN
			IF debug & (watcher # NIL) THEN watcher(2) END;
			MarkGlobals;
			MarkLocals;
			MarkFinObj;
			Sweep(FALSE)
		END
	END FastCollect;

	PROCEDURE WouldFinalize* (): BOOLEAN;
	BEGIN
		RETURN wouldFinalize
	END WouldFinalize;

	(* --------------------- memory allocation (portable) -------------------- *)

	PROCEDURE OldBlock (size: INTEGER): FreeBlock;	(* size MOD 16 = 0 *)
		VAR b, l: FreeBlock; s, i: INTEGER;
	BEGIN
		IF debug & (watcher # NIL) THEN watcher(3) END;
		s := size - 4;
		i := MIN(N - 1, s DIV 16);
		WHILE (i # N - 1) & (free[i] = sentinel) DO INC(i) END;
		b := free[i]; l := NIL;
		WHILE b.size < s DO l := b; b := b.next END;
		IF b # sentinel THEN
			IF l = NIL THEN free[i] := b.next ELSE l.next := b.next END
		ELSE b := NIL
		END;
		RETURN b
	END OldBlock;

	PROCEDURE LastBlock (limit: INTEGER): FreeBlock;	(* size MOD 16 = 0 *)
		VAR b, l: FreeBlock; s, i: INTEGER;
	BEGIN
		s := limit - 4;
		i := 0;
		REPEAT
			b := free[i]; l := NIL;
			WHILE (b # sentinel) & (S.VAL(INTEGER, b) + b.size # s) DO l := b; b := b.next END;
			IF b # sentinel THEN
				IF l = NIL THEN free[i] := b.next ELSE l.next := b.next END
			ELSE b := NIL
			END;
			INC(i)
		UNTIL (b # NIL) OR (i = N);
		RETURN b
	END LastBlock;

	PROCEDURE NewBlock (size: INTEGER): Block;
		VAR tsize, a, s: INTEGER; b: FreeBlock; new, c: Cluster; r: Reducer;
	BEGIN
		ASSERT(size >= 0, 20);
		IF size > MAX(INTEGER) - 19 THEN RETURN NIL END;
		tsize := (size + 19) DIV 16 * 16;
		b := OldBlock(tsize);	(* 1) search for free block *)
		IF b = NIL THEN
			FastCollect; b := OldBlock(tsize);	(* 2) collect *)
			IF b = NIL THEN
				Collect; b := OldBlock(tsize);	(* 2a) fully collect *)
			END;
			IF b = NIL THEN
				AllocHeapMem(tsize + 12, new);	(* 3) allocate new cluster *)
				IF new # NIL THEN
					IF (root = NIL) OR (S.VAL(INTEGER, new) < S.VAL(INTEGER, root)) THEN
						new.next := root; root := new
					ELSE
						c := root;
						WHILE (c.next # NIL) & (S.VAL(INTEGER, new) > S.VAL(INTEGER, c.next)) DO c := c.next END;
						new.next := c.next; c.next := new
					END;
					b := S.VAL(FreeBlock, S.VAL(INTEGER, new) + 12);
					b.size := (new.size - 12) DIV 16 * 16 - 4
				ELSE
					RETURN NIL	(* 4) give up *)
				END
			END
		END;
		(* b # NIL *)
		a := b.size + 4 - tsize;
		IF a > 0 THEN Insert(S.VAL(FreeBlock, S.VAL(INTEGER, b) + tsize), a) END;
		IF size > 0 THEN Erase(S.ADR(b.size), (size + 3) DIV 4) END;
		INC(allocated, tsize);
		RETURN S.VAL(Block, b)
	END NewBlock;

	PROCEDURE Allocated* (): INTEGER;
	BEGIN
		RETURN allocated
	END Allocated;

	PROCEDURE Used* (): INTEGER;
	BEGIN
		RETURN used
	END Used;

	PROCEDURE Root* (): INTEGER;
	BEGIN
		RETURN S.VAL(INTEGER, root)
	END Root;


	(* -------------------- Trap Handling --------------------- *)

	PROCEDURE Start* (code: Command);
	BEGIN
		restart := code;
		baseStack := GetFP();	(* save base stack *)
		res := Libc.sigsetjmp(loopContext, Libc.TRUE);
		code()
	END Start;

	PROCEDURE Quit* (exitCode: INTEGER);
		VAR m: Module; term: Command; t: BOOLEAN;
			res: INTEGER;
	BEGIN
		trapViewer := NIL; trapChecker := NIL; restart := NIL;
		t := terminating; terminating := TRUE; m := modList;
		WHILE m # NIL DO	(* call terminators *)
			IF ~static OR ~t THEN
				term := m.term; m.term := NIL;
				IF term # NIL THEN term() END
			END;
(*
			ReleaseIPtrs(m);
*)
			m := m.next
		END;
		CallFinalizers;
		hotFinalizers := finalizers; finalizers := NIL;
		CallFinalizers;
(*
		IF ~inDll THEN
			RemoveExcp(excpPtr^);
			WinApi.ExitProcess(exitCode)	(* never returns *)
		END
*)

		res := Libc.fflush(0);
		Libc.exit(exitCode)
	END Quit;

	PROCEDURE FatalError* (id: INTEGER; str: ARRAY OF CHAR);
		VAR res: INTEGER; title: ARRAY 16 OF CHAR; text: ARRAY 256 OF SHORTCHAR;
	BEGIN
		title := "Error xy";
		title[6] := CHR(id DIV 10 + ORD("0"));
		title[7] := CHR(id MOD 10 + ORD("0"));
(*
		res := WinApi.MessageBoxW(0, str, title, {});
*)
		text := SHORT(str$);
		res := MessageBox(title$, SHORT(str), {mbOk});
(*
		IF ~inDll THEN RemoveExcp(excpPtr^) END;
*)
(*
		WinApi.ExitProcess(1)
*)
		Libc.exit(1)
		(* never returns *)
	END FatalError;

	PROCEDURE DefaultTrapViewer;
		VAR len, ref, end, x, a, b, c: INTEGER; mod: Module;
			modName, name: Name; res: INTEGER; n: Utf8Name; out: ARRAY 1024 OF CHAR;

		PROCEDURE WriteString (IN s: ARRAY OF CHAR);
			VAR i: INTEGER;
		BEGIN
			i := 0;
			WHILE (len < LEN(out) - 1) & (s[i] # 0X) DO out[len] := s[i]; INC(i); INC(len) END
		END WriteString;

		PROCEDURE WriteHex (x, n: INTEGER);
			VAR i, y: INTEGER;
		BEGIN
			IF len + n < LEN(out) THEN
				i := len + n - 1;
				WHILE i >= len DO
					y := x MOD 16; x := x DIV 16;
					IF y > 9 THEN y := y + (ORD("A") - ORD("0") - 10) END;
					out[i] := CHR(y + ORD("0")); DEC(i)
				END;
				INC(len, n)
			END
		END WriteHex;

		PROCEDURE WriteLn;
		BEGIN
			IF len < LEN(out) - 1 THEN out[len] := 0AX (* 0DX on Windows *); INC(len) END
		END WriteLn;

	BEGIN
		len := 0;
		IF err = 129 THEN WriteString("invalid with")
		ELSIF err = 130 THEN WriteString("invalid case")
		ELSIF err = 131 THEN WriteString("function without return")
		ELSIF err = 132 THEN WriteString("type guard")
		ELSIF err = 133 THEN WriteString("implied type guard")
		ELSIF err = 134 THEN WriteString("value out of range")
		ELSIF err = 135 THEN WriteString("index out of range")
		ELSIF err = 136 THEN WriteString("string too long")
		ELSIF err = 137 THEN WriteString("stack overflow")
		ELSIF err = 138 THEN WriteString("integer overflow")
		ELSIF err = 139 THEN WriteString("division by zero")
		ELSIF err = 140 THEN WriteString("infinite real result")
		ELSIF err = 141 THEN WriteString("real underflow")
		ELSIF err = 142 THEN WriteString("real overflow")
		ELSIF err = 143 THEN WriteString("undefined real result")
		ELSIF err = 200 THEN WriteString("keyboard interrupt")
		ELSIF err = 202 THEN WriteString("illegal instruction:  ");
			WriteHex(val, 4)
		ELSIF err = 203 THEN WriteString("illegal memory read [ad = ");
			WriteHex(val, 8); WriteString("]")
		ELSIF err = 204 THEN WriteString("illegal memory write [ad = ");
			WriteHex(val, 8); WriteString("]")
		ELSIF err = 205 THEN WriteString("illegal execution [ad = ");
			WriteHex(val, 8); WriteString("]")
		ELSIF err < 0 THEN WriteString("exception #"); WriteHex(-err, 2)
		ELSE err := err DIV 100 * 256 + err DIV 10 MOD 10 * 16 + err MOD 10;
			WriteString("trap #"); WriteHex(err, 3)
		END;
		a := pc; b := fp; c := 12;
		REPEAT
			WriteLn; WriteString("- ");
			mod := modList;
			WHILE (mod # NIL) & ((a < mod.code) OR (a >= mod.code + mod.csize)) DO mod := mod.next END;
			IF mod # NIL THEN
				DEC(a, mod.code);
				IF mod.refcnt >= 0 THEN
					GetModName(mod, modName); WriteString(modName); ref := mod.refs;
					REPEAT GetRefProc(ref, end, n) UNTIL (end = 0) OR (a < end);
					IF a < end THEN
						Utf8ToString(n, name, res); WriteString("."); WriteString(name)
					END
				ELSE
					GetModName(mod, modName); WriteString("("); WriteString(modName); WriteString(")")
				END;
				WriteString("  ")
			END;
			WriteString("(pc="); WriteHex(a, 8);
			WriteString(", fp="); WriteHex(b, 8); WriteString(")");
			IF (b >= sp) & (b < stack) THEN
				S.GET(b+4, a);	(* stacked pc *)
				S.GET(b, b);	(* dynamic link *)
				DEC(c)
			ELSE c := 0
			END
		UNTIL c = 0;
		out[len] := 0X;
		x := MessageBox("BlackBox", out, {mbOk})
	END DefaultTrapViewer;

	PROCEDURE TrapCleanup;
		VAR t: TrapCleaner;
	BEGIN
		WHILE trapStack # NIL DO
			t := trapStack; trapStack := trapStack.next; t.Cleanup
		END;
		IF (trapChecker # NIL) & (err # 128) THEN trapChecker END
	END TrapCleanup;

	PROCEDURE SetTrapGuard* (on: BOOLEAN);
	BEGIN
		guarded := on
	END SetTrapGuard;

	PROCEDURE Try* (h: TryHandler; a, b, c: INTEGER);	
		VAR res: INTEGER; context: Libc.sigjmp_buf; oldContext: POINTER TO Libc.sigjmp_buf;
	BEGIN
		oldContext := currentTryContext;
		res := Libc.sigsetjmp(context, Libc.TRUE);
		currentTryContext := S.ADR(context);
		IF res = 0 THEN (* first time around *)
			h(a, b, c);
		ELSIF res = trapReturn THEN  (* after a trap *)
		ELSE
			HALT(100)
		END;
		currentTryContext := oldContext;
	END Try;

	(* -------------------- Initialization --------------------- *)

	PROCEDURE [ccall] TrapHandler (sig: INTEGER; siginfo: Libc.Ptrsiginfo_t; context: Libc.Ptrucontext_t);
	BEGIN
		IF isReadableCheck THEN
			isReadableCheck := FALSE;
			Msg("~IsReadable");
			Libc.siglongjmp(isReadableContext, 1)
		END;

	(*
		S.GETREG(SP, sp);
		S.GETREG(FP, fp);
	*)
		stack := baseStack;

	(*
		sp := context.uc_mcontext.gregs[7]; (* TODO: is the stack pointer really stored in register 7? *)
		fp := context.uc_mcontext.gregs[6]; (* TODO: is the frame pointer really stored in register 6? *)
		pc := context.uc_mcontext.gregs[14]; (* TODO: is the pc really stored in register 14? *)
		val := siginfo._sifields._sigfault.si_addr;
	*)

	(*
		Int(sig); Int(siginfo.si_signo); Int(siginfo.si_code); Int(siginfo.si_errno);
		Int(siginfo.si_status); Int(siginfo.si_value); Int(siginfo.si_int);
	*)
		err := sig;
		IF trapped THEN DefaultTrapViewer END;
		CASE sig OF
			Libc.SIGINT: 
				err := 200; (* Interrupt (ANSI). *)
				Quit(0)
			| Libc.SIGILL: (* Illegal instruction (ANSI). *)
				err := 202; val := 0;
				IF IsReadable(pc, pc + 4) THEN
					S.GET(pc, val);
					IF val MOD 100H = 8DH THEN	(* lea reg,reg *)
						IF val DIV 100H MOD 100H = 0F0H THEN
							err := val DIV 10000H MOD 100H	(* trap *)
						ELSIF val DIV 1000H MOD 10H = 0EH THEN
							err := 128 + val DIV 100H MOD 10H	(* run time error *)
						END
					END
				END
			| Libc.SIGFPE: 
				CASE siginfo.si_code OF
					0: (* TODO: ?????? *)
						(*
						IF siginfo.si_int = 8 THEN
							err := 139
						ELSIF siginfo.si_int = 0 THEN
							err := 143
						END
						*)
					| Libc.FPE_INTDIV: err := 139 (* Integer divide by zero.  *)
					| Libc.FPE_INTOVF: err := 138 (* Integer overflow.  *)
					| Libc.FPE_FLTDIV: err := 140 (* Floating point divide by zero.  *)
					| Libc.FPE_FLTOVF: err := 142 (* Floating point overflow.  *)
					| Libc.FPE_FLTUND: err := 141 (* Floating point underflow.  *)
					| Libc.FPE_FLTRES: err := 143 (* Floating point inexact result.  *)
					| Libc.FPE_FLTINV: err := 143 (* Floating point invalid operation.  *)
					| Libc.FPE_FLTSUB: err := 134 (* Subscript out of range.  *)
				ELSE
				END
			| Libc.SIGSEGV: (* Segmentation violation (ANSI). *) 
				err := 203
		ELSE
		END;
		INC(trapCount);
		(* InitFpu; *)
		TrapCleanup;
		IF err # 128 THEN
			IF (trapViewer = NIL) OR trapped THEN
				DefaultTrapViewer
			ELSE
				trapped := TRUE;
				trapViewer();
				trapped := FALSE
			END
		END;
		IF currentTryContext # NIL THEN (* Try failed *)
			Libc.siglongjmp(currentTryContext, trapReturn)
		ELSE
			IF restart # NIL THEN (* Start failed *)
				Libc.siglongjmp(loopContext, trapReturn)
			END;
			Quit(1); (* FIXME *)
		END;
		trapped := FALSE
	END TrapHandler;

	PROCEDURE InstallSignals*;
		VAR sa, old: Libc.sigaction_t; res, i: INTEGER;
(*
			sigstk: Libc.stack_t;
			errno: INTEGER;
*)
	BEGIN
(*
		(* A. V. Shiryaev: Set alternative stack on which signals are to be processed *)
			sigstk.ss_sp := sigStack;
			sigstk.ss_size := sigStackSize;
			sigstk.ss_flags := 0;
			res := Libc.sigaltstack(sigstk, NIL);
			IF res # 0 THEN Msg("ERROR: Kernel.InstallSignals: sigaltstack failed!");
				S.GET( Libc.__errno_location(), errno );
				Int(errno);
				Libc.exit(1)
			END;
*)

		sa.sa_sigaction := TrapHandler;
(*
		res := LinLibc.sigemptyset(S.ADR(sa.sa_mask));
*)
		res := Libc.sigfillset(S.ADR(sa.sa_mask));
		sa.sa_flags := (* Libc.SA_ONSTACK + *) Libc.SA_SIGINFO; (* TrapHandler takes three arguments *)
		(*
		IF LinLibc.sigaction(LinLibc.SIGINT, sa, old) # 0 THEN Msg("failed to install SIGINT") END;
		IF LinLibc.sigaction(LinLibc.SIGILL, sa, old) # 0 THEN Msg("failed to install SIGILL") END;
		IF LinLibc.sigaction(LinLibc.SIGFPE, sa, old) # 0 THEN Msg("failed to install SIGFPE") END;
		IF LinLibc.sigaction(LinLibc.SIGSEGV, sa, old) # 0 THEN Msg("failed to install SIGSEGV") END;
		IF LinLibc.sigaction(LinLibc.SIGPIPE, sa, old) # 0 THEN Msg("failed to install SIGPIPE") END;
		IF LinLibc.sigaction(LinLibc.SIGTERM, sa, old) # 0 THEN Msg("failed to install SIGTERM") END;
		*)
		(* respond to all possible signals *)
		FOR i := 1 TO Libc._NSIG - 1 DO
			IF (i # Libc.SIGKILL)
				& (i # Libc.SIGSTOP)
				& (i # Libc.SIGWINCH)
			THEN
				IF Libc.sigaction(i, sa, old) # 0 THEN (* Msg("failed to install signal"); Int(i) *) END;
			END
		END
	END InstallSignals;

	PROCEDURE Init;
		VAR i: INTEGER; t: Type;
	BEGIN
(*
		(* for sigaltstack *)
			sigStack := Libc.calloc(1, sigStackSize);
			IF sigStack = Libc.NULL THEN
				Msg("ERROR: Kernel.Init: calloc(1, sigStackSize) failed!");
				Libc.exit(1)
			END;
*)
		(* for mmap *)
			zerofd := Libc.open("/dev/zero", Libc.O_RDWR, {0..8});
			IF zerofd < 0 THEN
				Msg("ERROR: Kernel.Init: can not open /dev/zero!");
				Libc.exit(1)
			END;
		(* for mprotect *)
			pageSize := Libc.sysconf(Libc._SC_PAGESIZE);
			IF pageSize < 0 THEN
				Msg("ERROR: Kernel.Init: pageSize < 0!");
				Libc.exit(1)
			END;

		IF LibW.setlocale(LibW.LC_ALL, "") = NIL THEN
			Msg("Kernel.Init: setlocale failed")
		END;

		isReadableCheck := FALSE;

		InstallSignals; (* init exception handling *)
		currentTryContext := NIL;

		t := S.VAL(Type, S.ADR(Command));	(* type desc of Command *)
		comSig := t.size;	(* size = signature fprint for proc types *)
		allocated := 0; total := 0; used := 0;
		sentinelBlock.size := MAX(INTEGER);
		sentinel := S.ADR(sentinelBlock);

(*
		S.PUTREG(ML, S.ADR(modList));
*)

		i := N;
		REPEAT DEC(i); free[i] := sentinel UNTIL i = 0;

		IF inDll THEN
(*
			baseStack := FPageWord(4);	(* begin of stack segment *)
*)
		END;
		(* InitFpu; *)
		IF ~static THEN
			InitModule(modList);
			IF ~inDll THEN Quit(1) END
		END;
		(* told := 0; shift := 0 *)
	END Init;

PROCEDURE [code] AincludeStdio "#include <stdio.h>";
PROCEDURE [code] Halt (n: INTEGER) 'printf("\nTerminated by HALT(%d).\n", n); exit(n)';
PROCEDURE Trap* (n: INTEGER); BEGIN Halt(n) END Trap;

BEGIN
	IF modList = NIL THEN	(* only once *)
		baseStack := GetFP();	(* S.GETREG(SP, baseStack); *)	(* TODO: Check that this is ok. *)
		argc := GetArgc(); argv := GetArgv();
		modList := GetSystemModList();	(* S.GETREG(ML, modList); *)	(* linker loads module list to BX *)
		static := init IN modList.opts;
		inDll := dll IN modList.opts;
		Init
	END
CLOSE
	IF ~terminating THEN
		terminating := TRUE;
		Quit(0)
	END*)
END bbKernel.
